#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Trinamic_TMC2130.h>
#include <AccelStepper.h>

const char* ssid = "CloudyBrain";
const char* password = "333stamp";

// Pass touchscreen calibration.
#define TS_MINX 150
#define TS_MAXY 130
#define TS_MAXX 3800
#define TS_MINY 4000

// Screen related pins
#define tft_cs   15
#define tft_dc   33
#define touchScr_cs 32

// stepper pins
#define stpPin_cs 25
#define stpPin_dir 26
#define stpPin_enable 21
#define stpPin_step 4

#define stpPin_rwdTrig 13
#define rotateState 2

// make stepper(s) and screen
Trinamic_TMC2130 spiStepper(stpPin_cs);
Adafruit_STMPE610 touchScr = Adafruit_STMPE610(touchScr_cs);
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft_cs, tft_dc);

// we also make an accel stepper stepper object.
// we use the spi trinamic library configure the driver
// we use accelStepper to move it
AccelStepper accelStepper(AccelStepper::DRIVER, stpPin_step, stpPin_dir);

// Dashboard State Stuff
bool blockScreen = 0;
bool sHead[] = {0, 0, 0};
int curScreen = 1;

// **********************************
// **** Serial Variable Guide *******
// **********************************
// r/0: reward state (bool; 1 to trigger a reward)
// s/1: motor speed
// a/2: motor acceleration
// p/3: volume per microstep in pL
// v/4: reward volume in nl
// d/5: direction (0 for forward; 1 for backward)
// m/6: number of intrinsic steps per revolution the motor has (usually 200 or 400)
// u/7: microstepping resolution (powers of 2 til 256);


char knownHeaders[] = {'r', 's', 'a', 'p', 'v', 'd', 'm', 'u'};
int knownValues[] = {0, 25600, 10000, 14311, 1600, 0, 200, 256};
int lastValues[] = {0, 25600, 10000, 14311, 1600, 0, 200, 256};
bool varChanged[] = {0, 0, 0, 0, 0, 0, 0, 0};
int knownCount = 8;


// ******************************************
// **** Button State/Variable Tracker *******
// ******************************************
//
// Button catalog:
// 0:
// 1:
// 2:
int buttonBounceCounter[] = {0, 0, 0, 0, 0, 0 , 0, 0};
int buttonBounceTime[] = {500, 500, 2000, 500, 500, 500, 500, 2000};
int buttonCount = 8;

int dashState = 0;
int homeTimer = 0;
int homeRefreshTime = 1000;

// variables related to button rendering

int rowBuf = 5;
int textScale = 1;
int textHeight = 10 * textScale;
int textWidth = 5 * textScale;

int bWidth = 70;
int bHeight = 30;
int bBuf = 5;
int bTBuf = 18;
int bRow = 290;
float cur_lux = 0;
float last_lux = 0;

int dRwBtnSelected = 0;
bool onWifi = 0;




void setup(void) {
  tft.setFont(&FreeMono9pt7b);
  Serial.begin (115200);  // USB monitor
  delay(100);
  Serial2.begin(115200);  // HW UART1
  delay(100);
  initializeStepper();
  resetStepper();
  tft.begin();
  if (!touchScr.begin()) {
    Serial.println("Unable to start touchscreen.");
  }
  else {
    Serial.println("Touchscreen started.");
  }
  digitalWrite(stpPin_enable, HIGH);
  delay(10);
  dashState = 0;
}



void loop() {
  // a) Handle button latches.
  checkBounceStates();

  // b) Look for usb serial changes.
  int nD = flagReceive(knownHeaders, knownValues);
  checkForVarChange();

  // c) Give a reward bolus if asked.
  bool giveReward = digitalRead(stpPin_rwdTrig);
  if ((knownValues[0] == 1) || (giveReward == 1)) {
    dispReward(knownValues[4], knownValues[5], knownValues[7], knownValues[1], knownValues[2]);
    knownValues[0] = 0;
  }

  // d) If we allow screen updates, check for, and resolve button touches.
  if (blockScreen == 0) {
    resolveTaps();
    // *********** S0: Default Info State
    if (dashState == 0) {
      if (sHead[0] == 0) {
        sHead[0] = 1;
        sHead[1] = 0;
        sHead[2] = 0;
        createHome();
        curScreen = 1;
        homeTimer = 0;
      }
    }

    //    else if (dashState == 1) {
    //      if (sHead[1] == 0) {
    //        sHead[0] = 0;
    //        sHead[1] = 1;
    //        sHead[2] = 0;
    //        createMotor();
    //        curScreen = 2;
    //        homeTimer = 0;
    //      }
    //    }

  }
}




//void updateHomeDispVal(int gap, int row, int valID) {
//  tft.setCursor(gap * textWidth, (textHeight + rowBuf) * row);
//  tft.setTextColor(ILI9341_BLACK);
//  tft.print(home_lastValues[valID]);
//  tft.setTextColor(ILI9341_RED);
//  tft.setCursor(gap * textWidth, (textHeight + rowBuf) * row);
//  tft.print(home_knownValues[valID]);
//}


void stepSPI(int stepDelay) {
  digitalWrite(stpPin_enable, HIGH);
  delayMicroseconds(stepDelay);
  digitalWrite(stpPin_step, LOW);
  delayMicroseconds(stepDelay);
}


void resetStepper() {
  pinMode(stpPin_enable, OUTPUT);
  pinMode(stpPin_dir, OUTPUT);
  pinMode(stpPin_step, OUTPUT);
  digitalWrite(stpPin_enable, HIGH); // disable driver
  digitalWrite(stpPin_dir, LOW); // chose direction
  digitalWrite(stpPin_step, LOW); // no step yet
}

void initializeStepper() {
  spiStepper.init();
  spiStepper.set_mres(knownValues[7]); // ({1,2,4,8,16,32,64,128,256}) number of microsteps
  spiStepper.set_IHOLD_IRUN(31, 31, 5); // ([0-31],[0-31],[0-5]) sets all currents to maximum
  spiStepper.set_I_scale_analog(1); // ({0,1}) 0: I_REF internal, 1: sets I_REF to AIN
  spiStepper.set_tbl(1); // ([0-3]) set comparator blank time to 16, 24, 36 or 54 clocks, 1 or 2 is recommended
  spiStepper.set_toff(8); // ([0-15]) 0: driver disable, 1: use only with TBL>2, 2-15: off time setting during slow decay phase
}

void refreshHomeVars() {
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(0, (textHeight + rowBuf) * 1);
  tft.print("lux: ");
  tft.print(last_lux);
  last_lux = cur_lux;
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(0, (textHeight + rowBuf) * 1);
  tft.print("lux: ");
  tft.print(last_lux);
}

void createHome() {

  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(rotateState);

  s0Btn(1);
  s1Btn(0);
  s2Btn(0);
  s4Btn(0);
  s5Btn(0);
  s6Btn(0);
  s7Btn(0);
  s8Btn(0);
  s9Btn(0);
  s10Btn(0);
  s11Btn(0);

  // a) SSID INFO
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(textScale);
  if (onWifi == 0) {
    tft.print("ssid: Not Connected");
  }
  else if (onWifi == 0) {
    tft.print("ssid: ");
    tft.print("CloudyBrain");
  }


  //  tft.setCursor(0, (textHeight + rowBuf) * 1);
  //  tft.print("temp: ");
  //  tft.setCursor(0, (textHeight + rowBuf) * 2);
  //  tft.print("humid: ");
  tft.setCursor(0, (textHeight + rowBuf) * 2);
  tft.print("direction: ");
  if (knownValues[5] == 0) {
    tft.print("forward");
  }
  else if (knownValues[5] == 1) {
    tft.print("backward");
  }
  tft.setCursor(0, (textHeight + rowBuf) * 3);
  tft.print("vol (uL): ");
  tft.print(float((float(knownValues[3])*float(knownValues[4])) / float(10000000)));
  tft.setCursor(0, (textHeight + rowBuf) * 4);
  tft.print("motor speed: ");
  tft.print(knownValues[1]);
  tft.setCursor(0, (textHeight + rowBuf) * 5);
  tft.print("motor acel: ");
  tft.print(knownValues[2]);
}


// Button Functions
void s0Btn(bool selState) {
  int bLoc = ((1 * bBuf) + (0 * bWidth));
  if (selState == 0) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("home");
  }
  else if (selState == 1) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_GREEN);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("home");
  }
}

void s1Btn(bool selState) {
  int bLoc = ((2 * bBuf) + (1 * bWidth));
  if (selState == 0) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print(" ~ ");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_GREEN);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print(" ~ ");
  }
}


void s2Btn(bool selState) {
  int bLoc = ((3 * bBuf) + (2 * bWidth));
  if (selState == 0) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print(" ~ ");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_GREEN);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print(" ~ ");
  }
}


void s4Btn(bool selState) {
  // motor speed
  int bLoc = ((1 * bBuf) + (0 * bWidth));
  int tbRow = 110;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("sp +");
  }
  if (selState == 1) {
    if (buttonBounceCounter[0] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(textScale);
      tft.print("sp +");
      int tlVal = knownValues[1];
      knownValues[1] = knownValues[1] + 100;
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("motor speed: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("motor speed: ");
      tft.print(knownValues[1]);
      buttonBounceCounter[0] = buttonBounceCounter[0] + 1;
      delay(100);
      s4Btn(0);
    }
  }
}

void checkBounceStates() {
  for ( int i = 0; i < buttonCount; i++) {
    if (buttonBounceCounter[i] > 0) {
      buttonBounceCounter[i] = buttonBounceCounter[i] + 1;
      if (buttonBounceCounter[i] >= buttonBounceTime[i]) {
        buttonBounceCounter[i] = 0;
      }
    }
  }
}

void s5Btn(bool selState) {
  int bLoc = ((2 * bBuf) + (1 * bWidth));
  int tbRow = 110;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("sp -");
  }
  if (selState == 1) {
    if (buttonBounceCounter[1] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(textScale);
      tft.print("sp -");
      int tlVal = knownValues[1];
      knownValues[1] = knownValues[1] - 100;
      if (knownValues[1] <= 0) {
        knownValues[1] = 1;
      }
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("motor speed: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("motor speed: ");
      tft.print(knownValues[1]);
      buttonBounceCounter[1] = buttonBounceCounter[1] + 1;
      delay(100);
      s5Btn(0);
    }
  }
}

void s6Btn(bool selState) {
  int bLoc = ((3 * bBuf) + (2 * bWidth));
  int tbRow = 110;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("rwd");
  }
  if (selState == 1) {
    if (buttonBounceCounter[2] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(textScale);
      tft.print("rwding");
      knownValues[0] = 1;
      buttonBounceCounter[2] = buttonBounceCounter[2] + 1;
      delay(100);
      s6Btn(0);
    }
  }
}



void s7Btn(bool selState) {
  int bLoc = ((1 * bBuf) + (0 * bWidth));
  int tbRow = 150;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("ac +");
  }
  if (selState == 1) {
    if (buttonBounceCounter[3] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(textScale);
      tft.print("ac +");
      int tlVal = knownValues[2];
      knownValues[2] = knownValues[2] + 100;
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 5);
      tft.print("motor acel: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 5);
      tft.print("motor acel: ");
      tft.print(knownValues[2]);
      buttonBounceCounter[3] = buttonBounceCounter[3] + 1;
      delay(100);
      s7Btn(0);
    }
  }
}


void s8Btn(bool selState) {
  int bLoc = ((2 * bBuf) + (1 * bWidth));
  int tbRow = 150;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("ac -");
  }
  if (selState == 1) {
    if (buttonBounceCounter[4] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(textScale);
      tft.print("ac +");
      int tlVal = knownValues[2];
      knownValues[2] = knownValues[2] - 100;
      if (knownValues[2] <= 0) {
        knownValues[2] = 1;
      }
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 5);
      tft.print("motor acel: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 5);
      tft.print("motor acel: ");
      tft.print(knownValues[2]);
      buttonBounceCounter[4] = buttonBounceCounter[4] + 1;
      delay(100);
      s8Btn(0);
    }
  }
}

void s9Btn(bool selState) {
  int bLoc = ((1 * bBuf) + (0 * bWidth));
  int tbRow = 190;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("vol+");
  }
  if (selState == 1) {
    if (buttonBounceCounter[5] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(textScale);
      tft.print("vol+");
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 3);
      tft.print("vol (uL): ");
      tft.print(float((float(knownValues[3])*float(knownValues[4])) / float(10000000)));
      knownValues[4] = knownValues[4] + 160;
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 3);
      tft.print("vol (uL): ");
      tft.print(float((float(knownValues[3])*float(knownValues[4])) / float(10000000)));
      buttonBounceCounter[5] = buttonBounceCounter[5] + 1;
      delay(100);
      s9Btn(0);
    }
  }
}


void s10Btn(bool selState) {
  int bLoc = ((2 * bBuf) + (1 * bWidth));
  int tbRow = 190;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("vol-");
  }
  if (selState == 1) {
    if (buttonBounceCounter[6] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(textScale);
      tft.print("vol-");

      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 3);
      tft.print("vol (uL): ");
      tft.print(float((float(knownValues[3])*float(knownValues[4])) / float(10000000)));
      knownValues[4] = knownValues[4] - 160;
      if (knownValues[4] <= 160) {
        knownValues[4] = 160;
      }
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 3);
      tft.print("vol (uL): ");
      tft.print(float((float(knownValues[3])*float(knownValues[4])) / float(10000000)));
      buttonBounceCounter[6] = buttonBounceCounter[6] + 1;
      delay(100);
      s10Btn(0);
    }
  }
}

void s11Btn(bool selState) {
  int bLoc = ((1 * bBuf) + (0 * bWidth));
  int tbRow = 230;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(textScale);
    tft.print("dir");
  }
  if (selState == 1) {
    if (buttonBounceCounter[7] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(textScale);
      int tlVal = knownValues[5];
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 2);
      tft.print("direction: ");
      if (tlVal == 0) {
        tft.print("forward");
      }
      else if (tlVal == 1) {
        tft.print("backward");
      }
      knownValues[5] = 1 - knownValues[5];
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 2);
      tft.print("direction: ");
      if (knownValues[5] == 0) {
        tft.print("forward");
      }
      else if (knownValues[5] == 1) {
        tft.print("backward");
      }
      buttonBounceCounter[7] = buttonBounceCounter[7] + 1;
      delay(100);
      s11Btn(0);
    }
  }
}





void dispReward(int numMuSteps, int cDir, int muRes, int curSpeed, int curAccel) {
  int dirMult;
  if (cDir == 0) {
    dirMult = 1;
  }
  else if (cDir == 1) {
    dirMult = -1;
  }
  accelStepper.setMaxSpeed(muRes * curSpeed);
  accelStepper.setSpeed(muRes * curSpeed);
  accelStepper.setAcceleration(muRes * curAccel);
  digitalWrite(stpPin_enable, LOW);
  long curPos = accelStepper.currentPosition();
  accelStepper.runToNewPosition(curPos + (dirMult * numMuSteps));
  digitalWrite(stpPin_enable, HIGH);
}

void refreshScreen(int targScreen) {
  if (targScreen == 1) {
    createHome();
  }
}


void resolveTaps() {
  if (!touchScr.bufferEmpty()) {
    // Retrieve a point
    TS_Point p = touchScr.getPoint();
    // Scale using the calibration #'s
    // and rotate coordinate system
    p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
    p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
    int y = tft.height() - p.x;
    int x = p.y;

    // ds1,ds2,ds3
    int bX1[] = {15, 15, 15, 125, 125, 125, 100, 100};
    int bX2[] = {30, 30, 30, 150, 150, 150, 120, 120};
    int bY1[] = {30, 130, 220, 30, 130, 220, 30, 130};
    int bY2[] = {90, 190, 290, 90, 190, 290, 90, 190};

    // A) **** Resolve Button Taps
    if ((x >= bX1[0]) && (x <= bX2[0])) {
      if ((y >= bY1[0]) && (y <= bY2[0])) {
        dashState = 0;
      }
      else if ((y >= bY1[1]) && (y <= bY2[1])) {
        // dashState = 1;
      }
      else if ((y >= bY1[2]) && (y <= bY2[2])) {
        // dashState = 2;
      }
    }

    // A) **** Resolve Button Taps
    else if ((x >= bX1[3]) && (x <= bX2[3])) {
      if ((y >= bY1[3]) && (y <= bY2[3])) {
        s4Btn(1);
      }
      else if ((y >= bY1[4]) && (y <= bY2[4])) {
        s5Btn(1);
      }
      else if ((y >= bY1[5]) && (y <= bY2[5])) {
        s6Btn(1);
      }
    }

    // A) **** Resolve Button Taps
    else if ((x >= bX1[6]) && (x <= bX2[6])) {
      // Then on state line
      if ((y >= bY1[6]) && (y <= bY2[6])) {
        s7Btn(1);
      }
      else if ((y >= bY1[7]) && (y <= bY2[7])) {
        s8Btn(1);
      }
    }

    // A) **** Resolve Button Taps
    else if ((x >= 70) && (x <= 90)) {
      // Then on state line
      if ((y >= 30) && (y <= 90)) {
        s9Btn(1);
      }
      else if ((y >= 130) && (y <= 190)) {
        s10Btn(1);
      }
    }

    // A) **** Resolve Button Taps
    else if ((x >= 40) && (x <= 60)) {
      // Then on state line
      if ((y >= 30) && (y <= 90)) {
        s11Btn(1);
      }
      else if ((y >= 130) && (y <= 190)) {
        //        s12Btn(1);
      }
    }
  }
}

int flagReceive(char varAr[], int valAr[]) {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char endMarker = '>';
  char feedbackMarker = '<';
  char rc;
  int nVal;
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

        nVal = int(String(writeChar).toInt());
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



//int dashFlagReceive(char dVarAr[], uint32_t dValAr[]) {
//  static boolean dRecvInProgress = false;
//  static byte ddx = 0;
//  char dEndMarker = '>';
//  char dFeedbackMarker = '<';
//  char drc;
//  int dnVal;
//  const byte dNumChars = 32;
//  char dWriteChar[numChars];
//  int dSelectedVar = 0;
//  int dNewData = 0;
//
//  while (Serial2.available() > 0 && dNewData == 0) {
//    drc = Serial2.read();
//    if (dRecvInProgress == false) {
//      for ( int i = 0; i < knownDashCount; i++) {
//        if (drc == dVarAr[i]) {
//          dSelectedVar = i;
//          dRecvInProgress = true;
//        }
//      }
//    }
//
//    else if (dRecvInProgress == true) {
//      if (drc == dEndMarker ) {
//        dWriteChar[ddx] = '\0'; // terminate the string
//        dRecvInProgress = false;
//        ddx = 0;
//        dNewData = 1;
//
//        dnVal = int(String(dWriteChar).toInt());
//        dValAr[dSelectedVar] = dnVal;
//
//      }
//      else if (drc == dFeedbackMarker) {
//        dWriteChar[ddx] = '\0'; // terminate the string
//        dRecvInProgress = false;
//        ddx = 0;
//        dNewData = 1;
//        Serial2.print("echo");
//        Serial2.print(',');
//        Serial2.print(dVarAr[dSelectedVar]);
//        Serial2.print(',');
//        Serial2.print(dValAr[dSelectedVar]);
//        Serial2.print(',');
//        Serial2.println('~');
//      }
//
//      else if (drc != dFeedbackMarker || drc != dEndMarker) {
//        dWriteChar[ddx] = drc;
//        ndx++;
//        if (ndx >= numChars) {
//          ndx = numChars - 1;
//        }
//      }
//    }
//  }
//  return newData; // tells us if a valid variable arrived.
//}



uint32_t estimateVolume(int stepVol, int uRes, int tMuSteps) {
  float actualSteps = tMuSteps / uRes;
  uint32_t volDispensed = actualSteps * stepVol * 1000000;
  return volDispensed; // in nL
}

void checkForVarChange() {
  for ( int i = 0; i < knownCount; i++) {
    if (knownValues[i] != lastValues[i]) {
      varChanged[i] = 1;
    }
    else {
      varChanged[i] = 0;
    }

    if (varChanged[3] == 1) {
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 3);
      tft.print("vol (uL): ");
      tft.print(lastValues[3]);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 3);
      tft.print("vol (uL): ");
      tft.print(knownValues[4]);
    }
    lastValues[i] = knownValues[i];
  }
}

void primeMode(int mult) {

}

