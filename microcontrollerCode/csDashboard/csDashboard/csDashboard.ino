#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <Adafruit_GFX.h>
#include <Trinamic_TMC2130.h>
//https://github.com/makertum/Trinamic_TMC2130
#include <AccelStepper.h>


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
#define CS_PIN 27
#define EN_PIN 14
#define DIR_PIN 21
#define STEP_PIN 12

#define rewardPin 13


// make stepper(s) and screen
Trinamic_TMC2130 myStepper(CS_PIN);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

bool blockScreen = 0;






bool sHead[] = {0, 0, 0};
int stepsPerRev = 400;
char knownHeaders[] = {'w', 's', 'e', 'm', 'r'};
int knownValues[] = {0, 50, 1, 64, ((51200*3)*2)};
int knownCount = 5;

// motor variables
int stepRes = 256;
int microStepFactor = knownValues[3];
int totalStepsPerRev = stepsPerRev * stepRes;
int interpFullStep = totalStepsPerRev / microStepFactor;
int rwdVol = knownValues[4] * (interpFullStep / microStepFactor);
int totalDebugSteps = totalStepsPerRev / rwdVol;
int stepCounter;
int totalSteps = 0;
float curPosition = 0;

//float volPerStep = 2.23872607;
float volPerStep = 1.2647119363;
float volPerMuStep = volPerStep / microStepFactor;
float volDisp = 0;

int curScreen = 1;



// home state stuff
char home_knownHeaders[] = {'l', 'r', 't', 'h', 'z', 'y', 'w'};
int home_knownValues[] = {0, 0, 0, 0, 0, 0, 0};
int home_lastValues[] = {0, 0, 0, 0, 0, 0, 0};
int home_labCount[] = {5, 7, 6, 7, 5, 5, 8};
int home_knownCount = 7;


int rowBuf = 5;
int textScale = 1;
int textHeight = 10 * textScale;
int textWidth = 5 * textScale;

int dashState = 0;
int homeTimer = 0;
int homeRefreshTime = 1000;

// variables related to button rendering
int bWidth = 55;
int bHeight = 35;
int bBuf = 10;
int bTBuf = 5;
int bRow = 190;
float cur_lux = 0;
float last_lux = 0;



int dRwBtnSelected = 0;

char motor_knownHeaders[] = {'s', 'a', 'v', 'm'};
int motor_knownValues[] = {0, 0, 0, 0};
int motor_lastValues[] = {0, 0, 0, 0};
int motor_labCount[] = {7, 5, 8, 7};
int motor_knownCount = 4;

//HardwareSerial Serial1(2);


void setup(void) {
  initializeStepper();
  resetStepper();
  tft.begin();
  if (!ts.begin()) {
    Serial.println("Unable to start touchscreen.");
  }
  else {
    Serial.println("Touchscreen started.");
  }
  Serial.begin (115200);  // USB monitor
  Serial2.begin(115200);  // HW UART1
  delay(1000);
  digitalWrite(EN_PIN, HIGH); // disable stepper
  delay(10);



  dashState = 0;

  stepper.setMaxSpeed(interpFullStep * 1000);
  stepper.setSpeed(interpFullStep * 1000);
  stepper.setAcceleration(interpFullStep * 10);
}

void detectStateBody() {

  if (Serial.available()) {
    Serial2.write(Serial.read());
  }

  if (Serial2.available()) {
    Serial.write(Serial1.read());
  }

  if (dRwBtnSelected == 1) {
    Serial.println("on");
    dRwBtnSelected = 0;

  }
}

void loop() {

  int nD = flagReceive(knownHeaders, knownValues);
  if (nD > 0) {
    refreshScreen(curScreen);
  }
  pollMotorChange();
  bool giveReward = knownValues[0];//digitalRead(rewardPin);

  if (giveReward == 1) {
    knownValues[0] = 1;
  }



  if (knownValues[0] == 1) {
    //    digitalWrite(EN_PIN, LOW);
    dispReward(rwdVol);
    //    digitalWrite(EN_PIN, HIGH);
    knownValues[0] = 0;
  }

  if (blockScreen == 0) {
    // ***** Check for new interaction
    if (!ts.bufferEmpty()) {
      // Retrieve a point
      TS_Point p = ts.getPoint();
      // Scale using the calibration #'s
      // and rotate coordinate system
      Serial.println(p.x);
      p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
      p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
      int y = tft.height() - p.x;
      int x = p.y;

      // A) **** Resolve Button

      if ((x > 10) && (x <= 60) && (y > 180) && (y <= 200)) {
        dashState = 0;
      }
      else if ((x > 70) && (x <= 120) && (y > 180) && (y <= 200)) {
        dashState = 1;
      }
      else if ((x > 140) && (x <= 190) && (y > 180) && (y <= 200)) {
        dashState = 2;
      }
      //      else if ((x > 210) && (x <= 290) && (y > 60) && (y <= 90)) {
      ////        if ((updateServos == 0) && (resGalvoDebounce == 0)) {
      ////          resGalvoToggle = 1 - resGalvoToggle;
      ////          updateServos = 1;
      ////          s3Btn(resGalvoToggle);
      ////        }
      //      }
      //      else if ((x > 210) && (x <= 245) && (y > 20) && (y <= 50)) {
      //        bool ka = 1;
      //        }
      //      }
      //      else if ((x > 250) && (x <= 290) && (y > 20) && (y <= 50)) {
      //        bool ka = 1;
      //      }



      // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


      // B) **** State Specific Buttons
      if (dashState == 2) {
        if ((y > 30) && (y <= 50)) {
          if ((x > 5) && (x <= 55)) {
            dRwBtnSelected = 1;
          }
        }
      }
    }

    // C) **** Bounce/Debounce Buttons


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



      if (homeTimer > 100) {
        //        refreshHomeVars();
        homeTimer = 0;
      }
      homeTimer++;
    }

    // *********** S1: Motor State
    if (dashState == 1) {
      if (sHead[1] == 0) {
        sHead[0] = 0;
        sHead[1] = 1;
        sHead[2] = 0;
        createMotor();
        curScreen = 2;
      }


      //flagReceive(motor_knownHeaders, motor_knownValues, motor_knownCount);
      for ( int i = 0; i < motor_knownCount; i++) {
        if (motor_knownValues[i] != motor_lastValues[i]) {
          updateMotorDispVal(motor_labCount[i], i + 1, i);
        }
        motor_lastValues[i] = motor_knownValues[i];
      }
    }

    // *********** S2: Detection State
    if (dashState == 2) {
      if (sHead[2] == 0) {
        sHead[0] = 0;
        sHead[1] = 0;
        sHead[2] = 1;
        createDetect();
        curScreen = 3;
      }
      detectStateBody();
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

void updateMotorDispVal(int gap, int row, int valID) {
  tft.setCursor(gap * textWidth, (textHeight + rowBuf) * row);
  tft.setTextColor(ILI9341_BLACK);
  tft.print(motor_lastValues[valID]);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(gap * textWidth, (textHeight + rowBuf) * row);
  tft.print(home_knownValues[valID]);
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
  tft.setRotation(1);

  s0Btn(1);
  s1Btn(0);
  s2Btn(0);
  s3Btn(0);
  s4Btn(0);
  s5Btn(0);

  // a) SSID INFO
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(textScale);
  tft.print("ssid: Not Connected");

  tft.setCursor(0, (textHeight + rowBuf) * 1);
  tft.print("lux: ");
  last_lux = cur_lux;
  tft.print(last_lux);

  tft.setCursor(0, (textHeight + rowBuf) * 2);
  tft.print("range:");

  tft.setCursor(0, (textHeight + rowBuf) * 3);
  tft.print("temp:");

  tft.setCursor(0, (textHeight + rowBuf) * 4);
  tft.print("humid:");

  tft.setCursor(0, (textHeight + rowBuf) * 5);
  tft.print("voc: ");


  tft.setCursor(0, (textHeight + rowBuf) * 6);
  tft.print("co2: ");


  tft.setCursor(0, (textHeight + rowBuf) * 7);
  tft.print("weight:");
}

void createMotor() {
  //  int motorTextScale = 2;
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1);

  s0Btn(0);
  s1Btn(1);
  s2Btn(0);
  s3Btn(0);
  s4Btn(0);
  s5Btn(0);


  // a) SSID INFO
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(textScale);
  tft.print("ssid: Totes Moat");

  tft.setTextSize(textScale);
  tft.setCursor(0, (textHeight + rowBuf) * 1);
  tft.print("total steps: ");
  tft.print(totalSteps);

  tft.setCursor(0, (textHeight + rowBuf) * 2);
  tft.print("speed:");
  tft.print(knownValues[1]);

  tft.setCursor(0, (textHeight + rowBuf) * 3);
  tft.print("volume:");
  tft.setTextSize(textScale);
}

void createDetect() {

  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1);

  s0Btn(0);
  s1Btn(0);
  s2Btn(1);
  s3Btn(0);
  s4Btn(0);


  // a) SSID INFO
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(textScale);
  tft.print("ssid: Not Connected");

  tft.setCursor(0, (textHeight + rowBuf) * 1);
  tft.print("Running Detection Task: Relaying Serial Only");
  dRwBtn(0);
}

// Button Functions
void s0Btn(bool selState) {
  int bLoc = ((1 * bBuf) + (0 * bWidth));
  if (selState == 0) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("home");
  }
  else if (selState == 1) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_GREEN);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
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
    tft.setTextSize(1);
    tft.print("motor");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_GREEN);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("motor");
  }
}
void s2Btn(bool selState) {
  int bLoc = ((3 * bBuf) + (2 * bWidth));
  if (selState == 0) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("detect");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_GREEN);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("detect");
  }
}
void s3Btn(bool selState) {
  int bLoc = ((5 * bBuf) + (3 * bWidth));
  int tbRow = 60;
  int tbWidth = 95;
  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("pulse motor");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_BLUE);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("pulsing");

    knownValues[0] = 1;

  }
}
void s4Btn(bool selState) {
  int bLoc = ((5 * bBuf) + (3 * bWidth));
  int tbRow = 10;
  int tbWidth = 40;
  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("inc");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_BLUE);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("inc");
    int tlVal = knownValues[1];
    knownValues[1] = knownValues[1] + 5;
    Serial2.print('n');
    Serial2.print(1);
    Serial2.println('>');
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(0, (textHeight + rowBuf) * 2);
    tft.print("speed:");
    tft.print(tlVal);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(0, (textHeight + rowBuf) * 2);
    tft.print("speed:");
    tft.print(knownValues[1]);
  }
}
void s5Btn(bool selState) {
  int bLoc = ((10 * bBuf) + (3 * bWidth));
  int tbRow = 10;
  int tbWidth = 42;
  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("dec");
    Serial2.print('b');
    Serial2.print(5);
    Serial2.println('>');
  }
  if (selState == 1) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_BLUE);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("dec");
    Serial2.print('b');
    Serial2.print(50);
    Serial2.println('>');
    int tlVal = knownValues[1];
    knownValues[1] = knownValues[1] - 5;
    if (knownValues[1] < 1) {
      knownValues[1] = 1;
    }


    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(0, (textHeight + rowBuf) * 2);
    tft.print("speed:");
    tft.print(tlVal);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(0, (textHeight + rowBuf) * 2);
    tft.print("speed:");
    tft.print(knownValues[1]);

  }
}
void dRwBtn(bool selState) {
  int bLoc = 10;
  int bRow = 40;
  if (selState == 0) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_RED);
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("reward");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, bRow, bWidth, bHeight, ILI9341_GREEN);
    tft.setCursor(bLoc + bTBuf, bRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("reward");
  }
}

void dispReward(int numMuSteps) {


  digitalWrite(EN_PIN, LOW);
  long curPos = stepper.currentPosition();
  stepper.runToNewPosition(curPos + (numMuSteps));
  digitalWrite(EN_PIN, HIGH);


}






void refreshScreen(int targScreen) {
  if (targScreen == 1) {
    createHome();
  }
  else if (targScreen == 2) {
    createMotor();
  }
  else if (targScreen == 3) {
    createDetect();

  }
}

void pollMotorChange() {
  stepRes = 256;
  microStepFactor = knownValues[3];
  totalStepsPerRev = stepsPerRev * stepRes;
  interpFullStep = totalStepsPerRev / microStepFactor;
  rwdVol = knownValues[4] * (interpFullStep / microStepFactor);
  totalDebugSteps = totalStepsPerRev / rwdVol;
}

