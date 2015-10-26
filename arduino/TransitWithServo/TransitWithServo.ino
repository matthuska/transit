/* 
Visualize BVG Train with a physical train.

For use with the Adafruit Motor Shield v2 
---->	http://www.adafruit.com/products/1438
*/

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"
#include <Servo.h> 


// Servo steup
Servo myservo;  // create servo object to control a servo 
                // twelve servo objects can be created on most boards
 
int servoPos = 0;    // variable to store the servo position 
 
// ---------------------------------------------------------------------
// Motor setup

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
int stepperSteps = 200;
Adafruit_StepperMotor *myMotor = AFMS.getStepper(stepperSteps, 2);

// Servo
//Adafruit_StepperMotor *myMotor = AFMS.getStepper(stepperSteps, 2);

// ---------------------------------------------------------------------

// Are we debugging?
boolean debugging = false;
boolean logging = false;

// Calibration settings
float distancePerRotation = 110.f; // in mm
float distancePerStep = distancePerRotation / (stepperSteps * 2); // *2 because we are using INTERLEAVE
unsigned int motorRPM = 1;

// Start and end of the visible track region in mm
float trackStart = 0.f;
float trackEnd = 400.f; // 40 cm
float trackLength = trackEnd - trackStart;
// Station is 90% of the way along the track
float stationLocation = 0.81f;
float distanceTooLate = 0.45f; // At what distance do we flip the sign 

// TRAIN INFO
int trainID = 0;
// Current train location in our track
float trainLastLoc = 0.f;
//unsigned long trainLastTime = millis(); // in milliseconds
unsigned long trainLastTime = 0; // in milliseconds

// New location read in from the computer
float trainNewLoc = 0.f;
unsigned long trainNewTime = 0; // in milliseconds

// Distance to move the train
float dx = 0.f;
float dt = 0.f;

// Distance we couldn't move in a full step in the last iteration (in fractional steps)
float leftOver = 0.f;

//// How often to poll for new data (in Hz)
//float pollInterval = 4.f;

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  //Serial.println("Stepper test!");

  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(60);  // OR with a different frequency, say 1KHz

  // Attach a servo to pin #10 (Servo 1 on the motor shield)
  myservo.attach(10);
  myservo.write(servoPos); // Move to 0 at beginning
}

//// Calibrate the system to the length of our track and speed of the stepper motor
//void calibrate() {
//  distancePerStep = 10;
//}

// Read the new train location from serial
boolean updateLocation() {
  boolean updated = false;
  while (Serial.available()) {
    // Input is 1000 - 0, where 0 is arrival at the station
    trainNewLoc = 1.f - (Serial.parseInt() / 1000.f);
    trainNewTime = Serial.parseInt();
    //Serial.parseInt();
    //trainNewTime = millis();
    trainID = Serial.parseInt();

    if (Serial.read() == '\n') {
      if (trainLastTime == 0) {
        trainLastTime = trainNewTime;
        trainLastLoc = trainNewLoc;
      } else {
        updated = true;
      }
    }
  }
  return(updated);
}

// Current RPM value is too damn fast, making a custom version that will hopefully be slower
void slowStep(int steps, unsigned int mdt) {
  myMotor->setSpeed(1);
  int stepsPerIter = 1;
  unsigned int loops = steps / stepsPerIter;
  unsigned int delayTime = mdt / loops;
  for (int i = 0; i < loops; i++) {
    //myMotor->step(stepsPerIter, FORWARD, INTERLEAVE);
   // myMotor->onestep(FORWARD, INTERLEAVE);
    myMotor->onestep(BACKWARD, INTERLEAVE);
    delay(delayTime);
  }
}

// Total time (~1 mins)
void fakeUpdate() {
  // Runtime of our fake experiment (for a run of the whole length of the track)
  float runtimeInMins = 0.5f;
//  long int fakeUpdateInterval = 1000; // 0.25 seconds in millis
  long int fakeUpdateInterval = 1000; // update every 1 second
  
  long int numUpdates = (runtimeInMins * 60 * 1000) / fakeUpdateInterval;

  if (logging) {
//  Serial.println("--numupdates--");
//  Serial.println(numUpdates);
  }
  
  trainNewLoc = trainLastLoc + (1.0f / numUpdates);

  // in milliseconds
  trainNewTime = trainLastTime + fakeUpdateInterval;
}


void moveTrain() {
  dx = trainNewLoc - trainLastLoc;
  dt = trainNewTime - trainLastTime;

  if (logging) {
    Serial.println(dx);
    Serial.println(dt);
    Serial.println("---");
  }

  if (trainNewLoc >= 1.0) {
    chillWereDone();
    return;
  }
  
  // skip this if we don't want to move
  if (dx == 0) {
    return;
  }

  // New location is < old location, we need to get a new train
  if (dx < 0) {
    return; // for now do nothing
//    resetTrain()
//    dx = trainNewLoc;
  }

  // tell motor to move
  float distanceInMM = (dx * trackLength);
  float distanceInSteps = (distanceInMM / distancePerStep) + leftOver;
  int fullSteps = floor(distanceInSteps);
  leftOver = distanceInSteps - fullSteps;

  // set speed in RPM
  float revolutions = distanceInMM / distancePerRotation;
  float minutes = dt / (60 * 1000.f);
//  motorRPM = revolutions / minutes;  // 10 rpm  
//  motorRPM = 1;
  
  if (logging) {
    Serial.println("Distance in steps: ");
    Serial.println(distanceInSteps);
    Serial.println("Distance in mm: ");
    Serial.println(distanceInMM);
    Serial.println("Revolutions: ");
    Serial.println(revolutions); 
  //  Serial.println(dx * 10000.f);
  //  Serial.println(trackLength * 10000.f);
  //  Serial.println(distancePerStep * 10000.f);
    Serial.println("RPM");
    Serial.println(motorRPM);
    Serial.println(minutes);
    Serial.println("-------------------");
  }
  
  //myMotor->setSpeed(motorRPM);
  //myMotor->step(distanceInSteps, FORWARD, INTERLEAVE);

  //myMotor->setSpeed(1);
  slowStep(fullSteps, dt);
  //slowStep(distanceInSteps, dt);
}

void printState() {
  if (!logging) {
    return;
  }
  
  Serial.println("Last:");
  Serial.println(trainLastLoc);
  Serial.println(trainLastLoc * trackLength);
  Serial.println(trainLastTime);

  Serial.println("New:");
  Serial.println(trainNewLoc);
  Serial.println(trainNewLoc * trackLength);
  Serial.println(trainNewTime);
  Serial.println("------");
}

void moveSign() {
  if (trainLastLoc < distanceTooLate && trainNewLoc >= distanceTooLate && servoPos != 180) {
    myservo.write(180);
    servoPos = 180;
//    for(pos = 0; pos <= 180; pos += 1) // goes from 0 degrees to 180 degrees 
//    {                                  // in steps of 1 degree 
//      myservo.write(pos);              // tell servo to go to position in variable 'pos' 
//      delay(15);                       // waits 15ms for the servo to reach the position 
//    } 
  }
}

// Release the motor and then sleep when we're done
void chillWereDone() {
  myMotor->release();
  while (true) {
    delay(1000);
  }
}
//void oneRev() {
//  //myMotor->setSpeed(0.01);
//  myMotor->step(stepperSteps * 2, FORWARD, INTERLEAVE);
//  delay(1000);
//  myMotor->step(stepperSteps, FORWARD, SINGLE);
//  delay(1000);
//  myMotor->step(stepperSteps, FORWARD, DOUBLE);
//  delay(5000);
//}
unsigned long loopStart;
unsigned long looptime;
void loop() {

  // find out how far we move in one revolution of the motor
  //oneRev();

  loopStart = millis();
  // Update state of train (read from serial?)
  if (debugging) {
    fakeUpdate();
  } else {
    updateLocation();
  }

  printState();
  
  // Calculate difference between current state and new state
  //updateSpeed();
  
  // Update toy train location to the new location
  moveTrain();
  moveSign();

  // Update the state of the system
  if (trainLastTime != trainNewTime) {
    trainLastLoc = trainNewLoc;
    trainLastTime = trainNewTime;
  }

//  // Huge sleep if we think we are at a new train
//  if (trainLastLoc > (trainNewLoc + 0.2)) {
//    delay(100000);
//  }

  looptime = millis() - loopStart;
  
  delay(max(0, dt - looptime));
}

// Motor stuff

//  Serial.println("Double coil steps");
//  myMotor->step(100, FORWARD, DOUBLE); 
//  myMotor->step(100, BACKWARD, DOUBLE);
//  
//  Serial.println("Interleave coil steps");
//  myMotor->step(100, FORWARD, INTERLEAVE); 
//  myMotor->step(100, BACKWARD, INTERLEAVE); 
//  
//  Serial.println("Microstep steps");
//  myMotor->step(50, FORWARD, MICROSTEP); 
//  myMotor->step(50, BACKWARD, MICROSTEP);
