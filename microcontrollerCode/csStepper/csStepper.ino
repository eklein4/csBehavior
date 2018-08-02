// csStepper
// uses Moritz Walter's SPI tool to configure the 2130s
//
//
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Trinamic_TMC2130.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>

// stepper pins
#define CS_PIN 27
#define EN_PIN 14 //enable (CFG6)
#define DIR_PIN 12 //direction
#define STEP_PIN 21 //step

// screen pins
#define STMPE_CS 12
#define TFT_CS 39
#define TFT_DC 33

int stepRes = 256;
int stepCounter;
int totalSteps = 0;
float curPosition = 0;
int val;


// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

bool resGalvoToggle = 0; // 0 is galvo and 1 is resonant
bool updateServos = 0;
int resGalvoDebounce;

Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// make a stepper
Trinamic_TMC2130 myStepper(CS_PIN);
char knownHeaders[] = {'r', 's', 'c', 'e', 'd'};
int knownValues[] = {0, 20, 400 * stepRes, 1, 0};
int knownCount = 5;

bool sHead[] = {0, 0, 0};

int rowBuf = 5;
int textScale = 1;
int textHeight = 10 * textScale;
int textWidth = 5 * textScale;

bool dashState = 0;

// variables related to button rendering
int bWidth = 55;
int bHeight = 35;
int bBuf = 10;
int bTBuf = 5;
int bRow = 190;


// ******** Menu Buttons
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
    tft.print("galvo");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_BLUE);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("resonant");
  }
}

void s4Btn(bool selState) {
  int bLoc = ((5 * bBuf) + (3 * bWidth));
  int tbRow = 10;
  int tbWidth = 95;
  if (selState == 0) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_RED);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("galvo");
  }
  if (selState == 1) {
    tft.fillRect(bLoc, tbRow, tbWidth, bHeight, ILI9341_BLUE);
    // col,row || width, height
    tft.setCursor(bLoc + bTBuf, tbRow + bTBuf);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(1);
    tft.print("resonant");
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


void setup() {
  resetStepper();
  Serial.begin(9600);
  tft.begin();
  // stepper object
  myStepper.init();
  myStepper.set_mres(stepRes); // ({1,2,4,8,16,32,64,128,256}) number of microsteps
  myStepper.set_IHOLD_IRUN(31, 31, 5); // ([0-31],[0-31],[0-5]) sets all currents to maximum
  myStepper.set_I_scale_analog(1); // ({0,1}) 0: I_REF internal, 1: sets I_REF to AIN
  myStepper.set_tbl(1); // ([0-3]) set comparator blank time to 16, 24, 36 or 54 clocks, 1 or 2 is recommended
  myStepper.set_toff(8); // ([0-15]) 0: driver disable, 1: use only with TBL>2, 2-15: off time setting during slow decay phase
  digitalWrite(EN_PIN, LOW); // enable driver
  delay(100);
  //  createHome();
}

void createHome() {

  tft.fillScreen(ILI9341_BLACK); //ILI9341_BLACK
//  tft.setRotation(1);

s0Btn(1);
//  ////  s1Btn(0);
//  ////  s2Btn(0);
//  //
//  //
//  ////  // a) SSID INFO
//  ////  tft.setCursor(0, 0);
//  ////  tft.setTextColor(ILI9341_RED);
//  ////  tft.setTextSize(textScale);
//  ////  tft.print("ssid: Not Connected");
//  //
//  ////  tft.setCursor(0, (textHeight + rowBuf) * 1);
//  ////  tft.print("lux:");
//  ////
//  ////
//  ////  tft.setCursor(0, (textHeight + rowBuf) * 2);
//  ////  tft.print("range:");
//  ////
//  ////  tft.setCursor(0, (textHeight + rowBuf) * 3);
//  ////  tft.print("temp:");
//  ////
//  ////  tft.setCursor(0, (textHeight + rowBuf) * 4);
//  ////  tft.print("humid:");
//  ////
//  ////  tft.setCursor(0, (textHeight + rowBuf) * 5);
//  ////  tft.print("voc:");
//  ////
//  ////  tft.setCursor(0, (textHeight + rowBuf) * 6);
//  ////  tft.print("co2:");
//  ////
//  ////  tft.setCursor(0, (textHeight + rowBuf) * 7);
  ////  tft.print("weight:");
}

void loop() {
  if (dashState == 0) {
    createHome();
    dashState = 1;
  }
  flagReceive(knownHeaders, knownValues);
  if (knownValues[0] == 1) {
    digitalWrite(DIR_PIN, knownValues[4]);
    digitalWrite(EN_PIN, LOW);
    if (stepCounter <= knownValues[2]) {
      stepSPI(knownValues[1]);
    }
    else if (stepCounter > knownValues[2]) {
      digitalWrite(EN_PIN, HIGH);
      knownValues[0] = 0;
      stepCounter = 0;
    }
    stepCounter++;
  }
}

void resetStepper() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH); // disable driver
  digitalWrite(DIR_PIN, knownValues[4]); // chose direction
  digitalWrite(STEP_PIN, LOW); // no step yet
}

void stepSPI(int stepDelay) {

  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(stepDelay);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(stepDelay);

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





