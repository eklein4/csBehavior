#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <Adafruit_GFX.h>
#include <Trinamic_TMC2130.h>
#include <AccelStepper.h>
#include <WiFi.h>

const char* ssid     = "CloudyBrain";
const char* password = "333stamp";

// Pass touchscreen calibration.
#define TS_MINX 150
#define TS_MAXY 130
#define TS_MAXX 3800
#define TS_MINY 4000

// Screen related pins
#define STMPE_CS 32
#define TFT_CS   15
#define TFT_DC   33

// stepper pins
#define CS_PIN 25
#define DIR_PIN 26
#define EN_PIN 21
#define STEP_PIN 4

#define rewardPin 13

#define rotateState 2


// make stepper(s) and screen
Trinamic_TMC2130 myStepper(CS_PIN);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);




// Dashboard State Stuff
bool blockScreen = 0;
bool sHead[] = {0, 0, 0};

// motor variables
int stepRes = 256;
int stepsPerRev = 200;
//200 for most steppers; 400 for some

char knownHeaders[] = {'r', 's', 'a', 'm', 'd'};
int knownValues[] = {0, 25600, 5000, 1600, 0};
int lastValues[] = {0, 25600, 5000, 1600, 0};
bool varChanged[] = {0, 0, 0, 0, 0};
int knownCount = 5;



uint32_t rwdVol;
uint32_t volPerStep = 2.239; // ul
float volDisp = 0;

int curScreen = 1;

// home state stuff
char home_knownHeaders[] = {'l', 'r', 't', 'h', 'z', 'y', 'w'};
int home_knownValues[] = {0, 0, 0, 0, 0, 0, 0};
int home_lastValues[] = {0, 0, 0, 0, 0, 0, 0};
int home_labCount[] = {5, 7, 6, 7, 5, 5, 8};
int home_knownCount = 7;

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
int bTBuf = 5;
int bRow = 290;
float cur_lux = 0;
float last_lux = 0;

int dRwBtnSelected = 0;
bool onWifi = 0;




void setup(void) {
  Serial.begin (115200);  // USB monitor
  delay(100);
  Serial2.begin(115200);  // HW UART1
  delay(100);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  initializeStepper();
  resetStepper();
  tft.begin();
  if (!ts.begin()) {
    Serial.println("Unable to start touchscreen.");
  }
  else {
    Serial.println("Touchscreen started.");
  }

  digitalWrite(EN_PIN, HIGH); // disable stepper
  delay(10);
  dashState = 0;
}



void loop() {
  checkBounceStates();
  //  rwdVol = estimateVolume(volPerStep, 256, knownValues[3]);
  //  Serial.println(rwdVol);
  int nD = flagReceive(knownHeaders, knownValues);
  checkForVarChange();
  bool giveReward = digitalRead(rewardPin);

  if ((knownValues[0] == 1) || (giveReward == 1)) {
    dispReward(knownValues[3]);
    knownValues[0] = 0;
  }

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

    else if (dashState == 1) {
      if (sHead[1] == 0) {
        sHead[0] = 0;
        sHead[1] = 1;
        sHead[2] = 0;
        createMotor();
        curScreen = 2;
        homeTimer = 0;
      }
    }

  }
}




void updateHomeDispVal(int gap, int row, int valID) {
  tft.setCursor(gap * textWidth, (textHeight + rowBuf) * row);
  tft.setTextColor(ILI9341_BLACK);
  tft.print(home_lastValues[valID]);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(gap * textWidth, (textHeight + rowBuf) * row);
  tft.print(home_knownValues[valID]);
}
//
//void updateMotorDispVal(int gap, int row, int valID) {
//  tft.setCursor(gap * textWidth, (textHeight + rowBuf) * row);
//  tft.setTextColor(ILI9341_BLACK);
//  tft.print(motor_lastValues[valID]);
//  tft.setTextColor(ILI9341_RED);
//  tft.setCursor(gap * textWidth, (textHeight + rowBuf) * row);
//  tft.print(home_knownValues[valID]);
//}


void stepSPI(int stepDelay) {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(stepDelay);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(stepDelay);
}


void resetStepper() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH); // disable driver
  digitalWrite(DIR_PIN, LOW); // chose direction
  digitalWrite(STEP_PIN, LOW); // no step yet
}

void initializeStepper() {
  myStepper.init();
  myStepper.set_mres(stepRes); // ({1,2,4,8,16,32,64,128,256}) number of microsteps
  myStepper.set_IHOLD_IRUN(31, 31, 5); // ([0-31],[0-31],[0-5]) sets all currents to maximum
  myStepper.set_I_scale_analog(1); // ({0,1}) 0: I_REF internal, 1: sets I_REF to AIN
  myStepper.set_tbl(1); // ([0-3]) set comparator blank time to 16, 24, 36 or 54 clocks, 1 or 2 is recommended
  myStepper.set_toff(8); // ([0-15]) 0: driver disable, 1: use only with TBL>2, 2-15: off time setting during slow decay phase
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


  tft.setCursor(0, (textHeight + rowBuf) * 1);
  tft.print("temp: ");
  tft.setCursor(0, (textHeight + rowBuf) * 2);
  tft.print("humid: ");
  tft.setCursor(0, (textHeight + rowBuf) * 3);
  tft.print("direction: ");
  if (knownValues[4] == 0) {
    tft.print("forward");
  }
  else if (knownValues[4] == 1) {
    tft.print("backward");
  }
  tft.setCursor(0, (textHeight + rowBuf) * 4);
  tft.print("steps: ");
  tft.print(knownValues[3]);
  tft.setCursor(0, (textHeight + rowBuf) * 5);
  tft.print("motor speed: ");
  tft.print(knownValues[1]);
  tft.setCursor(0, (textHeight + rowBuf) * 6);
  tft.print("motor acel: ");
  tft.print(knownValues[2]);
}

void createMotor() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(rotateState);

  s0Btn(0);
  s1Btn(1);
  s2Btn(0);
  s4Btn(0);
  s5Btn(0);


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


  tft.setTextSize(textScale);
  tft.setCursor(0, (textHeight + rowBuf) * 1);
  tft.print("total steps: ");


  tft.setCursor(0, (textHeight + rowBuf) * 2);
  tft.print("speed:");
  tft.print(knownValues[1]);

  tft.setCursor(0, (textHeight + rowBuf) * 3);
  tft.print("volume:");
  tft.setTextSize(textScale);
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
    tft.setTextSize(1);
    tft.print(" ~ ");
  }
}


void s4Btn(bool selState) {
  int bLoc = ((1 * bBuf) + (0 * bWidth));
  int tbRow = 110;
  int tbHeight = 30;

  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("speed +");
  }
  if (selState == 1) {
    if (buttonBounceCounter[0] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(1);
      tft.print("speed +");
      int tlVal = knownValues[1];
      knownValues[1] = knownValues[1] + 100;
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 5);
      tft.print("motor speed: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 5);
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
    tft.setTextSize(1);
    tft.print("speed -");
  }
  if (selState == 1) {
    if (buttonBounceCounter[1] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(1);
      tft.print("speed -");
      int tlVal = knownValues[1];
      knownValues[1] = knownValues[1] - 100;
      if (knownValues[1] <= 0) {
        knownValues[1] = 1;
      }
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 5);
      tft.print("motor speed: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 5);
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
    tft.setTextSize(1);
    tft.print("reward");
  }
  if (selState == 1) {
    if (buttonBounceCounter[2] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(1);
      tft.print("rewarding");
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
    tft.setTextSize(1);
    tft.print("acel +");
  }
  if (selState == 1) {
    if (buttonBounceCounter[3] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(1);
      tft.print("acel +");
      int tlVal = knownValues[2];
      knownValues[2] = knownValues[2] + 100;
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 6);
      tft.print("motor acel: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 6);
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
    tft.setTextSize(1);
    tft.print("acel -");
  }
  if (selState == 1) {
    if (buttonBounceCounter[4] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(1);
      tft.print("acel +");
      int tlVal = knownValues[2];
      knownValues[2] = knownValues[2] - 100;
      if (knownValues[2] <= 0) {
        knownValues[2] = 1;
      }
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 6);
      tft.print("motor acel: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 6);
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
    tft.setTextSize(1);
    tft.print("steps +");
  }
  if (selState == 1) {
    if (buttonBounceCounter[5] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(1);
      tft.print("steps +");
      int tlVal = knownValues[3];
      knownValues[3] = knownValues[3] + 16;
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("steps: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("steps: ");
      tft.print(knownValues[3]);
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
    tft.setTextSize(1);
    tft.print("steps -");
  }
  if (selState == 1) {
    if (buttonBounceCounter[6] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(1);
      tft.print("steps -");
      int tlVal = knownValues[3];
      knownValues[3] = knownValues[3] - 16;
      if (knownValues[3] <= 0) {
        knownValues[3] = 16;
      }
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("steps: ");
      tft.print(tlVal);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("steps: ");
      tft.print(knownValues[3]);
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
    tft.setTextSize(1);
    tft.print("change dir");
  }
  if (selState == 1) {
    if (buttonBounceCounter[7] == 0) {
      tft.fillRect(bLoc, tbRow, bWidth, tbHeight, ILI9341_BLUE);
      // col,row || width, height
      tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(1);
      int tlVal = knownValues[4];
      tft.setTextColor(ILI9341_BLACK);
      tft.setCursor(0, (textHeight + rowBuf) * 3);
      tft.print("direction: ");
      if (tlVal == 0) {
        tft.print("forward");
      }
      else if (tlVal == 1) {
        tft.print("backward");
      }
      knownValues[4] = 1 - knownValues[4];
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 3);
      tft.print("direction: ");
      if (knownValues[4] == 0) {
        tft.print("forward");
      }
      else if (knownValues[4] == 1) {
        tft.print("backward");
      }
      buttonBounceCounter[7] = buttonBounceCounter[7] + 1;
      delay(100);
      s11Btn(0);
    }
  }
}





void dispReward(int numMuSteps) {
  int dirMult;
  if (knownValues[4] == 0) {
    dirMult = 1;
  }
  else if (knownValues[4] == 1) {
    dirMult = -1;
  }
  stepper.setMaxSpeed(stepRes * knownValues[1]);
  stepper.setSpeed(stepRes * knownValues[1]);
  stepper.setAcceleration(stepRes * knownValues[2]);
  digitalWrite(EN_PIN, LOW);
  long curPos = stepper.currentPosition();
  stepper.runToNewPosition(curPos + (dirMult * numMuSteps));
  digitalWrite(EN_PIN, HIGH);


}

void refreshScreen(int targScreen) {
  if (targScreen == 1) {
    createHome();
  }
  else if (targScreen == 2) {
    createMotor();
  }
}


void resolveTaps() {

  if (!ts.bufferEmpty()) {
    // Retrieve a point
    TS_Point p = ts.getPoint();
    // Scale using the calibration #'s
    // and rotate coordinate system
    p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
    p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
    int y = tft.height() - p.x;
    int x = p.y;
    //    Serial.print(x);
    //    Serial.print(',');
    //    Serial.println(y);

    // A) **** Resolve Button Taps
    if ((x >= 15) && (x <= 30)) {
      // Then on state line
      if ((y >= 30) && (y <= 90)) {
        dashState = 0;
      }
      else if ((y >= 130) && (y <= 190)) {
        // dashState = 1;
      }
      else if ((y >= 220) && (y <= 290)) {
        // dashState = 2;
      }
    }

    // A) **** Resolve Button Taps
    else if ((x >= 125) && (x <= 150)) {
      // Then on state line
      if ((y >= 30) && (y <= 90)) {
        s4Btn(1);
      }
      else if ((y >= 130) && (y <= 190)) {
        s5Btn(1);
      }
      else if ((y >= 220) && (y <= 290)) {
        s6Btn(1);
      }
    }

    // A) **** Resolve Button Taps
    else if ((x >= 100) && (x <= 120)) {
      // Then on state line
      if ((y >= 30) && (y <= 90)) {
        s7Btn(1);
      }
      else if ((y >= 130) && (y <= 190)) {
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


uint32_t estimateVolume(int stepVol, int uRes, int tMuSteps) {
  float actualSteps = tMuSteps / uRes;
  uint32_t volDispensed = actualSteps * stepVol * 1000;
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
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("steps: ");
      tft.print(lastValues[3]);
      tft.setTextColor(ILI9341_RED);
      tft.setCursor(0, (textHeight + rowBuf) * 4);
      tft.print("steps: ");
      tft.print(knownValues[3]);
    }
    lastValues[i] = knownValues[i];
  }
}

void primeMode(int mult){
  
}

