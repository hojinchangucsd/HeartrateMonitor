#define USE_ARDUINO_INTERRUPTS true
#include <PulseSensorPlayground.h>
#include "SevSegShift.h"

#define SHIFT_PIN_DS   8 /* Data input PIN */
#define SHIFT_PIN_STCP 7 /* Shift Register Storage PIN */
#define SHIFT_PIN_SHCP 6 /* Shift Register Shift PIN */

//Instantiate a seven segment controller object (with Shift Register functionality)
SevSegShift sss(
                  SHIFT_PIN_DS, 
                  SHIFT_PIN_SHCP, 
                  SHIFT_PIN_STCP, 
                  1, /* number of shift registers there is only 1 Shiftregister 
                        used for all Segments (digits are on Controller)
                        default value = 2 (see SevSegShift example)
                        */
                  true /* Digits are connected to Arduino directly 
                          default value = false (see SevSegShift example)
                        */
                );
                
const int PULSE_INPUT = A0;   // ANALOG PIN 0
const int PULSE_BLINK = 13;     // LED by PIN 13
int THRESHOLD = 700;    // Threshold for beat count

// STATISTIC VARIABLES
const int intervalRecSize = 10;
int beatIntervals[intervalRecSize];
int addIndex = 0;

const int intervalRecReadSize = 5;
int lastNIntervals[intervalRecReadSize];

int instabilityThreshold = 0;

int lastBeatTime = 0;

const int varianceGradientSize = intervalRecReadSize;
int varianceGradient[varianceGradientSize];

// LEVEL OF ACTIVITY VARIABLES

int Age = 21;         // CHANGE THIS TO USER'S AGE
int MaxBPM = 220 - Age;

int tooLow = 15;
int lowestModerate = 0.64 * MaxBPM;
int lowestVigorous = 0.77 * MaxBPM;
int tooHigh = 0.94 * MaxBPM;

const int RED_PIN = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN = 11;

int displayBPM = 0; // bpm to display (not to calculate other statistics)

PulseSensorPlayground pulseSensor;

void setup() {

  Serial.begin(9600);

  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.setThreshold(THRESHOLD);
  pulseSensor.blinkOnPulse(PULSE_BLINK);

  if (pulseSensor.begin()) {
    Serial.println("pulseSensor initialized");
  }

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  lastBeatTime = pulseSensor.getLastBeatTime();

  //unsigned long prevTimeMillis = millis();
  //std::list<unsigned long> intervals;

  // Setup LED segment display
  byte numDigits = 4;
  byte digitPins[] = {5, 4, 3, 2}; // These are the PINS of the ** Arduino **
  byte segmentPins[] = {0, 2, 4, 6, 7, 1, 3, 5}; // these are the PINs of the ** Shift register **
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_CATHODE; // See README.md for options
  bool updateWithDelays = false; // Default 'false' is Recommended
  bool leadingZeros = false; // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false; // Use 'true' if your decimal point doesn't exist or isn't connected. Then, you only need to specify 7 segmentPins[]

  sss.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
  updateWithDelays, leadingZeros, disableDecPoint);
  sss.setBrightness(100);
}

void loop() {

  int myBPM = pulseSensor.getBeatsPerMinute();
    if (pulseSensor.sawStartOfBeat()) {
      statistic(myBPM);
      levelOfActivity(myBPM);
      displayBPM = myBPM;
    }

  sss.setNumber(displayBPM);
  sss.refreshDisplay();
}

void statistic(int myBPM)
{
  //int interpulseInterval = pulseSensor.getLastBeatTime() - lastBeatTime; // BROKEN: randomly doubles at certain measures
  int interpulseInterval = pulseSensor.getInterBeatIntervalMs();
    /*if (interpulseInterval >= 0) {
      beatIntervals[addIndex] = interpulseInterval;
      // Serial.println("negative value captured");
    }*/

    beatIntervals[addIndex] = pulseSensor.getInterBeatIntervalMs();

    Serial.print("Interval:");
    Serial.println(interpulseInterval);
    //Serial.println(pulseSensor.getInterBeatIntervalMs());
    Serial.print("BPM:");
    Serial.println(myBPM);
    
    lastBeatTime = pulseSensor.getLastBeatTime();
    
    addIndex++;
    if (addIndex == intervalRecSize) { addIndex = 0; }

    // gather the indices of the N most recent captures, considering mod N
    for (int i = 0; i < intervalRecReadSize; i++) {
      int indexToRead = addIndex - i;
      if (indexToRead < 0) {
        indexToRead += intervalRecSize;
      }
      lastNIntervals[i] = indexToRead;
    }

    double intervalAvg;
    double intervalStdDev;
  
    int sum = 0;
    for (int i = 0; i < intervalRecReadSize; i++) {
      sum += beatIntervals[lastNIntervals[i]];
    }
    intervalAvg = sum / (double) intervalRecReadSize;
  
    double varianceN = 0;
    for (int i = 0; i < intervalRecReadSize; i++) {
      //Serial.print("in for: ");
      //Serial.println(lastNIntervals[i]);
      //Serial.println(beatIntervals[lastNIntervals[i]]);
      double contrib = sq((beatIntervals[lastNIntervals[i]] - intervalAvg));
      varianceN += contrib;
    }
    intervalStdDev = sqrt(varianceN / (intervalRecReadSize));
    Serial.print("avg:");
    Serial.println(intervalAvg);
    Serial.print("std:");
    Serial.println(intervalStdDev);

    double zScore = (interpulseInterval - intervalAvg) / intervalStdDev;
    //Serial.print("cur:");
    //Serial.println(interpulseInterval);
    Serial.print("zsc:");
    Serial.println(zScore);
}

void levelOfActivity(int myBPM)
{
  if (myBPM < tooLow) {
    Serial.println("No measured heart rate.");
    rgb(255, 255, 255); // white
  }
  else if (myBPM >= tooLow && myBPM < lowestModerate) {
    Serial.println("Resting heart rate.");
    rgb(255, 255, 0); // yellow
  }
  else if (myBPM >= lowestModerate && myBPM < lowestVigorous) {
    Serial.println("Moderate physical activity.");
    rgb(0, 255, 0); // green
  }
  else if (myBPM >= lowestVigorous && myBPM < tooHigh) {
    Serial.println("Vigorous physical activity.");
    rgb(0, 0, 255); // blue
  }
  else {
    Serial.println("Heart rate too high!");
    rgb(255, 0, 0); // red
  }
}

void rgb(int r, int g, int b) {
  analogWrite(RED_PIN, 255 - r);
  analogWrite(GREEN_PIN, 255 - g);
  analogWrite(BLUE_PIN, 255 - b);
}
