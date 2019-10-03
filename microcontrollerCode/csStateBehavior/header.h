//uint8_t dac3Address = 0x61;
//uint8_t dac4Address = 0x60;

#include <MCP4922.h>


uint32_t microsPerSamp = 1000;
//-----------------------------
// ~~~~~~~ IO Pin Defs ~~~~~~~~
//-----------------------------
//


// a) Analog Input Pins
#define genA0 A2
#define genA1 A3
#define genA2 A4
#define genA3 A5
#define analogMotion A6


// b) Digital Input Pins
#define lickPinA 2    // Lick/Touch Sensor A 
#define lickPinB 3    // Lick/Touch Sensor B
#define scaleData 4
#define scaleClock 5

// c) Digital Interrupt Input Pins
#define motionPin 6
#define framePin 28
#define yGalvo  31

// d) Digital Output Pins
#define rewardPin 9  // Trigger/signal a reward
#define syncPin  29   // Trigger other things like a microscope and/or camera
#define sessionOver 30  
#define ledSwitch 12

#define neoStripPin 21
#define pmtBlank  22



// f) True 12-bit DACs (I define as an array object to loop later)
// on a teensy 3.2 A14 is the only DAC
// MCP DACs (3&4) can be powered by 5V, and will give 5V out.
// Teensy DACs are 3.3V, but see documentation for simple opamp wiring to get 5V peak.
//#define DAC1 A21
//#define DAC2 A22

MCP4922 mDAC1(11, 13, 10, 7);
MCP4922 mDAC2(11, 13, 10, 8);


uint32_t lickSensorAValue = 0;
uint32_t genAnalogInput0 = 0;
uint32_t genAnalogInput1 = 0;
uint32_t genAnalogInput2 = 0;
uint32_t genAnalogInput3 = 0;
uint32_t analogAngle = 0;

// session header
bool startSession = 0;

uint32_t vStim_xPos = 800;
uint32_t vStim_yPos = 800;

elapsedMillis trialTime;
elapsedMillis stateTime;
elapsedMicros headerTime;
elapsedMicros loopTime;

bool stateChanged = 0;
