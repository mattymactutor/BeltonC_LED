// need to add day and tungstein pot pwm non ws2812

// RGB Screen
// TOuch screen slide ability but also touching one of the options should highlight it and the encoder knob will also change that value

// Bounce2 is a library used for reading button presses
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

#define PIN_DT 9
#define PIN_CLK 10
#define PIN_SW 8
mmEncoder enc(PIN_DT, PIN_CLK, PIN_SW);

// 5 Modes
// RGB
// INFERNO
// RAINBOW RUN
// Use a number to keep track of what mode we are in
// but you may add many modes and we don't want to have to memorize which numbers
// go with which modes so lets define them here
#define NUMBER_MODES 5
#define MODE_RGB 0
#define MODE_HSV 1
#define MODE_GRADIENT 2
#define MODE_PAPARAZZI 3
#define MODE_POLICE 4
#define DT 100 // delay time
#define REFRESH_TIME 300

// To add in another mode make another define line with a word that represents that mode, and use the next increasing number
// Also add one to the NUMBER_MODES variable above this
// For example:
//#define MODE_SOMETHING 3

// Button Wiring
// There are two pins here, they do not have polarity it does not matter which pins you chose for the following
// Pin1 - goes to the PIN_BTNMODE in this case Arduino D2
// Pin2 - goes to ground
// The button connects the two pins together when it is pressed

#include <FastLED.h>
// for most LED strips, and you can google the specific model number to check, each LED will pull 60ma when full brightness and white (all colors)
#define MAX_LEDS 250
int NUM_LEDS = 30;
#define DATA_PIN_WS2812B 53
#define DATA_PIN_WS2811 52
#define CLOCK_PIN 11

// This is an array - think of this like a row of lockers - where the locker numbers start at 0 and we use the locker number inside
// of brackets to open that locker
// Ex:  leds[0]  this opens up locker number 0 which is the first locker
CRGB leds[MAX_LEDS];
#define MASTER -1
int BRIGHTNESS_LED_STRIP = 100;

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
// 3660

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

 Groups can be added with
 gr{groupNum, groupType, group Parameters.....}
  RGB 0 to 5
  <gr{0,0,0,5,0,0,255,0,0,255}>

  HSV 6 to 11
  <gr{1,1,6,11,25,255,127}>

  Gradient  12 to 17
  <gr{2,2,12,17,25,255,127,255,255,255}>


*/

// Taking in commands
String incomingMsg = "";
long lastLEDBlink = 0;
boolean loadLEDStatus = false;
boolean isConnectedPC = false;

//#define DEBUG

void setup()
{
  Serial.begin(115200);
  // For right now only uncomment one of these at a time
  FastLED.addLeds<WS2812B, DATA_PIN_WS2812B, GRB>(leds, MAX_LEDS);
  // FastLED.addLeds<WS2811, DATA_PIN_WS2811, RBG>(leds, MAX_LEDS);

  // FastLED.addLeds<WS2812B, DATA_PIN, CLOCK_PIN, GRB>(leds, MAX_LEDS);
  for (int i = 0; i < MAX_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 0);
    // set all LEDs as master until group data comes in
    ledGroup[i] = MASTER;
  }
  FastLED.setBrightness(255);
  FastLED.show();

  /* processSerialMessage("init");
   delay(500);
   processSerialMessage("l0{0,120,0,51,55,253}");
   delay(500);
   processSerialMessage("gr{0,0,0,3,255,0,0,0,0,252}");
   delay(500);
   processSerialMessage("gr{1,0,10,14,0,0,159,0,0,251}");
   delay(500);
   processSerialMessage("l1{200,200,200}");
   delay(500);
   processSerialMessage("l2{200,200,200,10,200,50}");
   delay(500);*/
}

void loop()
{

  monitorSerialMessage();
  /* EVERY_N_MILLISECONDS(REFRESH_TIME) {
     for (int i = 0; i < NUM_GROUPS; i++) {
       Group t = groups[i];
       if (t.type == MODE_RGB) {
         groupModeRGB(t.startLED, t.stopLED, t.r, t.g, t.b, t.B);
       } else if (t.type == MODE_HSV) {
         groupModeHSV(t.startLED, t.stopLED, t.sh, t.ss, t.sv);
       }
     }
   }*/

  if (!isConnectedPC)
  {
    if (millis() - lastLEDBlink >= 500)
    {
      lastLEDBlink = millis();
      // flip the led to the opposite
      loadLEDStatus = !loadLEDStatus;
      if (!loadLEDStatus)
      {
        leds[0] = CRGB::Black;
      }
      else
      {
        leds[0] = CRGB::Red;
      }
      FastLED.show();
    }
  }
  else
  {
    // THIS IS THE MAIN LOOP
    enc.update();
  }
}

int getNextInt(String &input)
{
  // 1,2,200,200,200}
  int retVal = -100;
  int comma = input.indexOf(",");
  if (comma == -1)
  {
    // the last number will have a } on the end
    retVal = input.substring(comma + 1, input.length() - 1).toInt();
    // we're at the end so set it to nothing
    input = "";
  }
  else
  {
    retVal = input.substring(0, comma).toInt();
    input = input.substring(comma + 1);
  }

  return retVal;
}

void monitorSerialMessage()
{
  // get incoming data
  if (Serial.available() > 0)
  {
    char c = Serial.read();
    if (c == '<')
    {
      incomingMsg = "";
    }
    else if (c == '>')
    {
      processSerialMessage(incomingMsg);
    }
    else
    {
      incomingMsg += c;
    }
  }
}

void processSerialMessage(String input)
{

  if (input == "init")
  {
    isConnectedPC = true;
    Serial.print("<ready>");
    leds[0] = CRGB::Black;
    FastLED.show();
    postRecCmd();
    return;
  }
  if (input == "lastload")
  {
    debug("lastload");
    postRecCmd();
    return;
  }

  /*
  int startLED;
  int stopLED;
  int type;
  int r, g, b, d, t, B;
  int sh, ss, sv, eh, es, ev;
  */
  // a group is not a single key-value pair so parse different
  if (input.indexOf("gr{") == 0)
  {
    // Format is
    //  gr{ GROUP_NUM, GROUP_TYPE, PARAMETERS... }
    String params = input.substring(3);
    int groupNum = getNextInt(params);
    int groupType = getNextInt(params);
    // Serial.print("Group: " + String(groupNum));
    groups[groupNum].status = getNextInt(params);
    groups[groupNum].type = groupType;
    // if the start or step led is different than before we have to give those
    // LEDs back to the master
    int startLED = getNextInt(params);
    int stopLED = getNextInt(params);
    // if the LED's changed OR status is INACTIVE you need to give the LEDs back tot he master
    if (startLED != groups[groupNum].startLED || stopLED != groups[groupNum].stopLED || groups[groupNum].status == INACTIVE)
    {
      for (int i = groups[groupNum].startLED; i <= groups[groupNum].stopLED; i++)
      {
        ledGroup[i] = MASTER;
      }
    }
    groups[groupNum].startLED = startLED;
    groups[groupNum].stopLED = stopLED;
    // set all leds in this group
    for (int i = groups[groupNum].startLED; i <= groups[groupNum].stopLED; i++)
    {
      //if this group is INACTIVE the leds should belong to the master so don't set them here
      if (groups[groupNum].status != INACTIVE)
      {
        ledGroup[i] = groupNum;
      }
    }
    refreshMaster();

    if (groupType == MODE_RGB)
    {
      // Serial.println("  t:RGB");
      groups[groupNum].r = (int)getNextInt(params);
      groups[groupNum].g = (int)getNextInt(params);
      groups[groupNum].b = (int)getNextInt(params);
      groups[groupNum].d = (int)getNextInt(params);
      groups[groupNum].t = (int)getNextInt(params);
      groups[groupNum].B = (int)getNextInt(params);
    }
    else if (groupType == MODE_HSV)
    {

      groups[groupNum].sh = (int)getNextInt(params);
      groups[groupNum].ss = (int)getNextInt(params);
      groups[groupNum].sv = (int)getNextInt(params);
    }
    else if (groupType == MODE_GRADIENT)
    {

      groups[groupNum].sh = (int)getNextInt(params);
      groups[groupNum].ss = (int)getNextInt(params);
      groups[groupNum].sv = (int)getNextInt(params);
      groups[groupNum].eh = (int)getNextInt(params);
      groups[groupNum].es = (int)getNextInt(params);
      groups[groupNum].ev = (int)getNextInt(params);
    }
    refreshGroup(groups[groupNum]);
    printGroup("Group" + String(groupNum), groups[groupNum]);
    debug("g" + String(groupNum) + "!");
    postRecCmd();
    return;
  }

  
  // check if we're loading a new mode, which changes the master
  if (input.indexOf("l0{") == 0)
  {
    String params = input.substring(3);
    // since it's zero the type should be RGB
    MASTER_GROUP.type = MODE_RGB;
    // Serial.println(params);
    MASTER_GROUP.r = getNextInt(params);
    // Serial.println(params);
    MASTER_GROUP.g = getNextInt(params);
    // Serial.println(params);
    MASTER_GROUP.b = getNextInt(params);
    // Serial.println(params);
    MASTER_GROUP.d = getNextInt(params);
    // Serial.println(params);
    MASTER_GROUP.t = getNextInt(params);
    // Serial.println(params);
    MASTER_GROUP.B = getNextInt(params);
    printGroup("Master Group", MASTER_GROUP);
    refreshMaster();
    postRecCmd();
    return;
  }
  else if (input.indexOf("l1{") == 0)
  {
    String params = input.substring(3);
    MASTER_GROUP.type = MODE_HSV;
    // Serial.println(params);
    MASTER_GROUP.sh = getNextInt(params);
    // Serial.println(params);
    MASTER_GROUP.ss = getNextInt(params);
    // Serial.println(params);
    MASTER_GROUP.sv = getNextInt(params);
    // Serial.println(params);
    refreshMaster();
    printGroup("Master", MASTER_GROUP);
    postRecCmd();
    return;
  }
  else if (input.indexOf("l2{") == 0)
  {
    String params = input.substring(3);
    MASTER_GROUP.type = MODE_GRADIENT;
    MASTER_GROUP.sh = getNextInt(params);
    MASTER_GROUP.ss = getNextInt(params);
    MASTER_GROUP.sv = getNextInt(params);
    MASTER_GROUP.eh = getNextInt(params);
    MASTER_GROUP.es = getNextInt(params);
    MASTER_GROUP.ev = getNextInt(params);
    refreshMaster();
    printGroup("Master", MASTER_GROUP);
    postRecCmd();
    return;
  }
  // find the semicolon
  int colonIdx = input.indexOf(":");

  // if no colon then stop
  if (colonIdx == -1)
  {
    return;
  }

  // split the command at the semi colon
  String cmd = input.substring(0, colonIdx);
  int val = input.substring(colonIdx + 1).toInt();
  // Serial.println("CMD: " + cmd + " VAL: " + String(val));

  // add a command to change modes

  //change num leds
  if (cmd == "nl")
  {
    NUM_LEDS = val;
    refreshEverything();
  }
  //change master red
  else if (cmd == "r")
  {
    MASTER_GROUP.r = val;
    refreshMaster();
  }
  //change master green
  else if (cmd == "g")
  {
    MASTER_GROUP.g = val;
    refreshMaster();
  }
  //change master blue
  else if (cmd == "b")
  {
    MASTER_GROUP.b = val;
    refreshMaster();
  }
  //change master hue
  else if (cmd == "h")
  {
    MASTER_GROUP.sh = val;
    refreshMaster();
  }
  //change master sat
  else if (cmd == "s")
  {
    MASTER_GROUP.ss = val;
    refreshMaster();
  }
  //change master val
  else if (cmd == "v")
  {
    MASTER_GROUP.sv = val;
    refreshMaster();
  }
  //change master daylight
  else if (cmd == "d")
  {
    MASTER_GROUP.d = val;
    refreshMaster();
  }
  //change master tungsten
  else if (cmd == "t")
  {
    MASTER_GROUP.t = val;
    refreshMaster();
  }
//change master start hue 
  else if (cmd == "sh")
  {
    MASTER_GROUP.sh = val;
    refreshMaster();
  }
  //change master start sat
  else if (cmd == "ss")
  {
    MASTER_GROUP.ss = val;
    refreshMaster();
  }
   //change master start val
  else if (cmd == "sv")
  {
    MASTER_GROUP.sv = val;
    refreshMaster();
  }
  //change master end hue
  else if (cmd == "eh")
  {
    MASTER_GROUP.eh = val;
    refreshMaster();
  }
  //change master end sat
  else if (cmd == "es")
  {
    MASTER_GROUP.es = val;
    refreshMaster();
  }
  //change master end val
  else if (cmd == "ev")
  {
    MASTER_GROUP.ev = val;
    refreshMaster();
  }
  //change master Brightness
  else if (cmd == "B")
  {
    MASTER_GROUP.B = val;
    refreshMaster();
  }
  //delete a group
  else if (cmd == "dgr"){
    int groupToDelete = val;

    //give these LEDs back to the master before deleting
    for (int i = groups[groupToDelete].startLED; i <= groups[groupToDelete].stopLED; i++){
      ledGroup[i] = MASTER;
    }
    

    //to delete in an array just move all items after this one to the left
    //      minus 1 because we need to look at the group that comes after i
    for(int i = groupToDelete; i < MAX_NUM_GROUPS - 1; i++){
      //we have a MAX array but no variable to keep track of how many groups are actually in
      //the array, use the groups status to see if it isn't used
      if (groups[i].status == NOT_USED){
        //all groups should be in order so if we make it to a group that isnt active stop copying
        break;
      }

      groups[i] = groups[i+1];
    }

    refreshEverything();

  }else if (cmd == "MB"){
    BRIGHTNESS_LED_STRIP = val;
    FastLED.setBrightness(BRIGHTNESS_LED_STRIP);
    FastLED.show();
  }
  postRecCmd();
  // if it's been more than 100ms since the last message then update the groups
  if (millis() - lastMasterMessage >= 100)
  {
  }
  lastMasterMessage = millis();
}

void postRecCmd()
{
  // send ack
  Serial.print("<!>");
}

void turnOffLEDS()
{
  for (int i = 0; i < MAX_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

void refreshMaster()
{

  // gradient is different because you have to change across all even though some might not be filled in
  // because of groups
  if (MASTER_GROUP.type == MODE_GRADIENT)
  {
    // calculate the change over the LEDs
    float deltaH = (MASTER_GROUP.eh - MASTER_GROUP.sh) / (float)NUM_LEDS;
    float deltaS = (MASTER_GROUP.es - MASTER_GROUP.ss) / (float)NUM_LEDS;
    float deltaV = (MASTER_GROUP.ev - MASTER_GROUP.sv) / (float)NUM_LEDS;
    float H = MASTER_GROUP.sh;
    float S = MASTER_GROUP.ss;
    float V = MASTER_GROUP.sv;
    for (int i = 0; i < NUM_LEDS; i++)
    {
      if (ledGroup[i] == MASTER)
      {
        leds[i] = CHSV((int)H, (int)S, (int)V);
      }
      // change even if you dont draw on the actual LED so the gradient resumes after the group
      H += deltaH;
      S += deltaS;
      V += deltaV;
    }
    FastLED.show();
    return;
  }

  // rgb and hsv are similar do them here
  float bP = MASTER_GROUP.B / 255.0;
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (ledGroup[i] == MASTER)
    {
      // check for the group types
      if (MASTER_GROUP.type == MODE_RGB)
      {
        leds[i] = CRGB(MASTER_GROUP.r * bP, MASTER_GROUP.g * bP, MASTER_GROUP.b * bP);
      }
      else if (MASTER_GROUP.type == MODE_HSV)
      {
        leds[i] = CHSV(MASTER_GROUP.sh, MASTER_GROUP.ss, MASTER_GROUP.sv);
      }
    } // end if master group
  }
  FastLED.show();
}

void printGroup(String label, Group g)
{
  debug("--" + label + "--");
  debug("r:" + String(g.r) + " g:" + String(g.g) + " b:" + String(g.b) + " d:" + String(g.d) + " t:" + String(g.t) + " B:" + String(g.B));
  debug("sh:" + String(g.sh) + " ss:" + String(g.ss) + " sv:" + String(g.sv));
  debug("eh:" + String(g.eh) + " es:" + String(g.es) + " ev:" + String(g.ev));
}

void refreshGroup(Group g)
{

  //if the group is off then show all black here
  if (g.status == OFF){
    for (int i = g.startLED; i <= g.stopLED; i++)
    {
      leds[i] = CRGB(0,0,0);
    }
    FastLED.show();
    return;
  }

  //if this group is inactive then it should belong to the master and do nothing
  if (g.status == INACTIVE){
    return;
  }

  float bP = g.B / 255.0;
  if (g.type == MODE_RGB)
  {
    for (int i = g.startLED; i <= g.stopLED; i++)
    {
      leds[i] = CRGB(g.r * bP, g.g * bP, g.b * bP);
    }
  }
  else if (g.type == MODE_HSV)
  {
    for (int i = g.startLED; i <= g.stopLED; i++)
    {
      leds[i] = CHSV(g.sh, g.ss, g.sv);
    }
  }
  else if (g.type == MODE_GRADIENT)
  {
    // the given gradient function only does the whole entire strip so
    // calc how many LEDs this will change over
    // then calculate a delta for each H,S,V
    // then run through all LEDs in this group and change to startHue + delta, increasing delta by itself every time; ( so really hue + (delta*i))
    int numLEDs = g.stopLED - g.startLED + 1;
    float deltaH = (g.eh - g.sh) / (float)numLEDs;
    float deltaS = (g.es - g.ss) / (float)numLEDs;
    float deltaV = (g.ev - g.sv) / (float)numLEDs;
    float H = g.sh;
    float S = g.ss;
    float V = g.sv;
    for (int i = g.startLED; i <= g.stopLED; i++)
    {
      leds[i] = CHSV((int)H, (int)S, (int)V);
      H += deltaH;
      S += deltaS;
      V += deltaV;
    }
  }
  FastLED.show();
}

void debug(String msg)
{
#ifdef DEBUG
  Serial.println(msg);
#endif
}

void refreshEverything(){
  // TODO show all the current leds again
    turnOffLEDS();
    refreshMaster();
    // TODO SHOW ALL GROUPS HERE
    for (int i = 0; i < MAX_NUM_GROUPS; i++)
    {
      if (groups[i].status == ACTIVE)
      {
        refreshGroup(groups[i]);
      }
    }
}