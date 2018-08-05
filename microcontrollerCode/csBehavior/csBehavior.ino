// csStateBehavior v0.93 -- 32 bit version (teensy)
//
// changes: added a stimGen function which is currently accessible in state 0.

#include <Wire.h>
#include <FlexiTimer2.h>
#include <Adafruit_NeoPixel.h>


//-----------------------------
// ~~~~~~~ IO Pin Defs ~~~~~~~~
//-----------------------------
//
// a) Analog Input Pins
#define lickPinA  21      // Lick/Touch Sensor A 
#define lickPinB  20      // Lick/Touch Sensor B 
#define forcePin 17
#define genA0 A21
#define genA1 A22

// b) Digital Input Pins
#define scopePin 24       // State of 2P Scope (Digital)

// c) Digital Interrupt Input Pins
#define motionPin  6
#define framePin  5

// d) Digital Output Pins
#define syncPin  25    // Trigger other things like a microscope and/or camera
#define rewardPin  26  // Trigger/signal a reward
# define neoStripPin 14


// e) UARTs (Hardware Serial Lines)
#define visualSerial Serial1 // out to a computer running psychopy
#define dashSerial Serial3 // out to a csDashboard

// f) True DACs (I define as an array object to loop later)
// on a teensy 3.2 A14 is the only DAC
#define DAC1 A21
#define DAC2 A22

// **** Make neopixel object
// if rgbw use top line, if rgb use second.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(15, neoStripPin, NEO_GRBW + NEO_KHZ800);
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(15, neoStripPin, NEO_GRB + NEO_KHZ800);

//--------------------------------
// ~~~~~~~ Variable Block ~~~~~~~~
//--------------------------------


// ****** Analog Input Inits.
uint32_t weightOffset = 0;
float weightScale = 0;
uint32_t loadCellValue=0;
uint32_t lickSensorAValue=0;
uint32_t genAnalogInput0=0;
uint32_t genAnalogInput1=0;

// a) Set DAC and ADC resolution in bits.
uint32_t adcResolution = 10;
uint32_t dacResolution = 12;

// b) Position Encoder
volatile uint32_t encoderAngle = 0;
volatile uint32_t prev_time = 0;

// c) Frame Counter
volatile uint32_t pulseCount = 0;

// d) State Machine (vStates) Interupt Timing
int sampsPerSecond = 1000;
float evalEverySample = 1.0; // number of times to poll the vStates funtion

// e) bidirectional dynamic variables
// ** These are variables you may want to have other devices/programs change
// ** To set each send over serial the header char, the value as an int and the close char '>'
// ** To get each variable ask over serial header char and the close char '<'
// ***** You can add to this, but add a single char header, the default value and increment the knownCount.
// EXAMPLE: To set the state to 2, send "a2>" over serial (no quotes). To get the current state send "a<"
// The following is the legend for each array entry:
// ____ State Machine Related
// a/0: teensyState (the teensy is considered to be the primary in the state heirarchy)
// ____ Reinforcement Related
// r/1: reward duration (if solenoid) or volume (if pump)
// t/2: time out duration (could be used to time a negative tone etc)
// ____ Visual Stim Related
// c/3: contrast of a visual stimulus (python tells teensy and teensy relays to psychopy etc.)
// o/4: orientation of a visual stimulus
// s/5: spatial frequency of a visual stimulus
// f/6: temporal frequency of a visual stimulus
// ____ NEOPIXEL Related (These do not return actual values)
// b/7: brightness of a neopixel strip
// n/8: color of neopixels (1: all white; 2: all red; 3) all green;
// --------> 4) all blue; 5) all purple (the best color) 6) random; 7) pulse rainbows for a bit
// ____ DAC Stim Train Related
// d/9: baseline duration of train a
// e/10: pulse duration of train a
// f/11: pulse amplitude of train a
// g/12: stim type of train a

// h/13: baseline duration of train b
// i/14: pulse duration of train b
// j/15: pulse amplitude of train b
// k/16: stim type of train b

char knownHeaders[] = {'a', 'r', 't', 'c', 'o', 's', 'f', 'b', 'n', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k'};
uint32_t knownValues[] = {0, 5, 8000, 0, 0, 0, 0, 999, 0, 10, 10, 4000, 0, 10, 10, 4000, 1};
int knownCount = 17;



// f) stim trains
// **** here is a map of what each array entry actually does:
// 0: pulse or baseline?
// 1: sample counter to determine how long a train has been in pulse or baseline state
// 2/3: baseline/pulse duration in interrupts (ms by default)
// 4/5: baseline/stim amplitude (as a 12-bit version of 3.3V e.g. 0V is 0 and 3.3V is 4095)
// 6: Stim type (0 for pulse train; 1 for linear ramp) todo: Ramp has a bug I think.
// 7: Write Value (determined by the pulseGen function and is what we write to the DAC each interrupt).
// 8: completed pulses
// todo: swap 7 and 8
uint32_t pulseTrain_chanA[] = {1,0,knownValues[9],knownValues[10],0,knownValues[11],knownValues[12],0,0};
uint32_t pulseTrain_chanB[] = {1,0,knownValues[13],knownValues[14],0,knownValues[15],knownValues[16],0,0};



// g) Reward Params
uint32_t rewardDelivTypeA = 0; // 0 is solenoid; 1 is syringe pump; 2 is stimulus

// h) Various State Related Things
uint32_t lastState = knownValues[0];  // We keep track of current state "knownValues[0]" and the last state.
uint32_t loopCount = 0; // Count state machine interrupts
uint32_t timeOffs;      // Mark an offset time from when we began the state machine.
uint32_t stateTimeOffs; // Mark an offset time from when we enter a new state.
uint32_t trialTime;     // Mark time for each trial (state 1 to state 1).
uint32_t stateTime;     // Mark time for each state.
uint32_t trigTime = 10; // Duration (in interrupt time) of a sync out trigger.
bool trigStuff = 0;      // Keeps track of whether we triggered things.

bool blockStateChange = 0;   // Sometimes you want teensy to be able to finish something before python can push it.
bool rewarding = 0;
bool scopeState = 1;
// ***** All states have a header we keep track of whether it fired with an array.
// !!!!!! So, if you add a state, add a header entry and increment stateCount.
int headerStates[] = {0, 0, 0, 0, 0, 0, 0};
int stateCount = 7;

// i) csDashboard
char knownDashHeaders[] = {'b', 'n'};
uint32_t knownDashValues[] = {999, 0};
int knownDashCount = 2;


void setup() {
  // todo: Setup Cyclops
  // Start the device
  strip.begin();
  strip.show();
  strip.setBrightness(2);
  setStripRed();
  // ****** Setup Analog Out
  analogWriteResolution(12);
  attachInterrupt(motionPin, rising, RISING);
  attachInterrupt(framePin, frameCount, RISING);
  pinMode(syncPin, OUTPUT);
  digitalWrite(syncPin, LOW);
  pinMode(scopePin, INPUT);
  pinMode(rewardPin, OUTPUT);
  digitalWrite(rewardPin, LOW);

  dashSerial.begin(9600);
  visualSerial.begin(9600);
  Serial.begin(19200);
  delay(10000);
  FlexiTimer2::set(1, evalEverySample / sampsPerSecond, vStates);
  FlexiTimer2::start();
}

void loop() {
  // This is interupt based so nothing here.
}

void vStates() {

  // sometimes we block state changes, so let's log the last state.
  lastState = knownValues[0];

  // we then look for any changes to variables, or calls for updates
  flagReceive(knownHeaders, knownValues);

  // Some hardware actions need to complete before a state-change.
  // So, we have a latch for state change. We write over any change with lastState
  if (blockStateChange == 1) {
    knownValues[0] = lastState;
  }


  // **************************
  // State 0: Boot/Init State
  // **************************
  if (knownValues[0] == 0) {

    // a) run a header for state 0
    if (headerStates[0] == 0) {
      visStimEnd();
      genericHeader(0);
      loopCount = 0;
      setStripRed();
      pulseCount=0;
    }

    pollColorChange();

    // b) body for state 0
    genericStateBody();
    stimTrainsNoStimState();

  }


  // ******* ******************************
  // Some things we do for all non-boot states before the state code:
  if (knownValues[0] != 0) {


    // Get a time offset from when we arrived from 0.
    // This should be the start of the trial, regardless of state we start in.
    // Also, trigger anything that needs to be in sync.
    if (loopCount == 0) {
      timeOffs = millis();
      trigStuff = 0;
      digitalWrite(syncPin, HIGH);
    }

    // This ends the trigger.
    if (loopCount >= trigTime && trigStuff == 0) {
      digitalWrite(syncPin, LOW);
      trigStuff = 1;
    }

    //******************************************
    //@@@@@@ Start Non-Boot State Definitions.
    //******************************************

    // **************************
    // State 1: Boot/Init State
    // **************************
    if (knownValues[0] == 1) {
      if (headerStates[1] == 0) {
        visStimOff();
        genericHeader(1);
        blockStateChange = 0;
      }
      genericStateBody();
      stimTrainsNoStimState();
    }

    // **************************
    // State 2: Stim State
    // **************************
    else if (knownValues[0] == 2) {
      if (headerStates[2] == 0) {
        genericHeader(2);
        visStimOn();
        blockStateChange = 0;
      }
      genericStateBody();
      if (pulseTrain_chanA[8] <2){
       stimTrainsStimState(); 
      }
      else if (pulseTrain_chanA[8] >=2){
        stimTrainsNoStimState(); 
      }
    }

    // **************************************
    // State 3: Catch-Trial (no-stim) State
    // **************************************
    else if (knownValues[0] == 3) {
      if (headerStates[3] == 0) {
        blockStateChange = 0;
        genericHeader(3);
        visStimOn();
      }
      genericStateBody();
      stimTrainsNoStimState();
    }

    // **************************************
    // State 4: Reward State
    // **************************************
    else if (knownValues[0] == 4) {
      if (headerStates[4] == 0) {
        genericHeader(4);
        visStimOff();
        rewarding = 0;
        blockStateChange = 1;
      }
      genericStateBody();
      stimTrainsNoStimState();
      
      if (rewardDelivTypeA == 0 && rewarding == 0) {
        digitalWrite(rewardPin, HIGH);
        rewarding = 1;
      }
      if (stateTime >= uint32_t(knownValues[1])) {
        digitalWrite(rewardPin, LOW);
        blockStateChange = 0;
      }
    }

    // **************************************
    // State 5: Time-Out State
    // **************************************
    else if (knownValues[0] == 5) {
      if (headerStates[5] == 0) {
        genericHeader(5);
        visStimOff();
        blockStateChange = 1;
      }

      genericStateBody();
      stimTrainsNoStimState();
      // trap the state in time-out til timeout time over.
      if (stateTime >= uint32_t(knownValues[2])) {
        blockStateChange = 0;
      }
    }

    // **************************************
    // State 6: Manual Reward State
    // **************************************
    else if (knownValues[0] == 6) {
      if (headerStates[6] == 0) {
        genericHeader(6);
        //        visStimOff();
        rewarding = 0;
        blockStateChange = 0;
      }
      genericStateBody();
      stimTrainsNoStimState();
      
      if (rewardDelivTypeA == 0 && rewarding == 0) {
        digitalWrite(rewardPin, HIGH);
        rewarding = 1;
      }
      if (stateTime >= uint32_t(knownValues[1])) {
        digitalWrite(rewardPin, LOW);
        blockStateChange = 0;
      }
    }

    // ******* Stuff we do for all non-boot states.
    trialTime = millis() - timeOffs;
    dataReport();
    loopCount++;
  }
}



void dataReport() {
  Serial.print("tData");
  Serial.print(',');
  Serial.print(loopCount);
  Serial.print(',');
  Serial.print(trialTime);
  Serial.print(',');
  Serial.print(stateTime);
  Serial.print(',');
  Serial.print(knownValues[0]); //state
  Serial.print(',');
  Serial.print(loadCellValue);  //load cell
  Serial.print(',');
  Serial.print(lickSensorAValue); // lick sensor
  Serial.print(',');
  Serial.print(encoderAngle);     //rotary encoder value
  Serial.print(',');
  Serial.print(pulseCount);
  Serial.print(',');
  Serial.print(genAnalogInput0);
  Serial.print(',');
  Serial.println(genAnalogInput1);
}


int flagReceive(char varAr[], uint32_t valAr[]) {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char endMarker = '>';
  char feedbackMarker = '<';
  char rc;
  uint32_t nVal;
  const byte numChars = 32;
  char writeChar[numChars];
  int selectedVar = 0;
  int newData = 0;

  while (Serial.available() > 0 && newData == 0) {
    rc = Serial.read();
    if (recvInProgress == false) {
      for ( int i = 0; i < knownCount; i++) {
        if (rc == varAr[i]) {
          selectedVar = i;
          recvInProgress = true;
        }
      }
    }

    else if (recvInProgress == true) {
      if (rc == endMarker ) {
        writeChar[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = 1;

        nVal = uint32_t(String(writeChar).toInt());
        valAr[selectedVar] = nVal;

      }
      else if (rc == feedbackMarker) {
        writeChar[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = 1;
        Serial.print("echo");
        Serial.print(',');
        Serial.print(varAr[selectedVar]);
        Serial.print(',');
        Serial.print(valAr[selectedVar]);
        Serial.print(',');
        Serial.println('~');
      }

      else if (rc != feedbackMarker || rc != endMarker) {
        writeChar[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
    }
  }
  return newData; // tells us if a valid variable arrived.
}

int flagReceiveDashboard(char varAr[], uint32_t valAr[]) {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char endMarker = '>';
  char feedbackMarker = '<';
  char rc;
  uint32_t nVal;
  const byte numChars = 32;
  char writeChar[numChars];
  int selectedVar = 0;
  int newData = 0;

  while (dashSerial.available() > 0 && newData == 0) {
    rc = dashSerial.read();
    if (recvInProgress == false) {
      for ( int i = 0; i < knownDashCount; i++) {
        if (rc == varAr[i]) {
          selectedVar = i;
          recvInProgress = true;
        }
      }
    }

    else if (recvInProgress == true) {
      if (rc == endMarker ) {
        writeChar[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = 1;

        nVal = uint32_t(String(writeChar).toInt());
        valAr[selectedVar] = nVal;

      }
      else if (rc == feedbackMarker) {
        writeChar[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = 1;
        dashSerial.print("echo");
        dashSerial.print(',');
        dashSerial.print(varAr[selectedVar]);
        dashSerial.print(',');
        dashSerial.print(valAr[selectedVar]);
        dashSerial.print(',');
        dashSerial.println('~');
      }

      else if (rc != feedbackMarker || rc != endMarker) {
        writeChar[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
    }
  }
  return newData; // tells us if a valid variable arrived.
}


void resetHeaders() {
  for ( int i = 0; i < stateCount; i++) {
    headerStates[i] = 0;
  }
}

void genericHeader(int stateNum) {
  stateTimeOffs = millis();
  resetHeaders();
  headerStates[stateNum] = 1;
  resetStimTrains();
}

void genericStateBody() {
  stateTime = millis() - stateTimeOffs;
  lickSensorAValue = analogRead(lickPinA);
  loadCellValue = analogRead(forcePin);
  genAnalogInput0 = analogRead(genA0);
  genAnalogInput1 = analogRead(genA1);
  scopeState = digitalRead(scopePin);
}

// ****************************************************************
// **************  Visual Stimuli *********************************
// ****************************************************************

void visStimEnd() {
  visualSerial.print('v');
  visualSerial.print(',');
  visualSerial.print(0);
  visualSerial.print(',');
  visualSerial.print(999);  // I set psychopy to stop a session when contrast = 999
  visualSerial.print(',');
  visualSerial.print(0);
  visualSerial.print(',');
  visualSerial.println(0);
}

void visStimOff() {
  visualSerial.print('v');
  visualSerial.print(',');
  visualSerial.print(0);
  visualSerial.print(',');
  visualSerial.print(0);
  visualSerial.print(',');
  visualSerial.print(0);
  visualSerial.print(',');
  visualSerial.println(0);
}

void visStimOn() {
  visualSerial.print('v');
  visualSerial.print(',');
  visualSerial.print(knownValues[4]);
  visualSerial.print(',');
  visualSerial.print(knownValues[3]);
  visualSerial.print(',');
  visualSerial.print(knownValues[5]);
  visualSerial.print(',');
  visualSerial.println(knownValues[6]);
}

// **************************************************************
// **************  Motion Interrupts  ***************************
// **************************************************************

void rising() {
  attachInterrupt(motionPin, falling, FALLING);
  prev_time = micros();
}

void falling() {
  attachInterrupt(motionPin, rising, RISING);
  encoderAngle = micros() - prev_time;
}

void frameCount() {
  pulseCount++;
}


// ****************************************************************
// **************  Pulse Train Function ***************************
// ****************************************************************
void resetStimTrains() {
  pulseTrain_chanA[0] = 0;
  pulseTrain_chanB[0] = 0;
  pulseTrain_chanA[1] = 0;
  pulseTrain_chanB[1] = 0;
  pulseTrain_chanA[7] = 0;
  pulseTrain_chanB[7] = 0;
  pulseTrain_chanA[8] = 0;
  pulseTrain_chanB[8] = 0;
}

void stimTrainsNoStimState() {
  stimGen(pulseTrain_chanA);
  stimGen(pulseTrain_chanB);
  analogWrite(DAC1, 0);
  analogWrite(DAC2, 0);
}

void stimTrainsStimState() {
  stimGen(pulseTrain_chanA);
  stimGen(pulseTrain_chanB);
  analogWrite(DAC1, pulseTrain_chanA[7]);
  analogWrite(DAC2, pulseTrain_chanB[7]);
}

void stimGen(uint32_t pulseTracker[]) {
  if (pulseTracker[6] == 0) {
    // *** handle pulse state
    if (pulseTracker[0] == 1) {
      if (pulseTracker[1] >= pulseTracker[3]) {
        pulseTracker[1] = 0; // reset counter
        pulseTracker[0] = 0; // stop pulsing
        pulseTracker[8] = pulseTracker[8]+1;
      }
      else {
        pulseTracker[7] = pulseTracker[5]; // 5 is the pulse amp; 7 is the current output.
      }
    }

    // 0 tracks in pulse and 3 is the delayWidth; 1 is the counter
    else if (pulseTracker[0] == 0) {
      if (pulseTracker[1] >= pulseTracker[2]) {
        pulseTracker[1] = 0; // reset counter
        pulseTracker[0] = 1; // start pulsing
      }
      else{
        pulseTracker[7] = pulseTracker[4]; // 4 is the baseline amp; 6 is the current output.
      }
    }
  }

  else if (pulseTracker[6] == 1) {
    uint32_t incToPeak = (pulseTracker[5] - pulseTracker[4]) / pulseTracker[3];
    // *** handle pulse state
    // 0 tracks in pulse and 2 is the pulseWidth; 1 is the counter
    // 7 is the output value
    if (pulseTracker[0] == 1) {
      if (pulseTracker[1] >= pulseTracker[3]) {
        pulseTracker[1] = 0; // reset counter
        pulseTracker[0] = 0; // stop pulsing
        pulseTracker[8] = pulseTracker[8]+1;
      }
      else{
        pulseTracker[7] = pulseTracker[7] + incToPeak; // 5 is the pulse amp; 7 is the current output.
      }
    }

    // 0 tracks in pulse and 3 is the delayWidth; 1 is the counter
    else if (pulseTracker[0] == 0) {
      if (pulseTracker[1] >= pulseTracker[2]) {
        pulseTracker[1] = 0; // reset counter
        pulseTracker[0] = 1; // start pulsing
      }
      else{
        pulseTracker[7] = pulseTracker[4]; // 4 is the baseline amp; 7 is the current output.
      }
    }
  }
  pulseTracker[1] = pulseTracker[1] + 1;
}


// ----------------------------------------------
// ---------- NEOPIXEL FUNCTIONS ----------------
// ----------------------------------------------
void setStripGreen() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  }
  strip.show();
}

void setStripRed() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
  }
  strip.show();
}

void setStripWhite() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 255));
  }
  strip.show();
}

void clearStrip() {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}

void randomColors() {

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(random(256), random(256), random(256)));
  }
  strip.show();
}


void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void pollColorChange() {
  // a) Handle brightness calls.
  if (knownValues[7] < 999) {
    if (knownValues[7] > 100) {
      knownValues[7] = 100;
    }
    strip.setBrightness(knownValues[7]);
    strip.show();
  }

  if (knownDashValues[0] <999) {
    if (knownDashValues[0] > 100) {
      knownDashValues[0] = 100;
    }
    strip.setBrightness(knownDashValues[0]);
    strip.show();
  }

  // b) Handle color changes.
  if ((knownValues[8] == 1) || (knownDashValues[1] == 1)) {
    setStripWhite();
  }
  else if ((knownValues[8] == 2) || (knownDashValues[1] == 2)) {
    setStripRed();
  }
  else if ((knownValues[8] == 3) || (knownDashValues[1] == 3)) {
    setStripGreen();
  }
  else if ((knownValues[8] == 4) || (knownDashValues[1] == 4)) {
    randomColors();
  }
  else if ((knownValues[8] == 5) || (knownDashValues[1] == 5)) {
    rainbowCycle(10);
  }

  knownValues[7] = 999;
  knownDashValues[0] = 999;
  knownValues[8] = 0;
  knownDashValues[1] = 0;
}
