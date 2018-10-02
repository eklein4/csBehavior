

elapsedMicros loopTime;

bool pulseState = 0;
bool pulseSwitched = 0;

int lineTime = 100;
int flybackTime = 10;
int rampInc = 4095 / lineTime;
int downInc = 4095 / flybackTime;
int stimAmp = 0;
int gatePause = 2;

void setup() {
  analogWriteResolution(12);
  pinMode(23,OUTPUT);
  digitalWrite(23,LOW);

}

void loop() {


  if (pulseSwitched == 0) {
    stimAmp = stimAmp + rampInc;
    if (stimAmp >= 4095) {
      stimAmp = 4095;
      pulseSwitched = 1;
    }
  }
  if (pulseSwitched == 1) {
    stimAmp = stimAmp - downInc;
    if (stimAmp < 0) {
      stimAmp = 0;
      pulseSwitched = 0;
    }
  }
  analogWrite(A14, stimAmp);
  delayMicroseconds(1);

  if ((stimAmp>=2000) && (gatePause==0)){
    digitalWrite(23,HIGH);
    delayMicroseconds(200);
    digitalWrite(23,LOW);
    gatePause=1;
    loopTime=0;
  }

  if ((loopTime >=1000) && (gatePause==1)){
    gatePause=0;
  }

}
