#define pulsePin 23

elapsedMicros loopTime;

bool pulseState = 0;
bool pulseSwitched = 0;

int delayTime = 90;
int pulseTime = 10;

void setup() {
  pinMode(pulsePin, OUTPUT);
  digitalWrite(pulsePin, pulseState);
}

void loop() {
  loopTime = 0;

  if (pulseState == 0) {
    while (loopTime <= delayTime) {
      if (pulseSwitched == 0) {
        digitalWrite(pulsePin, LOW);
        pulseSwitched = 1;
      }
    }
    pulseSwitched = 0;
    pulseState = 1;
    loopTime = 0;
  }

  else if (pulseState == 1) {
    while (loopTime <= pulseTime) {
      if (pulseSwitched == 0) {
        digitalWrite(pulsePin, HIGH);
        pulseSwitched = 1;
      }
    }
    pulseSwitched = 0;
    pulseState = 0;
    loopTime = 0;
  }
}
