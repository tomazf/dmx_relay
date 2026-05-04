/*
  DMX Sender Device with State Machine
  ==================================================
  Controls multiple RGB PAR lamps and DMX relay boxes.
  Uses ledBlink class for non-blocking LED control.

  Hardware:
  - Start button: Pin 12 (INPUT_PULLUP - LOW when pressed)
  - Stop button:  Pin 1 (INPUT_PULLUP - LOW when pressed)
  - Green LED 1:  Pin 2 (through ULN2003A - HIGH = ON)
  - Green LED 2:  Pin 3 (through ULN2003A - HIGH = ON)
  - Red LED:      Pin 4 (through ULN2003A - HIGH = ON)
  - DMX output:   Pin 1 (TX) through QY-838 RS485 module
 
  Libraries: Bounce2, DMXSerial, ledBlink (custom class)

  NOTE: Hardware serial (pins 0 & 1) is used for DMX.

  State           Red LED   Green LEDs                  DMX Output
  Boot/Idle       ON        OFF                         Scene OFF
  Start pressed   ON        Blinking (0.5-2.5s random)  Scene OFF (blinking)
  After blink     OFF       Solid ON                    Scene ON
  Stop pressed    ON        OFF                         Scene OFF

  date: 26.4.2026
  ver:  v0.21 @Ferbi
  for:  Kajak-kanu Tacen
*/

#include <Bounce2.h>
#include <DMXSerial.h>
#include "src/ledBlink.h"

// ============================================
// PIN DEFINITIONS
// ============================================
#define PIN_BTN_START       12
#define PIN_BTN_STOP        11
#define PIN_LED_GREEN_1     2
#define PIN_LED_GREEN_2     3
#define PIN_LED_RED         4
#define PIN_LED_DEBUG       13

// ============================================
// DMX DEVICE CONFIGURATION
// ============================================
// Each RGB PAR lamp uses 4 CONSECUTIVE channels:
// Starting address: Red channel
// Starting address + 1: Green channel
// Starting address + 2: Blue channel
// Starting address + 3: Dimmer (optional)

const uint16_t rgbLampStartAddresses[] = {
  10,   // Lamp uses channels RGB 10,11,12 + 13 dimmer
  20,
  30
};

const uint16_t relayAddresses[] = {
  40,     // Relay 1 at channel 40 - NO
  41,     // Relay 2 at channel 41 - NO
  42      // Relay 3 at channel 42 - NO
};

const uint16_t relayAddressesINV[] = {
  45,     // Relay 4 at channel 45 - NC
  46,     // Relay 5 at channel 46 - NC
  47      // Relay 6 at channel 47 - NC
};


// ============================================
// DMX VALUES
// ============================================
#define DMX_OFF     0
#define DMX_FULL    255

const uint8_t COLOR_OFF[] = {255, 0, 0, 255};   // Pure red
const uint8_t COLOR_ON[] = {0, 255, 0, 255};    // Pure green

// ============================================
// TIMING CONFIGURATION
// ============================================
#define BLINK_INTERVAL          100    // Blink speed (milliseconds)
#define BLINK_INTERVAL_DEBUG    1000   // Blink speed (milliseconds)
#define RANDOM_MIN_MS           500    // Minimum random delay (0.5 sec)
#define RANDOM_MAX_MS           2500   // Maximum random delay (2.5 sec)
#define DEBOUNCE_MS             25     // Button debounce time

// ============================================
// STATE MACHINE DEFINITIONS
// ============================================
enum SystemState {
  STATE_IDLE,                         // Waiting for Start - RED on, everything off, DMX OFF
  STATE_BLINKING,                     // Random blinking - greens blink, RED on
  STATE_ACTIVE                        // Active mode - greens solid, RED off, DMX ON
};

// ============================================
// GLOBAL VARIABLES
// ============================================
Bounce startButton = Bounce();
Bounce stopButton = Bounce();

// LED objects - HIGH = ON
ledBlink greenLED1(PIN_LED_GREEN_1);
ledBlink greenLED2(PIN_LED_GREEN_2);
ledBlink redLED(PIN_LED_RED);
ledBlink debugLED(PIN_LED_DEBUG);

SystemState currentState = STATE_IDLE;

// Timing for random blink period
unsigned long blinkEndTime = 0;

// Cache current scene to avoid unnecessary DMX writes
bool currentDMXSceneIsOn = false;

// Calculate number of devices automatically
const uint8_t numRGBLamps = sizeof(rgbLampStartAddresses) / sizeof(rgbLampStartAddresses[0]);
const uint8_t numRelays = sizeof(relayAddresses) / sizeof(relayAddresses[0]);
const uint8_t numRelaysINV = sizeof(relayAddressesINV) / sizeof(relayAddressesINV[0]);
uint16_t maxDMXChannel = 0;

// ============================================
// FUNCTION PROTOTYPES
// ============================================
void sendDMXSceneOff();
void sendDMXSceneOn();
void setAllRGBLamps(const uint8_t* rgb);
void setAllRelays(uint8_t value);
void setAllRelaysINV(uint8_t value);
void updateMaxDMXChannel();
void startBlinkSequence();
void checkState();

// ============================================
// SETUP
// ============================================
void setup() {
  // Seed random number generator
  delay(50);
  randomSeed(analogRead(A0));

  // ========== BUTTON INITIALIZATION ==========
  pinMode(PIN_BTN_START, INPUT);
  pinMode(PIN_BTN_STOP, INPUT);

  startButton.attach(PIN_BTN_START);
  startButton.interval(DEBOUNCE_MS);

  stopButton.attach(PIN_BTN_STOP);
  stopButton.interval(DEBOUNCE_MS);

  // ========== DMX INITIALIZATION ==========
  DMXSerial.init(DMXController);
  updateMaxDMXChannel();
  DMXSerial.maxChannel(maxDMXChannel);

  // ========== BOOT STATE ==========
  currentDMXSceneIsOn = false;
  sendDMXSceneOff();

  // ========== LED INITIALIZATION ==========
  // Set LEDs: RED ON, Greens OFF
  redLED.on();           // RED on (HIGH)
  greenLED1.off();       // Green off (LOW)
  greenLED2.off();       // Green off (LOW)

  // blink onboard LED for debug
  debugLED.set_blink(BLINK_INTERVAL_DEBUG, BLINK_INTERVAL_DEBUG);
  debugLED.blink();

  currentState = STATE_IDLE;
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  // read button states
  startButton.update();
  stopButton.update();

  // Update LEDs (required for blinking etc.)
  greenLED1.update();
  greenLED2.update();
  redLED.update();
  debugLED.update();

  // define state
  checkState();

  // State machine - each state handles its own DMX and LED behavior
  switch (currentState) {
    case STATE_IDLE:
      if (currentDMXSceneIsOn) {        // Only write to DMX if scene actually changed
        sendDMXSceneOff();
        currentDMXSceneIsOn = false;
      }
      redLED.on();
      greenLED1.off();
      greenLED2.off();
      break;

    case STATE_BLINKING:
      redLED.on();
      // green LEDs are turned ON with startBlinkSequence();

      if (currentDMXSceneIsOn) {        // Send OFF scene if not already active - just in case
        sendDMXSceneOff();
        currentDMXSceneIsOn = false;
      }
      break;

    case STATE_ACTIVE:
      if (!currentDMXSceneIsOn) {       // Send ON scene if not already active
        sendDMXSceneOn();
        currentDMXSceneIsOn = true;
      }
      redLED.off();
      greenLED1.on();
      greenLED2.on();
      break;
  }
}

// ============================================
// STATE CONTROL FUNCTIONS
// ============================================
void checkState() {
  // Handle button presses (state transitions)
  if (startButton.fell() && currentState == STATE_IDLE) {
    startBlinkSequence();
    currentState = STATE_BLINKING;
  }

  if (stopButton.fell() && (currentState == STATE_BLINKING || currentState == STATE_ACTIVE)) {
    currentState = STATE_IDLE;
  }

  if (currentState == STATE_BLINKING && millis() >= blinkEndTime) {
    currentState = STATE_ACTIVE;
  }
}

// ============================================
// DMX CONTROL FUNCTIONS
// ============================================
void sendDMXSceneOff() {
  setAllRGBLamps(COLOR_OFF);
  setAllRelays(DMX_OFF);
  setAllRelaysINV(DMX_FULL);
}

void sendDMXSceneOn() {
  setAllRGBLamps(COLOR_ON);
  setAllRelays(DMX_FULL);
  setAllRelaysINV(DMX_OFF);
}

void setAllRGBLamps(const uint8_t* rgb) {
  for (uint8_t i = 0; i < numRGBLamps; i++) {
    uint16_t baseAddr = rgbLampStartAddresses[i];
    DMXSerial.write(baseAddr, rgb[0]);
    DMXSerial.write(baseAddr + 1, rgb[1]);
    DMXSerial.write(baseAddr + 2, rgb[2]);
    DMXSerial.write(baseAddr + 3, rgb[3]);
  }
}

void setAllRelays(uint8_t value) {
  for (uint8_t i = 0; i < numRelays; i++) {
    DMXSerial.write(relayAddresses[i], value);
  }
}

void setAllRelaysINV(uint8_t value) {
  for (uint8_t i = 0; i < numRelaysINV; i++) {
    DMXSerial.write(relayAddressesINV[i], value);
  }
}

void updateMaxDMXChannel() {
  maxDMXChannel = 0;

  for (uint8_t i = 0; i < numRGBLamps; i++) {
    uint16_t lastChannel = rgbLampStartAddresses[i] + 3;
    if (lastChannel > maxDMXChannel) maxDMXChannel = lastChannel;
  }

  for (uint8_t i = 0; i < numRelays; i++) {
    if (relayAddresses[i] > maxDMXChannel) maxDMXChannel = relayAddresses[i];
  }

  for (uint8_t i = 0; i < numRelaysINV; i++) {
    if (relayAddressesINV[i] > maxDMXChannel) maxDMXChannel = relayAddressesINV[i];
  }

}

// ============================================
// LED CONTROL FUNCTIONS
// ============================================
void startBlinkSequence() {
  unsigned long duration = random(RANDOM_MIN_MS, RANDOM_MAX_MS + 1);
  blinkEndTime = millis() + duration;

  greenLED1.set_blink(BLINK_INTERVAL, BLINK_INTERVAL);
  greenLED2.set_blink(BLINK_INTERVAL, BLINK_INTERVAL);
  greenLED1.blink();
  greenLED2.blink();
}
