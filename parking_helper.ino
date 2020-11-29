/**
 * Parking Assistant.
 * An ultrasonic sensor detects when a car is being parked
 * and fills a bar of LEDs proportional to the remaining parking distance.
 */

////////////////////////////////////////////
// Hardware
////////////////////////////////////////////
// Upload to: Arduino Uno.
// LEDs: WS2812B
// Ultrasonic sensor: HC-SR04

/////////////////////////////////////////////
// Required Libraries
/////////////////////////////////////////////
// FastLED - https://github.com/FastLED/FastLED
// Chrono - https://github.com/SofaPirate/Chrono


//////////////////////////////////////////////
// Config
//////////////////////////////////////////////
// LED display
const uint8_t NUM_LEDS = 10;
const uint8_t LED_PIN = 6;
const uint8_t FRAMES_PER_SECOND = 60;
// Ultrasonic sensor
const uint8_t TRIGGER_PIN = 2;
const uint8_t ECHO_PIN = 3;
const uint16_t SWEET_SPOT = 31; // distance in centimeters from sensor to parked bumper


///////////////////////////////////////////////
// Definitions and Initialization
///////////////////////////////////////////////
// State machine
enum states {
    AWAY_STATE      // Watching for a distance reading to appear.  Ping infrequently
    PARKING_STATE,  // Light display to show parking distance.  Ping constantly.  Transition after inactivity timeout.
    PARKED_STATE,   // Watch for car to begin backing up.  Ping infrequently.  Transition after distance timeout.
};
enum states state = AWAY_STATE;
bool isTransitioning = true;

// Timer
#include <Chrono.h>
#include <LightChrono.h>

// LED display
#include <FastLED.h>
CRGB leds[NUM_LEDS];
LightChrono nextFrameTimer;
uint8_t level = 0;

// Display colors
DEFINE_GRADIENT_PALETTE( warning_gp ) {
  0,     0,  255,  0,   //green
192,   255,  255,  0,   //bright yellow
255,   255,  0,    0 }; //red
CRGBPalette16 pallet = warning_gp;

// Ultrasonic sensor
const unsigned long timeoutMicros = 1000000UL;
const uint8_t INACTIVITY_SECONDS = 5;
const uint16_t MIN_DISTANCE = 2;
const uint16_t MAX_DISTANCE = 400;
const uint8_t INFREQUENT_READS_PER_SECOND = 2;
const uint8_t MAX_READS_PER_SECOND = 10;
LightChrono readTimer;
LightChrono inactivityTimer;


/////////////////////////////////////
// Utility Methods
/////////////////////////////////////
// Get ultrasonic sensor distance reading in centimeters.
// Returns 0 if no distance found before sensor timeout.
unsigned long readDistance() {
    // Sets the trigger pin on HIGH state for 10 micro seconds
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);

    // Reads the echo pin, returns the sound wave travel time in microseconds
    unsigned long durationMicros = pulseIn(ECHO_PIN, HIGH, timeoutMicros);
    // Calculating the distance in centimeters
    unsigned long distanceCm= durationMicros*0.034/2;

    return distanceCm;
}


////////////////////////////////////////
// Setup
////////////////////////////////////////
void setup() {
    delay(100);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    // LEDs
    FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, NUM_LEDS);
    fill_solid(leds, NUM_LEDS, CRGB(0,0,0));

    // Ultrasonic sensor
    pinMode(TRIGGER_PIN, OUTPUT); // Sets the trigger pin as an Output
    pinMode(ECHO_PIN, INPUT); // Sets the echo pin as an Input
    digitalWrite(TRIGGER_PIN, LOW);  // Clear the trigger pin
    delayMicroseconds(2);

    Serial.println();
    Serial.println("Running...");
    digitalWrite(LED_BUILTIN, LOW);
}


/////////////////////////////////////
// Loop
/////////////////////////////////////
void loop() {
    switch (state) {
      case AWAY_STATE:
          tickAway();
          break;
      case PARKING_STATE:
          tickParking();
          break;
      case PARKED_STATE:
          tickParked();
          break;
   }
}


///////////////////////////////////////
// State methods
///////////////////////////////////////

// state: PARKING_STATE
// Light display to show parking distance.  Ping constantly.  Transition after inactivity timeout.
void tickParking() {
    if (isTransitioning) {
        isTransitioning = false;
        // Log the state transition
        Serial.println("PARKING");

        inactivityTimer.restart();
        readTimer.restart();
        nextFrameTimer.restart();
    }

    // After motion inactivity, transition to PARKED_STATE
    if (inactivityTimer.hasPassed(INACTIVITY_SECONDS * 1000UL)) {
        state = PARKED_STATE;
        isTransitioning = true;
    }

    // Take frequent readings
    if(readTimer.hasPassed(1000UL / MAX_READS_PER_SECOND, true)) {
        unsigned long distance = readDistance();

        Serial.print("Distance: ");
        Serial.println(distance);

        if (distance > 0) {
            inactivityTimer.reset();

            if (distance <= SWEET_SPOT) {
                level = 255;
            } else {
                level = map(distance, MIN_DISTANCE, MAX_DISTANCE, 255, 0);
            }
        }
    }

    // Draw the next frame
    if (nextFrameTimer.hasPassed(1000UL / FRAMES_PER_SECOND, true)) {
        uint8_t litLeds = (NUM_LEDS/2) * level / 255;
        for(uint8_t i=0; i<litLeds; i++) {
            uint8_t colorIndex = 255 * i / (NUM_LEDS/2);
            CRGB color = ColorFromPalette( pallet, colorIndex );
            leds[i] = color;
            leds[NUM_LEDS-1-i] = color;
        };
        FastLED.show();
    }
}

// state: PARKED_STATE
// Watch for car to begin backing up.  Ping infrequently.  Transition after distance timeout.
void tickParked() {
    if (isTransitioning) {
        isTransitioning = false;
        // Log the state transition
        Serial.println("PARKED");

        // clear display
        FastLED.delay(1000UL / FRAMES_PER_SECOND);
        fill_solid(leds, NUM_LEDS, CRGB(0,0,0));
        FastLED.show();

        readTimer.reset();
    }

    // Take infrequent readings
    if(readTimer.hasPassed(1000UL / INFREQUENT_READS_PER_SECOND, true)) {
        unsigned long distance = readDistance();

        Serial.print("Distance: ");
        Serial.println(distance);

        if (distance > 0) {
            state = AWAY_STATE;
            isTransitioning = true;
        }
    }
}

// state: AWAY_STATE
// Watching for a distance reading to appear.  Ping infrequently
void tickAway() {
    if (isTransitioning) {
        isTransitioning = false;
        // Log the state transition
        Serial.println("AWAY");
    }

    // Take infrequent readings
    if(readTimer.hasPassed(1000UL / INFREQUENT_READS_PER_SECOND, true)) {
        unsigned long distance = readDistance();

        Serial.print("Distance: ");
        Serial.println(distance);

        if (distance > 0) {
            state = PARKING_STATE;
            isTransitioning = true;
        }
    }
}