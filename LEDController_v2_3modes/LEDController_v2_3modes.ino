//need to add day and tungstein pot pwm non ws2812

//RGB Screen
//TOuch screen slide ability but also touching one of the options should highlight it and the encoder knob will also change that value


//Bounce2 is a library used for reading button presses
#include <Bounce2.h>
#include <Encoder.h>

//Wiring for the Encoder
//GND - to GND
//+ to 5v
//DT to PIN_DT
//CLK to PIN_CLK
//

//---THIS IS A CLASS THAT READS THE ENCODER
#define LONGPRESS_TIME 750
class mmEncoder {

  private:
    byte PIN_DT, PIN_CLK, PIN_SW;
    long oldPosition;
    boolean gotLongPress;
    Encoder * knob;
    Bounce2::Button btnKnob;

  public:
    mmEncoder(byte dt, byte clk, byte sw) {
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

    void update() {
      //this works for the knob but we get 2 numbers printing for every 1 click
      long newPosition = knob->read();

      //% is modulus and this gives the remainder AFTER doing division
      if (newPosition % 4 == 0 && newPosition != oldPosition) {
        //if we are in here that means one full click happened so lets see which way we went

        //if the difference from the last number to this number is POSITIVE we turned it
        // oldPosition is 0 and now the newPosition is 2 we turned it CLOCKWISE
        if (newPosition - oldPosition > 0) {
          Serial.print("<enc+>");
        } else {
          Serial.print("<enc->");
        }
        oldPosition = newPosition;
        //Serial.println(newPosition);
      }


      btnKnob.update();
      // Serial.println("Val: " + String(btnKnob.read()) + " CurrentDuration: " + btnKnob.currentDuration() +  " PrevDuration: " + btnKnob.previousDuration() );
      //Because we have to implement long press we no longer want to know when the button is pressed
      //we want to know when the button is released
      if ( btnKnob.read() == LOW  && btnKnob.currentDuration() > LONGPRESS_TIME && gotLongPress == false) { // HIGH MEAN PRESSED
        //if the button was longpressed
        Serial.print("<enc_l>");
        //the line above was printing constantly while the button was still held down so we made a
        //flag variable to set when we detected a long press
        gotLongPress = true;
      }
      //the library saves how long the current state is happening as well as how long the previous
      //state happened, so when we pick the button up if it was HELD for a long period of time
      //then it should NOT count as a regular button press because we already sent the long press signal
      else if (btnKnob.released()) {

        if (!gotLongPress) {
          Serial.print("<enc>");
        }

        //if we had a long press when the button was held down we should reset that as soon as
        //the button is released so we can get more longpresses in the future
        gotLongPress = false;
      }
    }


};
//----END ENCODER CLASS---


#define PIN_DT 9
#define PIN_CLK 10
#define PIN_SW 8
mmEncoder enc(PIN_DT, PIN_CLK, PIN_SW);

//5 Modes
//RGB
//INFERNO
//RAINBOW RUN
//Use a number to keep track of what mode we are in
//but you may add many modes and we don't want to have to memorize which numbers
//go with which modes so lets define them here
#define NUMBER_MODES 5
#define MODE_RGB 0
#define MODE_HSV 1
#define MODE_GRADIENT 2
#define MODE_PAPARAZZI 3
#define MODE_POLICE 4
#define DT         100   // delay time

//To add in another mode make another define line with a word that represents that mode, and use the next increasing number
//Also add one to the NUMBER_MODES variable above this
//For example:
//#define MODE_SOMETHING 3

//hold the number of the current mode here
int CURRENT_MODE = MODE_RGB;

//Button Wiring
//There are two pins here, they do not have polarity it does not matter which pins you chose for the following
//Pin1 - goes to the PIN_BTNMODE in this case Arduino D2
//Pin2 - goes to ground
//The button connects the two pins together when it is pressed

#include <FastLED.h>
//for most LED strips, and you can google the specific model number to check, each LED will pull 60ma when full brightness and white (all colors)
#define NUM_LEDS 250
#define DATA_PIN_WS2812B 53
#define DATA_PIN_WS2811 52
#define CLOCK_PIN 11


//This is an array - think of this like a row of lockers - where the locker numbers start at 0 and we use the locker number inside
//of brackets to open that locker
//Ex:  leds[0]  this opens up locker number 0 which is the first locker
CRGB leds[NUM_LEDS];

/*

   This program accepts the following commands

   To change modes use m:*MODE NUMBER
   m:2  would change to INFERNO MODE

   To change RGB values use
   r:*RED VALUE
   g:*GREEN VALUE
   b:*BLUE VALUE
   d: *DAYLIGHT VALUE
   t: *TUNGSTEN VALUE

   To change HSV values use


   To change gradient values use:
   sh:*START HUE VAL
   ss:*START SAT VAL
   sv:*START VAL VAL
   eh:*END HUE VAL
   es:*END SAT VAL
   ev:*END VAL VAL

   To change brightness:
   B:*BRIGHTNESS VALUE


*/

//RGB VARIABLES
int RED = 0, GREEN = 0, BLUE = 0;
int DAYLIGHT = 0, TUNGSTEN = 0;
//HSV variables
int HUE = 0, SAT = 0, VAL = 0;
//GRADIENT VARIABLES
int START_HUE = 0, START_SAT = 0, START_VAL = 0, END_HUE = 0, END_SAT = 0, END_VAL = 0;
//brightness variables
int BRIGHTNESS = 50;

//INFERNO MODE
uint8_t colorIndex[NUM_LEDS];

#define NUM_COLORS_PALETTE 5
DEFINE_GRADIENT_PALETTE (heatmap_gp) {
  0,   0,   0,  0, //index 0
  100,   0,   0,  0, //index 1
  110,   255, 30, 0, //index 2
  230,   255, 50, 0, //index 3
  255,   0,   0,  0 //index 4
};
CRGBPalette16 heatmap = heatmap_gp;

/*int HSV_GRADIENT[NUM_COLORS_PALETTE][3] = {
  {0,0,0},
  {0,0,0},
  {7,100,100},
  {12,100,100},
  {0,0,0}
  }*/

CHSV huePalette[NUM_COLORS_PALETTE] = {
  CHSV(0, 0, 0),
  CHSV(0, 0, 0),
  CHSV(7, 100, 100),
  CHSV(12, 100, 100),
  CHSV(0, 0, 0)
};/*
  int HSV_GRADIENT[NUM_COLORS_PALETTE][3] = {
  {0,0,0},
  {0,0,0},
  {7,100,100},
  {12,100,100},
  {0,0,0}
  };
*/

uint8_t hue = 0;
uint8_t paletteIndex = 0;

CRGBPalette16 flashPalette = CRGBPalette16 (
                               CRGB::White,
                               CRGB::Black,
                               CRGB::Black,
                               CRGB::White,

                               CRGB::Black,
                               CRGB::Black,
                               CRGB::Black,
                               CRGB::Black,

                               CRGB::Black,
                               CRGB::Black,
                               CRGB::Black,
                               CRGB::White,

                               CRGB::Black,
                               CRGB::Black,
                               CRGB::Black,
                               CRGB::White );

CRGBPalette16 myPal = flashPalette;

//Taking in commands
String incomingMsg = "";
long lastLEDBlink = 0;
boolean loadLEDStatus = false;
boolean isConnectedPC = false;

void setup() {
  Serial.begin(115200);
  //For right now only uncomment one of these at a time
  FastLED.addLeds<WS2812B, DATA_PIN_WS2812B, GRB>(leds, NUM_LEDS);
  //FastLED.addLeds<WS2811, DATA_PIN_WS2811, RBG>(leds, NUM_LEDS);

  //FastLED.addLeds<WS2812B, DATA_PIN, CLOCK_PIN, GRB>(leds, NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    colorIndex[i] = random8(); //generate a random number from 0 to 255
    //store the original definition values
    //   originalDefinitionVals[i] = myPal[i].v;
  }

  

  


  /* leds[0] = CRGB::Blue;
    leds[1] = CRGB::Red;
    leds[2] = CRGB::Green;
    FastLED.show();
    while(1){ }*/

}


void loop() {

  //get incoming data
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '<') {
      incomingMsg = "";
    } else if (c == '>') {
      monitorSerialMessages(incomingMsg);
    } else {
      incomingMsg += c;
    }
  }

  if (!isConnectedPC) {
    if (millis() - lastLEDBlink >= 500){
      lastLEDBlink = millis();
      //flip the led to the opposite
      loadLEDStatus = !loadLEDStatus;
      if (!loadLEDStatus){
         leds[0] = CRGB::Black;
      } else {
         leds[0] = CRGB::Red;
      }
      FastLED.show();     
    }
  } else {


    enc.update();

    //if we are in rgb mode run the rgbmode code
    if (CURRENT_MODE == MODE_RGB) {
      //because the button needs to be read as quickly as possible at all times we can no longer use delay(210) in the
      //mode rgb because that means we're stopping for 210ms and possibly missing a button press
      //the following should still run every 210ms BUT IT WONT BLOCK THE REST OF THE CODE, we can still read from the button
      EVERY_N_MILLISECONDS(210) {
        modeRGB();
      }
    } else if (CURRENT_MODE == MODE_HSV) {
      //cant use delay need to monitor encoder
      EVERY_N_MILLISECONDS(210) {
        modeHSV();
      }
    } else if (CURRENT_MODE == MODE_GRADIENT) {
      modeGradient();
    } else if (CURRENT_MODE == MODE_PAPARAZZI) {
      modePaparazzi();
    } else if (CURRENT_MODE == MODE_POLICE) {
      modePolice();
    }


  }




}


void monitorSerialMessages(String input) {

  if (input == "init") {
    isConnectedPC = true;
    sendMsgToScreen("ready");
    sendMsgToScreen("Started TRONICA 3030");
    return;
  }
  if (input == "lastload"){
    sendMsgToScreen("lastload");
    return;
  }

  //find the semicolon
  int colonIdx = input.indexOf(":");

  //if no colon then stop
  if (colonIdx == -1) {
    return;
  }

  //split the command at the semi colon
  String cmd = input.substring(0, colonIdx);
  int val = input.substring(colonIdx + 1).toInt();
  Serial.println("CMD: " + cmd + " VAL: " + String(val));



  //add a command to change modes

  if (cmd == "m") {
    //this means we are changing the mode
    //check to see if the mode is an invalid number, less than 0 or greater than the modes we have
    if (val > NUMBER_MODES - 1 || val < 0 ) {
      sendMsgToScreen("Error: this is a bad mode number - " + String(val));
    } else {
      //set the current mode and print out to let us know that the mode has changed
      CURRENT_MODE = val;
      if (CURRENT_MODE == MODE_RGB) {
        sendMsgToScreen("Changed into RGB Mode");
      } else if (CURRENT_MODE == MODE_HSV) {
        FastLED.setBrightness(255);
        FastLED.show();
        sendMsgToScreen("Changed into HSV Mode");
      } else if (CURRENT_MODE == MODE_GRADIENT) {
        sendMsgToScreen("Changed into Gradient Mode");
      } else if (CURRENT_MODE == MODE_PAPARAZZI) {
        sendMsgToScreen("Changed into Paparazzi");
      } else if (CURRENT_MODE == MODE_POLICE) {
        sendMsgToScreen("Changed into Police");
      }
    }
  }
  //add in commands for RGB mode (0 to 255)_
  else if (cmd == "r") {
    RED = val;
    sendMsgToScreen("Red changed to " + String(RED));
  }  else if (cmd == "g") {
    GREEN = val;
    sendMsgToScreen("Green changed to " + String(GREEN));
  }
  else if (cmd == "b") {
    BLUE = val;
    sendMsgToScreen("Blue changed to " + String(BLUE));
  } else if (cmd == "h") {
    HUE = val;
    sendMsgToScreen("Hue changed to " + String(HUE));
  }  else if (cmd == "s") {
    SAT = val;
    sendMsgToScreen("Sat changed to " + String(SAT));
  }
  else if (cmd == "v") {
    VAL = val;
    sendMsgToScreen("Val changed to " + String(VAL));
  }
  else if (cmd == "d") {
    DAYLIGHT = val;
    sendMsgToScreen("Daylight changed to " + String(DAYLIGHT));
  }
  else if (cmd == "t") {
    TUNGSTEN = val;
    sendMsgToScreen("Tungsten changed to " + String(TUNGSTEN));
  }
  //add in commands for the gradient values - (0 to 255)
  else if (cmd == "sh") {
    START_HUE = val;
    sendMsgToScreen("START HUE changed to " + String(START_HUE));
  } else if (cmd == "ss") {
    START_SAT = val;
    sendMsgToScreen("START SAT changed to " + String(START_SAT));
  }  else if (cmd == "sv") {
    START_VAL = val;
    sendMsgToScreen("START VAL changed to " + String(START_VAL));
  } else if (cmd == "eh") {
    END_HUE = val;
    sendMsgToScreen("END HUE changed to " + String(END_HUE));
  } else if (cmd == "es") {
    END_SAT = val;
    sendMsgToScreen("END SAT changed to " + String(END_SAT));
  }  else if (cmd == "ev") {
    END_VAL = val;
    sendMsgToScreen("END VAL changed to " + String(END_VAL));
  } else if (cmd == "B") {
    BRIGHTNESS = val;
    sendMsgToScreen("BRIGHTNESS changed to " + String(BRIGHTNESS));
  }

}

void sendMsgToScreen(String msg) {
  Serial.print("<" + msg + ">");
}

void modeInferno() {
  //SETUP INFERNO MODE
  for (int i = 0; i < NUM_LEDS; i++) {
    colorIndex[i] = random8();
  }



  EVERY_N_MILLISECONDS(2000) {
    sendMsgToScreen("Inferno");
  }
}

void modeRGB() {

  //set the brightness of the led strip with the value that was converted to the range of 0 to 255
  FastLED.setBrightness(BRIGHTNESS);
  //go through all of the LEDs and set each one to have the values calculated above
  for (int i = 0; i < NUM_LEDS; i++)  {
    leds[i] = CRGB(RED, GREEN, BLUE);
  }
  FastLED.show();

  //this runs in our loop forever as fast as possible which will cause it to clear and reprint to the screen
  //very fast. This may give the effect that the screen is blinking so put a little delay so we dont clear and reprint as often
  // delay(210); //wait 750ms
}

void modeHSV() {

  //set the brightness of the led strip with the value that was converted to the range of 0 to 255
  FastLED.setBrightness(VAL);
  //go through all of the LEDs and set each one to have the values calculated above
  for (int i = 0; i < NUM_LEDS; i++)  {
    leds[i] = CHSV(HUE, SAT, VAL);
  }
  FastLED.show();

  //this runs in our loop forever as fast as possible which will cause it to clear and reprint to the screen
  //very fast. This may give the effect that the screen is blinking so put a little delay so we dont clear and reprint as often
  // delay(210); //wait 750ms
}



void modeGradient() {


  //if you notice these colors changing without touching the knobs we can put in code so that we don't actually change
  //the CHSV values until you move the knob X amount
  //Start Hue    Start Sat    Start Value          End Hue     End Sat      End Val
  fill_gradient(leds, NUM_LEDS, CHSV(START_HUE, START_SAT, START_VAL), CHSV(END_HUE, END_SAT, END_VAL));

  FastLED.show();

}


void modePaparazzi() {


  EVERY_N_MILLISECONDS(37) { ///adds motion
    leds[random8(0, NUM_LEDS - 1) ] = ColorFromPalette(myPal, random8(), 255, LINEARBLEND);

  }

  fadeToBlackBy(leds, NUM_LEDS, 2);

  FastLED.show();

}


void modePolice() {

  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(DT);

  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay(DT);


  EVERY_N_MILLISECONDS(2000) {
    sendMsgToScreen("Police");
  }
}
