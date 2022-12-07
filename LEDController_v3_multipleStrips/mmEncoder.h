#ifndef MM_ENCODER_H
#define MM_ENCODER_H
#include "Arduino.h"
#include <Bounce2.h>
#include <Encoder.h>
// Wiring for the Encoder
// GND - to GND
//+ to 5v
// DT to PIN_DT
// CLK to PIN_CLK
//
#define ACK 128
#define ENC_CW 129
#define ENC_CCW 130
#define ENC_PUSH 131
#define ENC_LONG 132
#define INIT 133

//---THIS IS A CLASS THAT READS THE ENCODER
#define LONGPRESS_TIME 750
class mmEncoder
{

private:
  int PIN_DT, PIN_CLK, PIN_SW;
  long oldPosition;
  boolean gotLongPress;
  Encoder *knob;
  Bounce2::Button btnKnob;

public:
  mmEncoder(int dt, int clk, int sw)
  {
    this->PIN_DT = dt;
    this->PIN_CLK = clk;
    this->PIN_SW = sw;
    oldPosition = 0;
    gotLongPress = false;
    knob = new Encoder(PIN_DT, PIN_CLK);
    btnKnob.attach(PIN_SW);
    btnKnob.interval(35);
    btnKnob.setPressedState(HIGH);
  }

  void update()
  {
    // this works for the knob but we get 2 numbers printing for every 1 click
    long newPosition = knob->read();

    //% is modulus and this gives the remainder AFTER doing division
    if (newPosition % 4 == 0 && newPosition != oldPosition)
    {
      // if we are in here that means one full click happened so lets see which way we went

      // if the difference from the last number to this number is POSITIVE we turned it
      //  oldPosition is 0 and now the newPosition is 2 we turned it CLOCKWISE
      if (newPosition - oldPosition > 0)
      {
        Serial.print("<+>");
      }
      else
      {
        Serial.print("<->");
      }
      oldPosition = newPosition;
      // Serial.println(newPosition);
    }

    btnKnob.update();
    // Serial.println("Val: " + String(btnKnob.read()) + " CurrentDuration: " + btnKnob.currentDuration() +  " PrevDuration: " + btnKnob.previousDuration() );
    // Because we have to implement long press we no longer want to know when the button is pressed
    // we want to know when the button is released
    if (btnKnob.read() == LOW && btnKnob.currentDuration() > LONGPRESS_TIME && gotLongPress == false)
    { // HIGH MEAN PRESSED
      // if the button was longpressed
      Serial.print("<E>");
      // the line above was printing constantly while the button was still held down so we made a
      // flag variable to set when we detected a long press
      gotLongPress = true;
    }
    // the library saves how long the current state is happening as well as how long the previous
    // state happened, so when we pick the button up if it was HELD for a long period of time
    // then it should NOT count as a regular button press because we already sent the long press signal
    else if (btnKnob.released())
    {

      if (!gotLongPress)
      {
        Serial.print("<e>");
      }

      // if we had a long press when the button was held down we should reset that as soon as
      // the button is released so we can get more longpresses in the future
      gotLongPress = false;
    }
  }
};
//----END ENCODER CLASS---
#endif