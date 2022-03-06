#define USE_ARDUINO_INTERRUPTS true
#include <PulseSensorPlayground.h>
//#include <ArduinoQueue.h>

const int PULSE_INPUT = A0;   // ANALOG PIN 0
const int PULSE_BLINK = 13;     // LED by PIN 13
int THRESHOLD = 550;    // Threshold for beat count

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

}

void loop() {

  int myBPM = pulseSensor.getBeatsPerMinute();
  if (pulseSensor.sawStartOfBeat()) {
    statistic(myBPM);
    levelOfActivity(myBPM);
  }
  delay(10);
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
  if (millis() % 10 == 1) { return; }
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
