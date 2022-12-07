
#ifndef VARIABLES_H
#define VARIABLES_H

#include "FastLED_RGBW.h"
#include <FastLED.h>
#include <avr/wdt.h> //for watchdog

//-----MODES-----
#define NUMBER_MODES 5
#define MODE_RGB 0
#define MODE_HSV 1
#define MODE_GRADIENT 2
#define MODE_PAPARAZZI 3
#define MODE_POLICE 4
#define DT 100 // delay time
#define REFRESH_TIME 300

//-----LEDS-----
// for most LED strips, and you can google the specific model number to check, each LED will pull 60ma when full brightness and white (all colors)
#define MAX_LEDS 250
int NUM_LEDS = 30;
// each LED keeps track of what group it belonds to, if it belongs to the MASTER group use this number
#define MASTER -1
// global brightness of the strip, independent of the brightness in each mode
int BRIGHTNESS_LED_STRIP = 100;
// leds on strips have two types
#define LED_TYPE_RGB 0
#define LED_TYPE_RGBW 1
#define LED_TYPE_HSV 2
int LED_TYPE = LED_TYPE_RGB;
// Set the global led array that will control the current strip
CRGB *leds;
// This variable may need to point to two different arrays
// based on whether or not we are in WHITE mode
CRGBW ledsRGBW[MAX_LEDS];
CRGB ledsRGB[MAX_LEDS];

//------LED STRIP TYPES-----
// THESE LABELS ARE ALSO USED IN FAST LED LIBRARY
//so put a prefix on them
#define STRIP_WS2812B 0
#define STRIP_APA102 1
#define STRIP_NEOPIXEL_SKINNY 2
#define STRIP_NEOPIXEL 3
int STRIP_TYPE = STRIP_WS2812B;

//---------------GROUPs----------------
#define NOT_USED -1
#define ACTIVE 0
#define INACTIVE 1
#define OFF 2
struct Group
{
public:
  int startLED;
  int stopLED;
  int type = -1;
  int r, g, b, d, t, B;
  int sh, ss, sv, eh, es, ev;
  int status = NOT_USED;
};

#define MAX_NUM_GROUPS 15
Group groups[MAX_NUM_GROUPS];
long lastMasterMessage = 0;

Group MASTER_GROUP;
int ledGroup[MAX_LEDS];

#endif

//---FUNCTIONS WITH DEFAULT VALUES--
//through testing i found that arduino wants prototypes if you use a default variable
void setLED(int type, int i, int arg1, int arg2, int arg3, int white=0);


//----RESET---
//the arduino can reset itself if you call a function that is assigned
//to address 0
void resetFunc(){
  //enable that we have to talk back in 15ms then get stuck in a loop
  wdt_enable(WDTO_15MS);
  for(;;){}

}