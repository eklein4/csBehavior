// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ekMicroController code, modified version of 
// csStateBehavior v0.99 -- 32 bit version (teensy)
// by Chris Deister --> cdeister@brown.edu
// 
// Interrupt timed state machine for running behavior tasks and delivering stimuli etc. with a Teensy 3.5/6 board.
// Intended to be used with a python program (csBehavior.py) that enables:
// a) on-demand insturment control
// b) data saving
// c) state-flow logic
//
// On a Teensy 3.6, each interrupt takes ~50-100 us depending on how many variables are processed.
//
// Changes from Deister version:
//  Removal of unused I/O 
//  Removal of unsused state processes
//  Changed process of pulse-train counting to make it independent of state changes
//  Added next stim time and stim num to process of stim gen to allow for push of stim train timing within 
//    a state and in advance of the trigger time. This allows for independent control of all stimuli within a unchanging state.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// ****************************************
// ***** Initialize All The Things ********
// ****************************************
//

#include <math.h>

// Builtin Libraries
#include <Wire.h>
#include <FlexiTimer2.h>
#include <SPI.h>

// Other people's libraries
// NO #include <Adafruit_NeoPixel.h>
#include "HX711.h"

#include <MCP4922.h>


//-----------------------------
// ~~~~~~~ IO Pin Defs ~~~~~~~~
//-----------------------------
//
// a) Analog Input Pins
// TODO get rid of one of these
#define lickPinA  23      // Lick/Touch Sensor A 
#define lickPinB  22      // Lick/Touch Sensor B 

#define genA0 A0
#define genA1 A1
#define genA2 A2
#define genA3 A3
#define analogMotion A16

// b) Digital Input Pins
#define scaleData  29
#define scaleClock  28

// TODO do we need:
// NO? #define dacGate1 38
// NO? #define dacGate2 39

// c) Digital Interrupt Input Pins
// NO #define motionPin 36
#define framePin  5
#define yGalvo  6

// d) Digital Output Pins
#define rewardPin 24  // Trigger/signal a reward
#define syncPin 25    // Trigger other things like a microscope and/or camera

// TODO decide if useful
#define sessionOver  26
#define rewardMirror 30

// TODO decide if we want TTL triggering #define syncMirror 27 removed all traces of LED swtich and other removed
// NO #define ledSwitch 31

// NO #define neoStripPin 2
// NO #define pmtBlank  34


// session header
bool startSession = 0;  // TODO this is only used for the end session, if we don't need -> delete

// NO uint32_t vStim_xPos = 800;
// NO uint32_t vStim_yPos = 800;

// TODO delete these if not needed

elapsedMillis trialTime;
elapsedMillis stateTime;
elapsedMicros headerTime;
elapsedMicros loopTime;

// TODO this is hard coded into the Stim Gen, change back
uint32_t dacNum = 5;


// NO e) UARTs (Hardware Serial Lines)
// NO #define visualSerial Serial1 // out to a computer running psychopy
// NO #define dashSerial Serial3 // out to a csDashboard

// f) True 12-bit DACs (I define as an array object to loop later)
// on a teensy 3.2 A14 is the only DAC
// MCP DACs (3&4) can be powered by 5V, and will give 5V out.
// Teensy DACs are 3.3V, but see documentation for simple opamp wiring to get 5V peak.
#define DAC1 A21
#define DAC2 A22

// ~~~ MCP DACs

// TODO Change LDAC pin (see readme) if appropriate
MCP4922 mDAC1(11, 13, 33, 37);
MCP4922 mDAC2(11, 13, 34, 32);

// NO  **** Make neopixel object
// NO if rgbw use top line, if rgb use second.
// NO Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, neoStripPin, NEO_GRBW + NEO_KHZ800);
// NO Adafruit_NeoPixel strip = Adafruit_NeoPixel(30, neoStripPin, NEO_GRB + NEO_KHZ800);
// NO uint32_t maxBrightness = 255;

//--------------------------------
// ~~~~~~~ Variable Block ~~~~~~~~
//--------------------------------

// make a loadcell object and set variables
#define calibration_factor 440000
#define zero_factor 8421804

HX711 scale(scaleData, scaleClock);
uint32_t weightOffset = 0;
float scaleVal = 0;

// NO bool sDacGate1 = 0;
// NO bool sDacGate2 = 0;

uint32_t lickSensorAValue = 0;
uint32_t genAnalogInput0 = 0;
uint32_t genAnalogInput1 = 0;
uint32_t genAnalogInput2 = 0;
uint32_t genAnalogInput3 = 0;
uint32_t analogAngle = 0;

// TODO These might be for something?
// uint32_t microTimer;
// uint32_t microTimer2;

// a) Set DAC and ADC resolution in bits.
uint32_t adcResolution = 12;
uint32_t dacResolution = 12;

// b) Position Encoder
volatile uint32_t encoderAngle = 0;
volatile uint32_t prev_time = 0;

// c) Frame Counter
volatile uint32_t pulseCount = 0;

// d) Flyback Signal
volatile uint32_t flybackVal = 0;

volatile uint32_t lineTime = 0;
volatile uint32_t curLine = 0;
volatile uint32_t lastLine = 0;

// e) State Machine (vStates) Interupt Timing
int sampsPerSecond = 1000;
float evalEverySample = 1.0; // number of times to poll the vStates funtion

// e) State Machine (vStates) Interupt Timing
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
// g/2: time out duration (could be used to time a negative tone etc)
// NO  ____ Visual Stim Related
// NO c/3: contrast of a visual stimulus (python tells teensy and teensy relays to psychopy etc.)
// NO o/4: orientation of a visual stimulus
// NO s/5: spatial frequency of a visual stimulus
// NO f/6: temporal frequency of a visual stimulus
// ____ NEOPIXEL Related (These do not return actual values)
// NO b/7: brightness of a neopixel strip
// NO n/8: color of neopixels (1: all white; 2: all red; 3) all green;
// --------> 4) all blue; 5) all purple (the best color) 6) random; 7) pulse rainbows for a bit
// ____ DAC Stim Train Related
// d/9: interpulse duration (us) of train X end the call with the DAC# so d1001 will set the IPI of DAC1 to 100.
// p/10: pulse duration (us) of train X end the call with the DAC# so p101 will set the pulse dur of DAC1 to 10.
// v/11: pulse amplitude of train X end the call with the DAC# so v40001 will set the pulse dur of DAC1 to 4000.
// t/12: stim type of train X end the call with the DAC# 0 is pulse train, 1 is ramp, 2 is asymmetric cosine stim; so t11> will set DAC1 to ramp.
// m/13: max pulses for a stimulus for channel X. m381> will set the number of pulses on DAC1 to 38.
//New Pulse Tracker Data
// n/ TODO: Next stim time - needs to be pushed with stims if not state dependent

// ____ Misc.
// l/14: current value on loadCell
// TODO do we need?  h/15: toggle a pin
// q/16: Flyback stim dur (in microseconds)
// NO e/17: led switch
// NO x/18: visStim xPos (times 10)
// NO y/19: visStim yPos
// NO z/20: visStim size (times 10)

char knownHeaders[] =    {'a', 'r', 'g', 'l', 'h', 'q', 'd', 'p', 'v', 't', 'm', 'n'};
 int32_t knownValues[] = { 0, 5, 8000, 0, 0, 100, 90, 10, 0, 0, 0, -1};
 int knownCount = 12;

//char knownHeaders[] =    {'a', 'r', 'g', 'c', 'o', 's', 'f', 'b', 'n', 'd', 'p', 'v', 't', 'm', 'l', 'h', 'q', 'e', 'x', 'y', 'z'};
// int32_t knownValues[] = { 0,  5, 8000, 0,  0,  4,  2, 1, 0, 90, 10, 0, 0, 0, 0, 0, 100, 1, 2, 0, 10};
//int knownCount = 21;



// f) stim trains
// **** here is a map of what each array entry actually does:
// 0: baseline (0) or pulse (1) states
// 1: stop bit; if you set a max pulse num, it will count down and flip this. If not 0, pulsing will stop.
// 2/3: baseline/pulse duration in interrupts (ms by default)
// 4/5: baseline/stim amplitude (as a 12-bit version of 3.3V e.g. 0V is 0 and 3.3V is 4095)
// 6: Stim type (0 for pulse train; 1 for linear ramp) todo: Ramp has a bug I think.
// 7: Write Value (determined by the pulseGen function and is what we write to the DAC each interrupt).
// 8: number of pulses to complete
// 9: up or down bit: used for complex kernels like asymmetric cosine
// todo: swap 7 and 8

// TODO some of these are a lie, update with real behaviors, add two more categories for new counting method
// added next stim time and stim count (11/12)
// changed data type to int 32 no need for it to be higher, negative is useful
int32_t pulseTrainVars[][12] =
{ {1, 0, knownValues[9], knownValues[10], 0, knownValues[11], knownValues[12], 0, knownValues[13], 0, -1, 0},
  {1, 0, knownValues[9], knownValues[10], 0, knownValues[11], knownValues[12], 0, knownValues[13], 0, -1, 0},
  {1, 0, knownValues[9], knownValues[10], 0, knownValues[11], knownValues[12], 0, knownValues[13], 0, -1, 0},
  {1, 0, knownValues[9], knownValues[10], 0, knownValues[11], knownValues[12], 0, knownValues[13], 0, -1, 0},
  {1, 0, knownValues[9], knownValues[10], 0, knownValues[11], knownValues[12], 0, knownValues[13], 0, -1, 0}
};

// stim trains are timed with elapsedMicros timers, which we store in an array to loop with channels.
elapsedMillis trainTimer[5];


uint32_t analogOutVals[] = {pulseTrainVars[0][7], pulseTrainVars[1][7], pulseTrainVars[2][7], pulseTrainVars[3][7], pulseTrainVars[4][7]};

// g) Reward Params
// NO just use solonoid uint32_t rewardDelivTypeA = 0; // 0 is solenoid; 1 is syringe pump; 2 is stimulus

// h) Various State Related Things
uint32_t lastState = knownValues[0];  // We keep track of current state "knownValues[0]" and the last state.

// TODO why do we care about last state?

uint32_t loopCount = 0; // Count state machine interrupts
uint32_t trigTime = 10; // Duration (in interrupt time) of a sync out trigger.

// NO uint32_t lastBrightness = 10;

// NOT NEEDED bool trigStuff = 0;      // Keeps track of whether we triggered things.

// TODO might not need
bool blockStateChange = 0;   // Sometimes you want teensy to be able to finish something before python can push it.
bool rewarding = 0;
//NO bool scopeState = 1;

// ***** All states have a header we keep track of whether it fired with an array.
// !!!!!! So, if you add a state, add a header entry and increment stateCount.

//TODO set states to something else
int headerStates[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int stateCount = 9;

/* NO // i) csDashboard
//char knownDashHeaders[] = {'b', 'n','v'};
uint32_t knownDashValues[] = {10, 0, 10};
//int knownDashCount = 3;
*/

void setup() {
 // Serial Lines
 // dashSerial.begin(115200);
 // visualSerial.begin(115200);
  // Serial.begin(19200);
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect 
  }
  // Start MCP DACs

  SPI.begin();
  pinMode(33, OUTPUT);
  pinMode(34, OUTPUT);
 /* // neopixels
  strip.begin();
  strip.show();
  strip.setBrightness(100);
  setStrip(2);
  */
  
  // loadcell
  scale.set_scale(calibration_factor);
  scale.set_offset(zero_factor);
  scale.tare();

  // Analog In/Out
  analogReadResolution(12);
  analogWriteResolution(12);

  // Interrupts
  attachInterrupt(motionPin, rising, RISING);
  attachInterrupt(framePin, frameCount, RISING);
  attachInterrupt(yGalvo, flybackStim_On, FALLING);

  // DIO Pin States
  pinMode(syncPin, OUTPUT);
  digitalWrite(syncPin, LOW);
  pinMode(sessionOver, OUTPUT);
  digitalWrite(sessionOver, LOW);
  //  pinMode(scaleData, OUTPUT);


  // NO pinMode(dacGate1, OUTPUT);
  // NO pinMode(dacGate2, OUTPUT);

    pinMode(rewardPin, OUTPUT);
  digitalWrite(rewardPin, LOW);
  // pinMode(pmtBlank, OUTPUT);
  // digitalWrite(pmtBlank, LOW);
  // pinMode(ledSwitch, OUTPUT);
  // digitalWrite(ledSwitch, LOW);

  pinMode(syncMirror, INPUT);
  pinMode(rewardMirror, INPUT);

  // Start Program Timer
  FlexiTimer2::set(1, evalEverySample / sampsPerSecond, vStates);
  FlexiTimer2::start();
}

void loop() {
  // This is interupt based so nothing here.
}


void vStates() {
  // ***************************************************************************************
  // **** Loop Timing/Serial Processing:
  // Every loop resets the timer, then looks for serial variable changes.
  loopTime = 0;
  lastState = knownValues[0];

  // we then look for any changes to variables, or calls for updates
  int curSerVar = flagReceive(knownHeaders, knownValues);

  // TODO these need to change based on new KnwnV's
  // changed from 9-13 added 11
  if ((curSerVar == 6) || (curSerVar == 7) || (curSerVar == 8) || (curSerVar == 9) || (curSerVar == 10) || (curSerVar == 11)) {
    setPulseTrainVars(curSerVar, knownValues[curSerVar]);
  }

  //TODO evaluate this and  determine need for this function and last state

  // Some hardware actions need to complete before a state-change.
  // So, we have a latch for state change. We write over any change with lastState
  if (blockStateChange == 1) {
    knownValues[0] = lastState;
  }

  // ***************************************************************************************

  // **************************
  // State 0: Boot/Init State
  // **************************
  if (knownValues[0] == 0) {

    // a) run a header for state 0
    if (headerStates[0] == 0) {
      // visStim(0);
      genericHeader(0);
      loopCount = 0;
      // setStrip(3); // red
      pulseCount = 0;
      // reset session header
      if (startSession == 1) {
        digitalWrite(sessionOver, HIGH);
        digitalWrite(13, HIGH);
        delay(10);
        digitalWrite(sessionOver, LOW);
        digitalWrite(13, LOW);
      }
      startSession = 0;
    }

    // pollColorChange();
    pollToggle();
    // pollRelays(); // Let other users use the trigger lines
    // b) body for state 0
    genericStateBody();


  }

  // **************************
  // State != 0: (in task)
  // **************************
  if (knownValues[0] != 0) {

    if (loopCount == 0) {
     // why? trigStuff = 0;
      digitalWrite(syncPin, HIGH);
    }

    // This ends the trigger.
    // why? if (loopCount >= trigTime && trigStuff == 0) {
    if (loopCount >= trigTime) {
      digitalWrite(syncPin, LOW);
      // why? trigStuff = 1;
    }

    //******************************************
    //@@@@@@ Start Non-Boot State Definitions.
    //******************************************


    // **************************
    // State 1: Boot/Init State
    // **************************
    if (knownValues[0] == 1) {
      // run this stuff once per session
      if (startSession == 0) {
        startSession = 1;
        trialTime = 0;
      }

      if (headerStates[1] == 0) {
      //  visStim(0);
        genericHeader(1);
        blockStateChange = 0;
      }
      genericStateBody();
    }

    // **************************
    // State 2: Stim State
    // **************************
    /*
     * This Needs to be removed
     
    else if (knownValues[0] == 2) {
      if (headerStates[2] == 0) {
        genericHeader(2);
        // visStim(2);
        blockStateChange = 0;
      }
      analogOutVals[0] = 0;
      analogOutVals[1] = 0;
      analogOutVals[2] = 0;
      analogOutVals[3] = 0;
      genericStateBody();
    }
    */
    // **************************************
    // State 3: Catch-Trial (no-stim) State
    // **************************************
    else if (knownValues[0] == 3) {
      if (headerStates[3] == 0) {
        blockStateChange = 0;
        genericHeader(3);
       // visStim(0);
      }

      stimGen(pulseTrainVars);
      setAnalogOutValues(analogOutVals, pulseTrainVars);
      genericStateBody();
    }

    // **************************************
    // State 4: Reward State
    // **************************************
    else if (knownValues[0] == 4) {
      if (headerStates[4] == 0) {
        blockStateChange = 0;
        genericHeader(4);
        // visStim(0);
        rewarding = 0;
      }
      genericStateBody();
// TODO Superfluous conditional confirm how 'rewarding' is cleared type is gone 
      if (rewardDelivTypeA == 0 && rewarding == 0) {
        digitalWrite(rewardPin, HIGH);
        rewarding = 1;
      }
      if (stateTime >= uint32_t(knownValues[1])) {
        digitalWrite(rewardPin, LOW);
      }
    }

    /*
     * Remove
     // **************************************
    // State 5: Time-Out State
    // **************************************
    else if (knownValues[0] == 5) {
      if (headerStates[5] == 0) {
        blockStateChange = 0;
        genericHeader(5);
        //visStim(0);
      }
      genericStateBody();
    }
    */
    // **************************************
    // State 6: Manual Reward State
    // **************************************
    else if (knownValues[0] == 6) {
      if (headerStates[6] == 0) {
        genericHeader(6);
        rewarding = 0;
        blockStateChange = 0;
      }
      genericStateBody();
      
// TODO Superfluous conditional confirm how 'rewarding' is cleared type is gone

      if (rewardDelivTypeA == 0 && rewarding == 0) {
        digitalWrite(rewardPin, HIGH);
        rewarding = 1;
      }
      if (stateTime >= uint32_t(knownValues[1])) {
        digitalWrite(rewardPin, LOW);
        blockStateChange = 0;
      }
    }

    // ****************************************
    // State 7: Single Pulse Train Trial State
    // ****************************************
    else if (knownValues[0] == 7) {
      if (headerStates[7] == 0) {
        genericHeader(7);
       // visStim(0);
        blockStateChange = 0;
      }
      stimGen(pulseTrainVars);
      setAnalogOutValues(analogOutVals, pulseTrainVars);
      genericStateBody();
    }

    // ****************************************
    // State 8: Flyback Pulse State
    // ****************************************
    else if (knownValues[0] == 8) {
      if (headerStates[8] == 0) {
        genericHeader(8);
      //  visStim(0);
        blockStateChange = 0;
      }
      stimGen(pulseTrainVars);
      analogOutVals[0] = 0;
      analogOutVals[1] = 0;
      analogOutVals[2] = 0;
      analogOutVals[3] = 0;
      genericStateBody();
    }

    // ******* Stuff we do for all non-boot states at the end.
    dataReport();
    loopCount++;
  }
}


// ******************************************************
// FUNCTIONS IN USE
// ******************************************************

/* Flag recieve changed to return specific non-variable integer (-1) if called w/o a serial buffer or if a paremeter 
 *  request is recieved. Returns header number only when paremeter change is rececieved. Also the max receieved int size limit
 *  was changed to reflect 12 bit (10 digits plus null) and the exceed case was changed to an error message escape with
 *  a buffer flush.
 *  NOTE: This WILL bug on slow serial (i.e. genuine Arduino / other slow TTL serial 9600 baud definitey bugs) unless
 *  blocking term is added after serial.read (e.g., while(rc != something))
 */

int flagReceive(char varAr[], int32_t valAr[]) {
  static byte ndx = 0;          // finds place being read
  char endMarker = '>';         // Designates recieve message
  char feedbackMarker = '<';    // Designates send/requested message
  char rc;                      // recieved variable header character
  uint32_t nVal;                // Variable value (I think)
  const byte numChars = 11;     // Size of string to store each digit of int 16 bit 10 digits + 1 null
  char writeChar[numChars];     // Make the string of this size
  int selectedVar = -1;          // Returned index of kV header of recieved variable for Pulse update TODO: pulses be updated here?
  static boolean recvInProgress = false;  // Bit indicating in recieve PERSISTS THROUGH CALLS TO THIS FUNCTION !
  bool newData = 0;               // Bit breaks loop when true, fliped when variable recieve is finished 
  int32_t negScale = 1;         // used to convert the ABS value of the variable recieved to negative when '-' read out

  while (Serial.available() > 0 && newData == 0) {    // set a loop to run if there is a bit in the buffer until the message is read
    rc = Serial.read();                               // read the (next) bit and store it to RC
    //delay(10);                                // this is for slow serial - 'while' statement is better but EITHER BLOCKS
    if (recvInProgress == false) {                // If this is a new message (variable; not partially read)
      for ( int i = 0; i < knownCount; i++) {   // Loop through the number of knownValues variables
        if (rc == varAr[i]) {                   // if the read byte == the looped to (any) kV Header char
          selectedVar = i;                      // set the return value to variable's the index of the kV Header 
          recvInProgress = true;                //  indicate the message is being read by flipping rcvInProg
          break;                                // added for efficency and prevent redunfance bugs 
        }
      }
    }

    else if (recvInProgress == true) {        // if the message is in the process of being read
      
      if (rc == endMarker ) {                 // if the bit is a revieve message stop 
        writeChar[ndx] = '\0';                // terminate the string by writing a non-integer to writeChar at present ndx
        recvInProgress = false;               // indicate finished message by flipping rcvInProg
        ndx = 0;                              // reset the index 
        newData = 1;                          // indecate that the message is read (and ready)
        nVal = int32_t(String(writeChar).toInt());  //convert the Char to string and then to integer and call it nVal
        valAr[selectedVar] = nVal * negScale; // process ABS value with +/- read and set the (global) array value equal to it
        return selectedVar;                   // message is done, return the variable's array index and break
      }

      else if (rc == feedbackMarker) {        // if the bit is a send message stop
        writeChar[ndx] = '\0';                // terminate the string by writing a non-integer to writeChar at present ndx
        recvInProgress = false;               // indicate finished message by flipping rcvInProg 
        ndx = 0;                              // reset the index 
        newData = 1;                          // indecate that the message is read (and ready)
                                  //don't write anything
        Serial.print("echo");                 // send a line through serial with the requested value
        Serial.print(',');
        Serial.print(varAr[selectedVar]);
        Serial.print(',');
        Serial.print(valAr[selectedVar]);
        Serial.print(',');
        Serial.println('~');
        selectedVar = -1;                     // prevent returning "1"
        return selectedVar;                   // message is done, return the variable's array index and break
      }
      else if (rc == '-') {                   // if "-" read out, store command to process the variable recieved
                                              // as negative TODO, BUG this could occur anywere before < >, requires
                                              // variable to store ndx where recieved was flagged to look only there
        negScale = -1;
      }
      else if ((rc != feedbackMarker || rc != endMarker) && isDigit(rc)) { // if the bit wasn't accounted for yet, assume it's a digit
                                                        // BUG if it's not a digit, could bug the string2int FIXED
        writeChar[ndx] = rc;          // Write the digit to the character array at ndx
        ndx++;                        // increment ndx
        //if (ndx >= numChars) {
        if (ndx > numChars) {       // original 'if' here didn't make much sense if things are working but could cause
                                    // bugs in the recieved, so turned this into a error & escape
          //ndx = numChars - 1;
          recvInProgress = false;   //reset the statics
          ndx = 0;
          Serial.print("error");                 // send an error
          Serial.print(',');
          Serial.println('~');
          while (Serial.available()>0) {    // flush the buffer
            Serial.read();
          }
          selectedVar = -1;                     // prevent returning "1"
          return selectedVar; 
        }
      }
    }
  }
  return selectedVar;                   // prevent returning "1" or "0"
}


// ******************************************************
// FUNCTIONS TO REVIEW 
// ******************************************************

// TODO remap and add variables

void setPulseTrainVars(int recVar, int recVal) {

  int parsedChan = recVal % 10;  // ones digit is the channel
  int parsedValue = recVal * 0.1; // divide by 10 and round up

  // IPI
  if (recVar == 9) {
    pulseTrainVars[parsedChan - 1][2] = parsedValue;
  }
  else if (recVar == 10) {
    pulseTrainVars[parsedChan - 1][3] = parsedValue;
  }
  else if (recVar == 11) {
    pulseTrainVars[parsedChan - 1][5] = parsedValue;
  }
  else if (recVar == 12) {
    pulseTrainVars[parsedChan - 1][6] = parsedValue;
  }
  else if (recVar == 13) {
    // if you push pulses; make sure stop bit is off
    pulseTrainVars[parsedChan - 1][8] = parsedValue;
    pulseTrainVars[parsedChan - 1][1] = 0;
  }
}

void dataReport() {
  Serial.print("tData");
  Serial.print(',');
  Serial.print(loopCount);
  Serial.print(',');
   // TODO needed?
  Serial.print(trialTime);
  Serial.print(',');
  Serial.print(stateTime);
  Serial.print(',');
  Serial.print(knownValues[0]); //state
  Serial.print(',');
  Serial.print(knownValues[14]);  //load cell
  Serial.print(',');
  Serial.print(lickSensorAValue); // lick sensor
  Serial.print(',');
  Serial.print(analogAngle);     //rotary encoder value
  Serial.print(',');
  Serial.print(pulseCount);
  Serial.print(',');
  Serial.print(loopTime);
  Serial.print(',');
  Serial.print(genAnalogInput0);
  Serial.print(',');
  Serial.print(genAnalogInput1);
  Serial.print(',');
  // Serial.print(pulseTrainVars[0][7]);
  Serial.print(genAnalogInput2);
  Serial.print(',');
  // Serial.println(pulseTrainVars[0][8]);
  Serial.print(genAnalogInput3);
}



/*int flagReceiveDashboard(uint32_t valAr[]) {
  static boolean recvInProgress2 = false;
  static byte ndx2 = 0;
  char endMarker = '>';
  char feedbackMarker = '<';
  char rc;
  uint32_t nVal;
  const byte numChars = 32;
  char writeChar[numChars];
  static int selectedVar = 0;
  int newData = 0;

  while (dashSerial.available() > 0 && newData == 0) {
    rc = dashSerial.read();

    if (recvInProgress2 == false) {
      if (rc == 'b') {
        selectedVar = 0;
        recvInProgress2 = true;
      }
      else if (rc == 'n') {
        selectedVar = 1;
        recvInProgress2 = true;
      }

    }

    else if (recvInProgress2 == true) {
      if (rc == endMarker ) {
        //        Serial.println(selectedVar);
        writeChar[ndx2] = '\0'; // terminate the string
        recvInProgress2 = false;
        ndx2 = 0;
        newData = 1;

        nVal = uint32_t(String(writeChar).toInt());
        valAr[selectedVar] = nVal;

      }
      else if (rc == feedbackMarker) {
        writeChar[ndx2] = '\0'; // terminate the string
        recvInProgress2 = false;
        ndx2 = 0;
        newData = 1;
        dashSerial.print("echo");
        dashSerial.print(',');
        dashSerial.print(valAr[selectedVar]);
        dashSerial.print(',');
        dashSerial.println('~');
      }

      else if (rc != feedbackMarker || rc != endMarker) {
        writeChar[ndx2] = rc;
        ndx2++;
        if (ndx2 >= numChars) {
          ndx2 = numChars - 1;
        }
      }
    }
  }
  return newData; // tells us if a valid variable arrived.
}*/

void resetHeaders() {
  for ( int i = 0; i < stateCount; i++) {
    headerStates[i] = 0;
  }
}

void genericHeader(int stateNum) {
  // a: reset header timer
  headerTime = 0;

  // b: reset header states and set current state's header to 1 (fired).
  resetHeaders();
  headerStates[stateNum] = 1;

  for ( int i = 0; i < 5; i++) {
    trainTimer[i] = 0;
  }

  headerStates[stateNum] = 1;
  // c: set analog output values to 0.
  analogOutVals[0] = 0;
  analogOutVals[1] = 0;
  analogOutVals[2] = 0;
  analogOutVals[3] = 0;
  analogOutVals[4] = 0;
  // NO sDacGate1 = digitalRead(dacGate1);
  // NO sDacGate2 = digitalRead(dacGate2);
  pollToggle();

  pulseTrainVars[0][0] = 1;
  pulseTrainVars[1][0] = 1;
  pulseTrainVars[2][0] = 1;
  pulseTrainVars[3][0] = 1;
  pulseTrainVars[4][0] = 1;

  pulseTrainVars[0][1] = 0;
  pulseTrainVars[1][1] = 0;
  pulseTrainVars[2][1] = 0;
  pulseTrainVars[3][1] = 0;
  pulseTrainVars[4][1] = 0;

  pulseTrainVars[0][7] = 0;
  pulseTrainVars[1][7] = 0;
  pulseTrainVars[2][7] = 0;
  pulseTrainVars[3][7] = 0;
  pulseTrainVars[4][7] = 0;

  pulseTrainVars[0][9] = 0;
  pulseTrainVars[1][9] = 0;
  pulseTrainVars[2][9] = 0;
  pulseTrainVars[3][9] = 0;
  pulseTrainVars[4][9] = 0;

  // d: reset state timer.
  stateTime = 0;
}

void genericStateBody() {

  lickSensorAValue = analogRead(lickPinA);
  // NO lickSensorAValue = analogRead(lickPinB);
  genAnalogInput0 = analogRead(genA0);
  genAnalogInput1 = analogRead(genA1);
  genAnalogInput2 = analogRead(genA2);
  genAnalogInput3 = analogRead(genA3);
  pollToggle();
  analogAngle = analogRead(analogMotion);
  writeAnalogOutValues(analogOutVals);
  if (scale.is_ready()) {
    scaleVal = scale.get_units() * 22000;
    // this scale factor gives hundreths of a gram as the least significant int
    knownValues[14] = scaleVal;
  }
}

// ****************************************************************
// **************  Visual Stimuli *********************************
// ****************************************************************

void visStim(int stimType) {
  if (stimType == 0) {
    visualSerial.print('v');
    visualSerial.print(',');
    visualSerial.print(0);
    visualSerial.print(',');
    visualSerial.print(0);
    visualSerial.print(',');
    visualSerial.print(0);
    visualSerial.print(',');
    visualSerial.print(0);
    visualSerial.print(',');
    visualSerial.print(knownValues[18]);
    visualSerial.print(',');
    visualSerial.print(knownValues[19]);
    visualSerial.print(',');
    visualSerial.println(knownValues[20]);
  }
  //1 is on
  if (stimType == 2) {
    visualSerial.print('v');
    visualSerial.print(',');
    visualSerial.print(knownValues[4]);
    visualSerial.print(',');
    visualSerial.print(knownValues[3]);
    visualSerial.print(',');
    visualSerial.print(knownValues[5]);
    visualSerial.print(',');
    visualSerial.print(knownValues[6]);
    visualSerial.print(',');
    visualSerial.print(knownValues[18]);
    visualSerial.print(',');
    visualSerial.print(knownValues[19]);
    visualSerial.print(',');
    visualSerial.println(knownValues[20]);
  }
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

void flybackStim_On() {

  pulseCount = pulseCount + 1;
  if (knownValues[0] == 8) {

    elapsedMicros pfTime;
    pfTime = 0;
    while (pfTime <= knownValues[16]) {
      stimGen(pulseTrainVars);
      analogWrite(DAC1, pulseTrainVars[0][7]);
      analogWrite(DAC2, pulseTrainVars[1][7]);
      mDAC1.Set(pulseTrainVars[2][7], pulseTrainVars[3][7]);
    }
    analogWrite(DAC1, 0);
    analogWrite(DAC2, 0);
    mDAC1.Set(0, 0);
  }
}



// ****************************************************************
// **************  Pulse Train Function ***************************
// ****************************************************************
void setAnalogOutValues(uint32_t dacVals[], uint32_t pulseTracker[][10]) {
  if (sDacGate1 == 0) {
    dacVals[0] = pulseTracker[0][7];
    dacVals[1] = pulseTracker[1][7];
  }
  else if (sDacGate1 == 1) {
    dacVals[0] = pulseTracker[1][7];
    dacVals[1] = pulseTracker[0][7];
  }
  if (sDacGate2 == 0) {
    dacVals[2] = pulseTracker[2][7];
    dacVals[3] = pulseTracker[3][7];
  }
  else if (sDacGate2 == 1) {
    dacVals[2] = pulseTracker[3][7];
    dacVals[3] = pulseTracker[2][7];
  }
  dacVals[4] = pulseTracker[4][7];
}

void writeAnalogOutValues(uint32_t dacVals[]) {
  analogWrite(DAC1, dacVals[0]);
  analogWrite(DAC2, dacVals[1]);
  mDAC1.Set(dacVals[2], dacVals[3]);
  mDAC2.Set(dacVals[4], dacVals[4]);
}

void stimGen(uint32_t pulseTracker[][10]) {

  int i;
  int updateCount = 0;
  for (i = 0; i < dacNum; i = i + 1) {
    int stimType = pulseTracker[i][6];
    int pulseState = pulseTracker[i][0];

    //**************************
    // *** 0 == Square Waves
    //**************************
    if (stimType == 0) {

      // a) pulse state
      if (pulseState == 1) {
        // *** 1) check for exit condition (pulse over)
        if (trainTimer[i] >= pulseTracker[i][3]) {
          trainTimer[i] = 0; // reset counter
          pulseTracker[i][0] = 0; // stop pulsing
          pulseTracker[i][7] = pulseTracker[i][4];
          // *** 1b) This is where we keep track of pulses completed. (99999 prevents pulse counting)
          updateCount = 1;
        }

        // *** 2) determine pulse amplitude
        else {
          if (pulseTracker[i][1] == 0) {
            pulseTracker[i][7] = pulseTracker[i][5]; // 5 is the pulse amp; 7 is the current output.
          }
          else if (pulseTracker[i][1] == 1) {
            pulseTracker[i][7] = pulseTracker[i][4]; // baseline
          }
        }
      }

      // b) baseline/delay state
      else if (pulseState == 0) {
        // if we are out of baseline time; move to stim state
        if (trainTimer[i] >=  pulseTracker[i][2]) {
          trainTimer[i] = 0; // reset counter
          pulseTracker[i][0] = 1; // start pulsing
          if (pulseTracker[i][1] == 0) {
            pulseTracker[i][7] = pulseTracker[i][5];
          }
          else if (pulseTracker[i][1] == 1) {
            pulseTracker[i][7] = pulseTracker[i][4];
          }
        }
        else {
          // but if we have delay time, then we use baseline
          pulseTracker[i][7] = pulseTracker[i][4];
          // 4 is the baseline amp; 6 is the current output.
        }
      }
    }

    // add ramp back here

    // 2) Asymm Cosine
    else if (stimType == 2) {
      // PULSE STATE
      pulseTracker[i][7] = pulseTracker[i][4];
      if (pulseState == 1) {
        // These pulses idealy have a variable time.
        if (trainTimer[i] <= 10) {
          float curTimeSc = PI + ((PI * (trainTimer[i] - 0)) / 10);
          pulseTracker[i][7] = pulseTracker[i][5] * ((cosf(curTimeSc) + 1) * 0.5); // 5 is the pulse amp; 7 is the current output.
        }
        else if ((trainTimer[i] > 10) && (trainTimer[i] < 106)) {
          float curTimeSc = 0 + ((PI * (trainTimer[i] - 8)) / 100);
          pulseTracker[i][7] = pulseTracker[i][5] * ((cosf(curTimeSc) + 1) * 0.5); // 5 is the pulse amp; 7 is the current output.
        }
        else if (trainTimer[i] >= 106) {
          pulseTracker[i][0] = 0;
          trainTimer[i] = 0;
          pulseTracker[i][7] = pulseTracker[i][4];
          updateCount = 1;
        }
        // if pulse count flips the bit, just do baseline (veto)
        if (pulseTracker[i][1] == 1) {
          pulseTracker[i][7] = pulseTracker[i][4]; // baseline
        }
      }

      // baseline STATE
      else if (pulseState == 0) {
        pulseTracker[i][7] = pulseTracker[i][4];
        if (trainTimer[i] >=  pulseTracker[i][2]) {
          pulseTracker[i][0] = 1;
          pulseTracker[i][7] = pulseTracker[i][4];
          trainTimer[i] = 0;
        }
      }
    }

    // *** Type Independent Stuff ***
    // *** This is where we keep track of pulses completed. (99999 prevents pulse counting)
    if (updateCount==1){
      if ((pulseTracker[i][8]  > 0) && (pulseTracker[i][8] != 99999)) {
        pulseTracker[i][8] = pulseTracker[i][8] - 1;
        if (pulseTracker[i][8] <= 0) {
          // flip the stop bit
          pulseTracker[i][1] = 1;
          // make the pulse number 0 (don't go negative)
          pulseTracker[i][8] = 0;
        }
      }
      updateCount = 0;
    }
    // *** next chan
  }
}



// ----------------------------------------------
// ---------- NEOPIXEL FUNCTIONS ----------------
// ----------------------------------------------

void setStrip(uint32_t stripState) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    if (stripState == 1) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    else if (stripState == 2) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, 255));
    }
    else if (stripState == 3) {
      strip.setPixelColor(i, strip.Color(255, 0, 0));
    }
    else if (stripState == 4) {
      strip.setPixelColor(i, strip.Color(0, 255, 0));
    }
    else if (stripState == 5) {
      strip.setPixelColor(i, strip.Color(0, 0, 255));
    }
    else if (stripState == 6) {
      strip.setPixelColor(i, strip.Color(255, 0, 255));
    }
    else if (stripState == 7) {
      strip.setPixelColor(i, strip.Color(random(256), random(256), random(256)));
    }
  }
  strip.show();
}

void pollToggle() {
  if (knownValues[15] != 0) {
    int parsedValue = knownValues[15] % 10;  // ones digit is the bool
    int parsedChan = knownValues[15] * 0.1; // divide by 10 and round up

    if (parsedValue == 2) {
      digitalWrite(parsedChan, 1);
    }
    else if (parsedValue == 1) {
      digitalWrite(parsedChan, 0);
    }
    else {
      digitalWrite(parsedChan, 0);
    }
    knownValues[15] = 0;
  }
}

void pollRelays() {
  bool rTrig;
  bool rRwd;
  rTrig = digitalRead(syncMirror);
  rRwd = digitalRead(rewardMirror);
  if (rTrig == 1) {
    digitalWrite(syncPin, HIGH);
    delay(5);
    digitalWrite(syncPin, LOW);
  }
  if (rRwd == 1) {
    digitalWrite(rewardPin, HIGH);
    delay(5);
    digitalWrite(rewardPin, LOW);
  }
}

void pollTones(uint32_t tonePin, uint32_t toneFreq, uint32_t toneDuration) {
  tone(tonePin, toneFreq, toneDuration);
}

void pollColorChange() {

  if (knownValues[7] != lastBrightness) {
    if (knownValues[7] > maxBrightness) {
      knownValues[7] = maxBrightness;
    }
    strip.setBrightness(knownValues[7]);
    strip.show();
    lastBrightness = knownValues[7];
  }

  // b) Handle color changes.
  if (knownValues[8] > 0 && knownValues[8] < 8) {
    setStrip(knownValues[8]);
    knownValues[8] = 0;
  }
  else {
    knownValues[8] = 0;
  }
}

void secSPIWrite(uint32_t value, int csPin) {
  uint16_t out = (0 << 15) | (1 << 14) | (1 << 13) | (1 << 12) | ( int(value) );
  digitalWriteFast(csPin, LOW);
  SPI.transfer(out >> 8);                   //you can only put out one byte at a time so this is splitting the 16 bit value.
  SPI.transfer(out & 0xFF);
  digitalWriteFast(csPin, HIGH);
}


// ******************************************************
// OLD OR UNUSED FUNCTIONS
// ******************************************************

// ******************************************************
/* Old flag recieve changed to return specific non-variable integer (-1) is called w/o a serial buffer or if a paremeter 
 *  request is recieved. Returns header number only when paremeter change is rececieved. Also the max receieved int size limit
 *  was changed to reflect 12 bit (10 digits plus null) and the exceed case was changed to an error message escape with
 *  a buffer flush.
 */

  /*
int flagReceive(char varAr[], int32_t valAr[]) {
  static byte ndx = 0;          // finds place being read
  char endMarker = '>';         // Designates recieve message
  char feedbackMarker = '<';    // Designates send/requested message
  char rc;                      // recieved variable header character
  uint32_t nVal;                // Variable value (I think)
  const byte numChars = 32;     // Size of string to store each digit of int TODO make smaller? <= 10 ?
  char writeChar[numChars];     // Make the string of this size
  int selectedVar = 0;          // Returned index of kV header of recieved variable for Pulse update TODO: pulses be updated here?
  static boolean recvInProgress = false;  // Bit indicating in recieve PERSISTS THROUGH CALLS TO THIS FUNCTION !
  bool newData = 0;               // Bit breaks loop when true, fliped when variable recieve is finished 
  int32_t negScale = 1;         // used to convert the ABS value of the variable recieved to negative when '-' read out

  while (Serial.available() > 0 && newData == 0) {    // set a loop to run if there is a bit in the buffer until the message is read
    rc = Serial.read();                               // read the (next) bit and store it to RC

    if (recvInProgress == false) {                // If this is a new message (variable; not partially read)
      for ( int i = 0; i < knownCount; i++) {   // Loop through the number of knownValues variables
        if (rc == varAr[i]) {                   // if the read byte == the looped to (any) kV Header char
          selectedVar = i;                      // set the return value to variable's the index of the kV Header 
          recvInProgress = true;                //  indicate the message is being read by flipping rcvInProg
        }
      }
    }

    else if (recvInProgress == true) {        // if the message is in the process of being read
      
      if (rc == endMarker ) {                 // if the bit is a revieve message stop 
        writeChar[ndx] = '\0';                // terminate the string by writing a non-integer to writeChar at present ndx
        recvInProgress = false;               // indicate finished message by flipping rcvInProg
        ndx = 0;                              // reset the index 
        newData = 1;                          // indecate that the message is read (and ready)
        nVal = int32_t(String(writeChar).toInt());  //convert the Char to string and then to integer and call it nVal
        valAr[selectedVar] = nVal * negScale; // process ABS value with +/- read and set the (global) array value equal to it
        return selectedVar;                   // message is done, return the variable's array index and break
      }

      else if (rc == feedbackMarker) {        // if the bit is a send message stop
        writeChar[ndx] = '\0';                // terminate the string by writing a non-integer to writeChar at present ndx
        recvInProgress = false;               // indicate finished message by flipping rcvInProg 
        ndx = 0;                              // reset the index 
        newData = 1;                          // indecate that the message is read (and ready)
                                  //don't write anything
        Serial.print("echo");                 // send a line through serial with the requested value
        Serial.print(',');
        Serial.print(varAr[selectedVar]);
        Serial.print(',');
        Serial.print(valAr[selectedVar]);
        Serial.print(',');
        Serial.println('~');
      }
      else if (rc == '-') {                   // if "-" read out, store command to process the variable recieved
                                              // as negative TODO, BUG this could occur anywere before < >, requires
                                              // variable to store ndx where recieved was flagged to look only there
        negScale = -1;
      }
      else if ((rc != feedbackMarker || rc != endMarker) && isDigit(rc)) { // if the bit wasn't accounted for yet, assume it's a digit
                                                        // BUG if it's not a digit, could bug the string2int FIXED
        writeChar[ndx] = rc;          // Write the digit to the character array at ndx
        ndx++;                        // increment ndx
        //if (ndx >= numChars) {
        if (ndx > numChars) {       // original 'if' here didn't make much sense if things are working but could cause
                                    // bugs in the recieved, so turned this into a error & escape
          //ndx = numChars - 1;
          recvInProgress = false;   //reset the statics
          ndx = 0;
          Serial.print("error");                 // send an error
          Serial.print(',');
          Serial.println('~');
          while(Serial.available>0){    // flush the buffer
            Serial.read();
          }
          return;
        }
      }
    }
  }
} */
// ******************************************************
