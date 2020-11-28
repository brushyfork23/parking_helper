/*
* Based on:
* Ultrasonic Sensor HC-SR04 and Arduino Tutorial
* by Dejan Nedelkovski,
* www.HowToMechatronics.com
*
*/


// LED display
#include <FastLED.h>
const uint8_t NUM_LEDS = 10;
const uint8_t LED_PIN = 6;
const uint8_t FRAMES_PER_SECOND = 60;
CRGB leds[NUM_LEDS];

// Display colors
DEFINE_GRADIENT_PALETTE( warning_gp ) {
  0,     0,  255,  0,   //green
192,   255,  255,  0,   //bright yellow
255,   255,  0,    0 }; //red
CRGBPalette16 pallet = warning_gp;

// Ultrasonic sensor: HC-SR04
const uint8_t TRIGGER_PIN = 2;
const uint8_t ECHO_PIN = 3;
const unsigned long timeoutMicros = 1000000UL;

void setup() {
    Serial.begin(9600); // Starts the serial communication

    // LEDs
    FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, NUM_LEDS);
    fill_solid(leds, NUM_LEDS, CRGB(0,0,0));

    // Ultrasonic sensor
    pinMode(TRIGGER_PIN, OUTPUT); // Sets the trigger pin as an Output
    pinMode(ECHO_PIN, INPUT); // Sets the echo pin as an Input
    // Clears the trigger pin
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
}
void loop() {
    // Take a reading
    unsigned long distance = readDistance();

    // Prints the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.println(distance);

    // Scale distance to number of LEDs lit
    // Light a number of LEDs proportional to the distance
    uint8_t litLeds = constrain(map(distance, 30, 305, 0, NUM_LEDS/2), 0, NUM_LEDS/2);
    Serial.print("Fit LEDs: ");
    Serial.println(litLeds);
    fill_solid(leds, NUM_LEDS, CRGB(0,0,0));
    for(uint8_t i=0; i<litLeds; i++) {
        CRGB color = ColorFromPalette( pallet, 255-255*i/(NUM_LEDS/2));
        leds[i] = color;
        leds[NUM_LEDS-1-i] = color;
    };
    FastLED.show();

    FastLED.delay(1000UL / FRAMES_PER_SECOND);
}

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
