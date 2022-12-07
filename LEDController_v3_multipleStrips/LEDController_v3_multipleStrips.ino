#include <EEPROM.h>
#include "variables.h"
#include "mmEncoder.h"
#include <FastLED.h>


/*
TODO


test with the WS2811
test the CRGBW but with HSV. maybe because HSV doesnt need white you can just still use CHSV


*/


#define PIN_DT 9
#define PIN_CLK 10
#define PIN_SW 8
mmEncoder enc(PIN_DT, PIN_CLK, PIN_SW);

//----SET PINS----
#define DATA_PIN 53
#define CLOCK_PIN 52


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

   To change striptype reference the defines int he variables folder
   st:*strip type number*

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
  //FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, MAX_LEDS);
  // FastLED.addLeds<WS2811, DATA_PIN_WS2811, RBG>(leds, MAX_LEDS);

  int stripType = EEPROM.read(0);
  setStripType(stripType);

  //FOR TESTING
  //setStripType(STRIP_NEOPIXEL_SKINNY);

  //set all leds to belong to the master by default
  for (int i = 0; i < MAX_LEDS; i++)
  {   
    // set all LEDs as master until group data comes in
    ledGroup[i] = MASTER;
  }

  turnOffLEDS();
/*
  processSerialMessage("init");
  delay(500);
  processSerialMessage("l0{0,0,120,51,55,253}");
   delay(500);

  processSerialMessage("gr{0,0,0,0,3,255,0,0,0,0,252}");
  delay(500);
  processSerialMessage("gr{1,0,10,14,0,0,159,0,0,251}");
  delay(500);
  processSerialMessage("l1{200,200,200}");
  delay(500);
  processSerialMessage("l2{200,200,200,10,200,50}");
  delay(500);*/

   //init watchdog timer so we can software reset
   //delay(2000);
   //wdt_enable(WDT0_4S);
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
        //leds[0] = CRGB::Black;
        setLED(LED_TYPE_RGB,0,0,0,0);
      }
      else
      {
        //leds[0] = CRGB::Red;
        setLED(LED_TYPE_RGB,0,255,0,0);
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

  //shouldnt need this anymore - arduino resets itself when the serial gets closed and reopened
  /*if (input == "RESET"){
    debug("Reset...");
    delay(1000);
    resetFunc();
    return;
  }*/
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
    //  gr{ GROUP_NUM, GROUP_TYPE, GROUP_STATUS,PARAMETERS... }
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
      // if this group is INACTIVE the leds should belong to the master so don't set them here
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
    //printGroup("Master Group", MASTER_GROUP);
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

  // change num leds
  if (cmd == "nl")
  {
    NUM_LEDS = val;
    refreshEverything();
  }
  // change master red
  else if (cmd == "r")
  {
    MASTER_GROUP.r = val;
    refreshMaster();
  }
  // change master green
  else if (cmd == "g")
  {
    MASTER_GROUP.g = val;
    refreshMaster();
  }
  // change master blue
  else if (cmd == "b")
  {
    MASTER_GROUP.b = val;
    refreshMaster();
  }
  // change master hue
  else if (cmd == "h")
  {
    MASTER_GROUP.sh = val;
    refreshMaster();
  }
  // change master sat
  else if (cmd == "s")
  {
    MASTER_GROUP.ss = val;
    refreshMaster();
  }
  // change master val
  else if (cmd == "v")
  {
    MASTER_GROUP.sv = val;
    refreshMaster();
  }
  // change master daylight
  else if (cmd == "d")
  {
    MASTER_GROUP.d = val;
    refreshMaster();
  }
  // change master tungsten
  else if (cmd == "t")
  {
    MASTER_GROUP.t = val;
    refreshMaster();
  }
  // change master start hue
  else if (cmd == "sh")
  {
    MASTER_GROUP.sh = val;
    refreshMaster();
  }
  // change master start sat
  else if (cmd == "ss")
  {
    MASTER_GROUP.ss = val;
    refreshMaster();
  }
  // change master start val
  else if (cmd == "sv")
  {
    MASTER_GROUP.sv = val;
    refreshMaster();
  }
  // change master end hue
  else if (cmd == "eh")
  {
    MASTER_GROUP.eh = val;
    refreshMaster();
  }
  // change master end sat
  else if (cmd == "es")
  {
    MASTER_GROUP.es = val;
    refreshMaster();
  }
  // change master end val
  else if (cmd == "ev")
  {
    MASTER_GROUP.ev = val;
    refreshMaster();
  }
  // change master Brightness
  else if (cmd == "B")
  {
    MASTER_GROUP.B = val;
    refreshMaster();
  }
  // delete a group
  else if (cmd == "dgr")
  {
    int groupToDelete = val;

    // give these LEDs back to the master before deleting
    for (int i = groups[groupToDelete].startLED; i <= groups[groupToDelete].stopLED; i++)
    {
      ledGroup[i] = MASTER;
    }

    // to delete in an array just move all items after this one to the left
    //       minus 1 because we need to look at the group that comes after i
    for (int i = groupToDelete; i < MAX_NUM_GROUPS - 1; i++)
    {
      // we have a MAX array but no variable to keep track of how many groups are actually in
      // the array, use the groups status to see if it isn't used
      if (groups[i].status == NOT_USED)
      {
        // all groups should be in order so if we make it to a group that isnt active stop copying
        break;
      }

      groups[i] = groups[i + 1];
    }

    refreshEverything();
  }
  else if (cmd == "MB")
  {
    BRIGHTNESS_LED_STRIP = val;
    FastLED.setBrightness(BRIGHTNESS_LED_STRIP);
    FastLED.show();
  }
  // Change the LED Strip type
  else if (cmd == "st")
  {
    debug("Changing strip type to #" + String(val));
    STRIP_TYPE = val;
    setStripType(STRIP_TYPE);
    //save this to eeprom
    EEPROM.write(0,STRIP_TYPE);
    //the GUI closes and reopens the serial communication which restarts the arduino
    //and it should load the new strip type from EEPROM
  } 
  postRecCmd();
  // if it's been more than 100ms since the last message then update the groups
  /*if (millis() - lastMasterMessage >= 100)
  {
  }
  lastMasterMessage = millis();*/
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
    if (STRIP_TYPE == STRIP_NEOPIXEL_SKINNY){
      ledsRGBW[i] = CRGBW(0,0,0,0);
    } else {
     ledsRGB[i] = CRGB(0, 0, 0);
    }
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
        //leds[i] = CHSV((int)H, (int)S, (int)V);
        setLED(LED_TYPE_HSV,i,(int)H, (int)S, (int)V);
        
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
       //leds[i] = CRGB(MASTER_GROUP.r * bP, MASTER_GROUP.g * bP, MASTER_GROUP.b * bP);
        setLED(LED_TYPE_RGB, i, MASTER_GROUP.r * bP, MASTER_GROUP.g * bP, MASTER_GROUP.b * bP);
      }
      else if (MASTER_GROUP.type == MODE_HSV)
      {
       // leds[i] = CHSV(MASTER_GROUP.sh, MASTER_GROUP.ss, MASTER_GROUP.sv);
        setLED(LED_TYPE_HSV,i,MASTER_GROUP.sh, MASTER_GROUP.ss, MASTER_GROUP.sv);
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

  // if the group is off then show all black here
  if (g.status == OFF)
  {
    for (int i = g.startLED; i <= g.stopLED; i++)
    {
      //leds[i] = CRGB(0, 0, 0);
      setLED(LED_TYPE_RGB, i, 0,0,0);
    }
    FastLED.show();
    return;
  }

  // if this group is inactive then it should belong to the master and do nothing
  if (g.status == INACTIVE)
  {
    return;
  }

  float bP = g.B / 255.0;
  if (g.type == MODE_RGB)
  {
    for (int i = g.startLED; i <= g.stopLED; i++)
    {
      //leds[i] = CRGB(g.r * bP, g.g * bP, g.b * bP);
      setLED(LED_TYPE_RGB,i,g.r * bP, g.g * bP, g.b * bP);
      Serial.println("Changed led in group: " + String(i));
    }
  }
  else if (g.type == MODE_HSV)
  {
    for (int i = g.startLED; i <= g.stopLED; i++)
    {
      //leds[i] = CHSV(g.sh, g.ss, g.sv);
      setLED(LED_TYPE_HSV, i, g.sh, g.ss, g.sv);
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
      //leds[i] = CHSV((int)H, (int)S, (int)V);
      setLED(LED_TYPE_HSV, i, (int)H, (int)S, (int)V);
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

//this function sets the led based on if the led type is RGB or CHSV
//the RGB vs RGBW will be decided by the strip type
void setLED(int type, int i, int arg1, int arg2, int arg3, int white=0)
{
  //change rgb versions first
  if (type == LED_TYPE_RGB){
    //check strip type
    if (STRIP_TYPE == STRIP_NEOPIXEL_SKINNY){
      ledsRGBW[i] = CRGBW(arg1,arg2,arg3,white);
    }
    //right now thats the only one with white so use else 
    else {
      ledsRGB[i] = CRGB(arg1,arg2,arg3);
      //Serial.println("Set led " + String(i) + " to " + String(arg1) + " " + String(arg2) + " " + String(arg3));
    }
  }
  //check HSV versions
  else if (type == LED_TYPE_HSV){
 //check strip type
    if (STRIP_TYPE == STRIP_NEOPIXEL_SKINNY){
      ledsRGBW[i] = CHSV(arg1,arg2,arg3);
    }
    //right now thats the only one with white so use else 
    else {
      ledsRGB[i] = CHSV(arg1,arg2,arg3);
    }
  }

}


void setStripType(int t)
{
  STRIP_TYPE = t;
  // change to ws2812b
  if (t == STRIP_WS2812B)
  {
    setLEDArray(LED_TYPE_RGB);
    // WS2812B Strip
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, MAX_LEDS);    
    Serial.println("Changed to WS2812B...");
  }
  // change to APA102
  else if (t == STRIP_APA102)
  {
    setLEDArray(LED_TYPE_RGB);
    // Adafruit Dotstar - RGB (no white) right now I have a strip version of this adafruit part 2329, and a matrix version
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, MAX_LEDS);
    Serial.println("Changed to APA102...");
  }
  // change to neo pixel skinny
  else if (t == STRIP_NEOPIXEL_SKINNY)
  {
    setLEDArray(LED_TYPE_RGBW);
    FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, getRGBWsize(MAX_LEDS));
    Serial.println("Changed to Neopixel Skinny...");
  }
  FastLED.setBrightness(255);
  FastLED.show();
}

void setLEDArray(int type)
{
  LED_TYPE = type;
  if (type == LED_TYPE_RGBW)
  {
    leds = (CRGB *)&ledsRGBW[0];
  }
  else if (type == LED_TYPE_RGB)
  {
    leds = &ledsRGB[0];
  }
}

void refreshEverything()
{

  turnOffLEDS();
  refreshMaster();
  //show all groups after master so they override it
  for (int i = 0; i < MAX_NUM_GROUPS; i++)
  {
    if (groups[i].status == ACTIVE)
    {
      refreshGroup(groups[i]);
    }
  }
}
