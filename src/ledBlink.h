// ---------------------------------------------------------------------------
// Copyright 2021 - Under creative commons license 3.0:
//
// LED blink class
//  v0.1 - 12.12.2020 (basic functionalities)
//
// @author T. Ferbezar - tomaz.ferbezar@sc-nm.si
// ---------------------------------------------------------------------------

#ifndef blinkLED_h
#define blinkLED_h

#include <inttypes.h>

class ledBlink
{
  public:
  
    /*!
      @method
      @abstract   Class constructor.
      @discussion Initializes class variables

      @param     pin of LED
    */

    ledBlink (uint8_t);

    /*!
      @method
      @abstract   Class constructor.
      @discussion Initializes class variables

      @param     pin of LED, time_on, time_off
    */

    ledBlink (uint8_t, long, long);

    /*!
      @methods
      @abstract   Class constructor.
      @discussion Initializes class variables - update method
    */
    void update();

    /*!
      @methods
      @abstract   Class constructor.
      @discussion Initializes class variables - turn ON LED
    */
    void on();

    /*!
      @methods
      @abstract   Class constructor.
      @discussion Initializes class variables - turn OFF LED
    */
    void off();

    /*!
      @methods
      @abstract   Class constructor.
      @discussion Initializes blink mode

      @param     
    */
    void blink();

    /*!
      @methods
      @abstract   Class constructor.
      @discussion Initializes class variables - blink LED

      @param     time_on, time_off
    */
    void set_blink(int, int);

  private:

    /*!
      @variables
    */

    uint8_t _ledPin;                 // the number of the LED pin
    int _OnTime = 200;              // milliseconds of on-time
    int _OffTime = 200;             // milliseconds of off-time
    uint8_t _ledState;               // ledState used to set the LED
    unsigned long _previousMillis;   // will store last time LED was updated
    uint8_t _static;

};

#endif
