/*
  Blink.ino - Marc Quinton
    object oriented version of the classical blinking LED
*/

#if (ARDUINO <  100)
#include <WProgram.h>
#else
#include <Arduino.h>
#endif
#include <inttypes.h>

#include "ledBlink.h"

// CONSTRUCTORS
// ---------------------------------------------------------------------------

ledBlink::ledBlink(uint8_t pin)
{
  _ledPin = pin;
  pinMode(_ledPin, OUTPUT);

  _ledState = LOW;
  _previousMillis = 0;
  _static = 0;
}


ledBlink::ledBlink(uint8_t pin, long on, long off)
{
  _ledPin = pin;
  pinMode(_ledPin, OUTPUT);

  _OnTime = on;
  _OffTime = off;

  _ledState = LOW;
  _previousMillis = 0;
  _static = 0;
}

// PUBLIC METHODS
// ---------------------------------------------------------------------------
void ledBlink::update()
{
  if (_static == 0)
  {
    // check to see if it's time to change the state of the LED
    unsigned long currentMillis = millis();

    if ((_ledState == HIGH) && (currentMillis - _previousMillis >= _OnTime))
    {
      _ledState = LOW;  // Turn it off
      _previousMillis = currentMillis;      // Remember the time
      digitalWrite(_ledPin, _ledState);     // Update the actual LED
    }
    else if ((_ledState == LOW) && (currentMillis - _previousMillis >= _OffTime))
    {
      _ledState = HIGH;  // turn it on
      _previousMillis = currentMillis;      // Remember the time
      digitalWrite(_ledPin, _ledState);     // Update the actual LED
    }
  }
  else if ( _static == 2)
  {
    _ledState = LOW;                     // turn it on
    digitalWrite(_ledPin, _ledState);     // Update the actual LED
  }
  else if ( _static == 1)
  {
    _ledState = HIGH;                      // Turn it off
    digitalWrite(_ledPin, _ledState);     // Update the actual LED
  }

}

void ledBlink::on(){
  _static = 1;
}

void ledBlink::off() {
  _static = 2;
}

void ledBlink::set_blink(int on, int off) {
  _OnTime = on;
  _OffTime = off;
}

void ledBlink::blink() {
  _static = 0;
}

