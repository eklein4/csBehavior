/* ~~~~ Teensey Analog Cap Relay ~~~~
   Notes: Teensey 3.6 has two built in 12 bit dacs.
   Teensey(s) LC,3.1/2, 3.5 have built in cap sensing.
   The cap sensing is awesome! But, it is blocking. 
   So, I sense on a 3.6 teensy and use its nice dacs to stream analog version of the data.
   This assumes you are using a 3.6 and wanting to convert two cap reads to an anlaog signal.
   v1.0
   cdeister@brown.edu
*/

const int capSensPinL = 23;
int lickValA = 0;


void setup() {
  Serial.begin(9600);
}

void loop() {
  // read, map teensey vals into 
  lickValA = touchRead(capSensPinL);
  Serial.println(lickValA);
  delay(10);
}
