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
// AsyncSonarLib - https://github.com/luisllamasbinaburo/Arduino-AsyncSonar
// YetAnotherArduinoPcIntLibrary - https://github.com/paulo-raca/YetAnotherArduinoPcIntLibrary/
// FastLED - https://github.com/FastLED/FastLED
// Chrono - https://github.com/SofaPirate/Chrono


//////////////////////////////////////////////
// Config
//////////////////////////////////////////////
// LED display
const uint8_t NUM_LEDS = 24;
const uint8_t LED_PIN = 3;
// Ultrasonic sensor
const uint8_t TRIGGER_PIN = 2;
const uint16_t MIN_TRIGGER_DISTANCE = 4;
const uint16_t MAX_TRIGGER_DISTANCE = 100;
const uint8_t INACTIVITY_SECONDS = 5;
const uint16_t MIN_DISPLAY_DISTANCE = 10; // distance in inches from sensor to parked bumper
const uint16_t MAX_DISPLAY_DISTANCE = 80;
const uint8_t FAST_TRIGGER_INTERVAL = 35;
const uint8_t SLOW_TRIGGER_INTERVAL = 250;

// #define DEBUG_DISTANCE
// #define DEBUG_LEDS


///////////////////////////////////////////////
// Definitions and Initialization
///////////////////////////////////////////////
// State machine
enum states {
    AWAY_STATE,     // Watching for a distance reading to appear.  Ping infrequently
    PARKING_STATE,  // Light display to show parking distance.  Ping constantly.  Transition after inactivity timeout.
    PARKED_STATE,   // Watch for car to begin backing up.  Ping infrequently.  Transition after distance timeout.
};
enum states state = AWAY_STATE;
bool isTransitioning = true;

// Timer
#include <LightChrono.h>

// LED display
#include <FastLED.h>
CRGB leds[NUM_LEDS];

// Display colors
DEFINE_GRADIENT_PALETTE( warning_gp ) {
  0,     0,  255,  0,   //green
128,   255,  255,  0,   //bright yellow
224,   255,  0,    0,   //red
255,   255,  0,    0 }; //red
CRGBPalette16 pallet = warning_gp;

// Ultrasonic sensor
#include "AsyncSonarLib.h"
LightChrono inactivityTimer;
unsigned int prevDistance = 0;


///////////////////////////////////////////////
// Async Sonar callback methods
///////////////////////////////////////////////
void PingRecieved(AsyncSonar& sensor)
{
    switch (state) {
        case PARKING_STATE:
            parkingPingReceived(sensor);
            break;
        case PARKED_STATE:
            parkedPingReceived(sensor);
            break;
        case AWAY_STATE:
            awayPingReceived(sensor);
            break;
   }
}

void PingTimeout(AsyncSonar& sensor)
{
    switch (state) {
        case PARKED_STATE:
            Serial.println("Timed out")
            state = AWAY_STATE;
            isTransitioning = true;
            break;
   }
}

AsyncSonar sonar(TRIGGER_PIN, PingRecieved, PingTimeout);


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
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    fill_solid(leds, NUM_LEDS, CRGB(0,0,0));

    // Ultrasonic sensor
    sonar.Start(500); // start in 500ms

    Serial.println();
    Serial.println("Running...");
    digitalWrite(LED_BUILTIN, LOW);
}


/////////////////////////////////////
// Loop
/////////////////////////////////////
void loop() {
  sonar.Update(&sonar);

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

// state: AWAY_STATE
// Watching for a distance reading to appear.  Ping infrequently
void tickAway() {
  
    if (isTransitioning) {
        isTransitioning = false;
        // Log the state transition
        Serial.println("AWAY");
        
        sonar.SetTriggerInterval(SLOW_TRIGGER_INTERVAL);
    }
}

void awayPingReceived(AsyncSonar& sensor) {
    unsigned int distance = sensor.GetFilteredMM() * .03937; // in inches

    #ifdef DEBUG_DISTANCE
    Serial.print("Away: ");
    Serial.print(distance);
    Serial.println(" inches");
    #endif
    
    if (distance > MIN_TRIGGER_DISTANCE && distance < MAX_TRIGGER_DISTANCE) {
        state = PARKING_STATE;
        isTransitioning = true;
    }
}

// state: PARKING_STATE
// Light display to show parking distance.  Ping constantly.  Transition after inactivity timeout.
void tickParking() {
    if (isTransitioning) {
        isTransitioning = false;
        // Log the state transition
        Serial.println("PARKING");

        sonar.SetTriggerInterval(FAST_TRIGGER_INTERVAL);

        inactivityTimer.restart();
    }

    // After motion inactivity, transition to PARKED_STATE
    if (inactivityTimer.hasPassed(INACTIVITY_SECONDS * 1000UL)) {
        state = PARKED_STATE;
        isTransitioning = true;
    }
}

void parkingPingReceived(AsyncSonar& sensor) {
    unsigned int distance = sensor.GetFilteredMM() * .03937; // in inches

    #ifdef DEBUG_DISTANCE
    Serial.print("Parking: ");
    Serial.print(distance);
    Serial.println(" inches");
    #endif
    
    // ignore readings out of bounds
    if (distance < MIN_TRIGGER_DISTANCE || distance > MAX_TRIGGER_DISTANCE) {
        return;
    }

    if (abs(distance-prevDistance) > 4) {
        prevDistance = distance;
        inactivityTimer.restart();
    }

    // convert distance to number of leds
    distance = constrain(distance, MIN_DISPLAY_DISTANCE, MAX_DISPLAY_DISTANCE);
    uint8_t litLeds = (NUM_LEDS/2) - (NUM_LEDS/2) * (distance-MIN_DISPLAY_DISTANCE) / (MAX_DISPLAY_DISTANCE-MIN_DISPLAY_DISTANCE);

    #ifdef DEBUG_LEDS
    Serial.print("Lit LEDs: ");
    Serial.println(litLeds);
    #endif

    for(uint8_t i=0; i<litLeds; i++) {
        uint8_t colorIndex = 255 * i / (NUM_LEDS/2);
        CRGB color = ColorFromPalette( pallet, colorIndex );
        leds[i] = color;
        leds[NUM_LEDS-1-i] = color;
    };
    FastLED.show();
}

// state: PARKED_STATE
// Watch for car to begin backing up.  Ping infrequently.  Transition after distance timeout.
void tickParked() {
    if (isTransitioning) {
        isTransitioning = false;
        // Log the state transition
        Serial.println("PARKED");

        // clear display
        FastLED.delay(100);
        fill_solid(leds, NUM_LEDS, CRGB(0,0,0));
        FastLED.show();

        sonar.SetTriggerInterval(SLOW_TRIGGER_INTERVAL);
    }
}

void parkedPingReceived(AsyncSonar& sensor) {
    unsigned int distance = sensor.GetFilteredMM() * .03937; // in inches

    #ifdef DEBUG_DISTANCE
    Serial.print("Parked: ");
    Serial.print(distance);
    Serial.println(" inches");
    #endif
    
    if (distance > MAX_TRIGGER_DISTANCE) {
        state = AWAY_STATE;
        isTransitioning = true;
    }
}
