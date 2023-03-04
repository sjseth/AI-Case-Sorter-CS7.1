/// VERSION CS 7.1.230303.1 ///

#include <Wire.h>
#include <SoftwareSerial.h>

//ARDUINO UNO WITH 4 MOTOR CONTROLLER
//Stepper controller is set to 16 Microsteps (3 jumpers in place)
#define FEED_DIRPIN 5
#define FEED_STEPPIN 2
#define FEED_Enable 8
#define FEED_MICROSTEPS 16
#define FEED_HOMING_SENSOR 10
#define FEED_SENSOR 9

#define SORT_MICROSTEPS 16
#define SORT_DIRPIN 6
#define SORT_STEPPIN 3
#define SORT_Enable 8
#define SORT_HOMING_SENSOR 11

#define SORTER_CHUTE_SEPERATION 20  //number of steps between chutes
#define QUEUE_LENGTH 2              //usually 1 but it is the positional distance between your camera and the sorter.
#define PRINT_QUEUEINFO false       //used for debugging in serial monitor. prints queue info
#define FEEDSENSOR_ENABLED true     //enabled if feedsensor is installed and working;//this is a proximity sensor under the feed tube which tells us a case has dropped completely

#define FEED_DONE_SIGNAL 3     // Writes HIGH Signal When Feed is done
int feedDoneSignalTime = 0;  //The amount of time in MS to send the feed done signal;

bool autoFeedHoming = true;  //if true, then homing will be checked and adjusted on each feed cycle. Requires homing sensor.
int homingFeedOffset = 4;
int homingSortOffset = 2;
int slotDropDelay = 250;
bool autoSorterHoming = false;

bool homeSorterOnStartup = false;
bool homeFeedOnStartup = false;

//acceleration settings for sorter motor
bool useAcceleration = true;
int upslope = 2 * SORT_MICROSTEPS;
int downslope = 2 * SORT_MICROSTEPS;

int accelerationFactor = 1400;              //the top delay value when ramping. somewhere between 1000 and 2000 seems to work well.
int rampFactor = accelerationFactor / 100;  //the ramp delay microseconds to add/remove per step; should be roughly accelerationfactor / 50
int sortTestDelay = 150;                    //stop delay time in between sorts in test mode
//end acceleration settins


int pulsewidth = 10;  //this is used by both feed and sort motors to set the pulse width in micro seconds.

//not used but could be if you wanted to specify exact positions.
//referenced in the commented out code in the runsorter method below
//int sorterChutes[] ={0,17, 33, 49, 66, 83, 99, 116, 132};

//if your sorter design has a queue, you will set this value to your queue length.
//example of a queue would be like a rotary wheel where the currently recognized brass isn't going to fall into sorter for 3 more positions

int sorterQueue[QUEUE_LENGTH];

//inputs which can be set via serial console like:  feedspeed:50 or sortspeed:60
int feedSpeed = 95;  //range: 1..100
int feedSteps = 60;  //range 1..1000

int sortSpeed = 85;     //range: 1..100
int sortSteps = 20;     //range: 1..500 //20 default
int feedPauseTime = 0;  //range: 1.2000 //if you feed has two parts (back, forth), this is the pause time between the two parts.

//used to calculate motor speeds and works in conjunction with microsteps.
//480 for 8 microsteps, 240 for 16, 120 for 32,
int motorPulseDelay = 240;

//FEED STEP OVERRIDES
//These settings override the feedSteps&b and oddFeed settings above.
//If feedSteps & B are too coarse, you can directly calculate the microsteps needed between feed positions and use that here.
int feedMicroSteps = 0;        // how many microsteps between feeds - 0 to disable.
int feedFactionalStep = 1;     //this allows you to add a micro step every [fractionInterval].
int feedFractionInterval = 3;  //the interval at which micro steps get added.

//END FEED STEP OVERRIDES

//tracking variables
int sorterMotorCurrentPosition = 0;
bool isHomed = true;
int sorterMotorSpeed = 500;  //this is default and calculated at runtime. do not change this value
int feedMotorSpeed = 500;    //this is default and calculated at runtime. do not change this value


bool useFeedSensor = FEEDSENSOR_ENABLED;
void setup() {
  Serial.begin(9600);

  setSorterMotorSpeed(sortSpeed);
  setFeedMotorSpeed(feedSpeed);

  pinMode(FEED_Enable, OUTPUT);
  pinMode(SORT_Enable, OUTPUT);
  pinMode(FEED_DIRPIN, OUTPUT);
  pinMode(FEED_STEPPIN, OUTPUT);
  pinMode(SORT_DIRPIN, OUTPUT);
  pinMode(SORT_STEPPIN, OUTPUT);

  pinMode(FEED_HOMING_SENSOR, INPUT);
  pinMode(FEED_SENSOR, INPUT);

  digitalWrite(FEED_Enable, HIGH);
  digitalWrite(SORT_Enable, HIGH);
  digitalWrite(FEED_DIRPIN, HIGH);

  if(homeSorterOnStartup){
      checkSorterHoming(false);
  }
  if(homeFeedOnStartup){
      checkFeedHoming(false);
  }

  Serial.write("Ready\n");
}

void loop() {

  if (Serial.available() > 0) {

    String input = Serial.readStringUntil('\n');

    //normal input would just be the tray # you want to sort to which would be something like "1\n" or "9\n" (\n)if you are using serial console, the \n is already included in the writeline
    //so just the number 1-10 would suffice.
    //the software on the other end should be listening for  "done\n".

    //if true was returned, we go to next looping, else we continue down below
    if (parseSerialInput(input) == true) {
      return;
    }

    int sortPosition = input.toInt();
    QueueAdd(sortPosition);



    runSorterMotor(QueueFetch());

    runFeedMotorManual();
    checkFeedHoming(true);
    delay(50);  //allow for vibrations to calm down for clear picture
    Serial.print("done\n");
    PrintQueue();
  }
  delay(1);
}

//polls the homing sensor for about 10 seconds
void testHomingSensor() {
  for (int i = 0; i < 500; i++) {
    int value = digitalRead(FEED_HOMING_SENSOR);
    Serial.print(value);
    Serial.print("\n");
    delay(30);
  }
}

void feedDone() {
  digitalWrite(FEED_DONE_SIGNAL, HIGH);
  delay(feedDoneSignalTime);
  digitalWrite(FEED_DONE_SIGNAL, LOW);
}

void checkFeedHoming(bool isAutoHomeCycle) {
  if (isAutoHomeCycle == true && autoFeedHoming == false)
    return;

  int homingSensorVal = digitalRead(FEED_HOMING_SENSOR);

  if (homingSensorVal == 1) {
    return;  //we are homed! Continue
  }

  int i = 0;  //safety valve..
  int offset = homingFeedOffset * FEED_MICROSTEPS;
  while ((homingSensorVal == 0 && i < 12000) || offset > 0) {
    runFeedMotor(1);
    homingSensorVal = digitalRead(FEED_HOMING_SENSOR);
    i++;
    if (homingSensorVal == 1) {
      offset--;
    }
  }
}

void checkSorterHoming(bool isAutoHomeCycle) {
  if (isAutoHomeCycle == true && autoSorterHoming == false)
    return;

  int homingSensorVal = digitalRead(SORT_HOMING_SENSOR);
  int offset = homingSortOffset * FEED_MICROSTEPS;

  //if we are triggering the sensor and isAutoHomeCycle is not true, we run the motor forward a few steps and home back slowely
  //This does the jump. But if isAutoHomeCycle is false, the we just need to run the offset and are back off to the races.
  if (homingSensorVal == 1 && isAutoHomeCycle == false) {
    int hfs = 15 * SORT_MICROSTEPS;
    runSortMotorManual(hfs);
  } else if (homingSensorVal == 1) {
    runFeedMotor(offset);
    return;
  }

    //when trying to find home, the sorter arm should not move more than one full rotation (200 steps)
    int homingMax = 198 * FEED_MICROSTEPS;
    while (homingSensorVal == 0 && homingMax > 0) {
      runFeedMotor(-1);
      homingSensorVal = digitalRead(FEED_HOMING_SENSOR);
      homingMax--;
    }

    //now lets run forward our offset
    runFeedMotor(offset);

}

//moves the sorter arm. Blocking operation until complete
void runSorterMotor(int chute) {
  //you can uncomment to use the array sorterChutes for exact positions of each slot
  //int newStepsPos = sorterChutes[chute]; //the steps position from zero of the "slot"

  //rather than use exact positions, we are assuming uniform seperation betweeen slots specified by sorter_chute_seperation constant
  int newStepsPos = chute * SORTER_CHUTE_SEPERATION;

  //calculate the amount of movement and move.
  int nextmovement = newStepsPos - sorterMotorCurrentPosition;  //the number of +-steps between current position and next position
  int movement = nextmovement * SORT_MICROSTEPS;                //calculate the number of microsteps required

  if (movement != 0) {
    delay(slotDropDelay);
  }

  if (useAcceleration) {
    runSortMotorManualAcc(movement);
  } else {
    runSortMotorManual(movement);
  }

  //if the chute command is zero and we were not already at zero, we autohome if enabled
  if(chute==0 && abs(movement) > 0){
    checkSorterHoming(true); 
  }

  sorterMotorCurrentPosition = newStepsPos;
}

int fractionIntervalIndex = 0;

void runFeedMotorManual() {

  if (useFeedSensor) {
    int i = 0;
    while (digitalRead(FEED_SENSOR) != 0 && i < 60) {
      i++;
      delay(50);
    }
    if (i > 60) {
      return;
    }
  }
  int steps = 0;

  //if the feedMicroSteps override is used, we do that and return.
  if (feedMicroSteps > 0) {
    steps = feedMicroSteps;
    if (fractionIntervalIndex == feedFractionInterval) {
      steps = steps + feedFactionalStep;
      fractionIntervalIndex = 0;
    } else {
      fractionIntervalIndex++;
    }
    runFeedMotor(steps);
    return;
  }

  digitalWrite(FEED_DIRPIN, LOW);
  int curFeedSteps = abs(feedSteps);
  //calculate the steps based on microsteps.
  steps = curFeedSteps * FEED_MICROSTEPS;

  for (int i = 0; i < steps; i++) {
    digitalWrite(FEED_STEPPIN, HIGH);
    delayMicroseconds(pulsewidth);  //pulse. recommended minimum > than 1 micro
    digitalWrite(FEED_STEPPIN, LOW);
    delayMicroseconds(feedMotorSpeed);
  }
}

void runFeedMotor(int steps) {
  digitalWrite(FEED_DIRPIN, LOW);
  for (int i = 0; i < steps; i++) {
    digitalWrite(FEED_STEPPIN, HIGH);
    delayMicroseconds(pulsewidth);  //pulse. i have found 60 to be very consistent with tb6600. Noticed that faster pulses tend to drop steps.
    digitalWrite(FEED_STEPPIN, LOW);
    delayMicroseconds(feedMotorSpeed);  //speed 156 = 1 second per revolution
  }
}

void runSortMotorManual(int steps) {

  if (steps > 0) {
    digitalWrite(SORT_DIRPIN, LOW);
  } else {
    digitalWrite(SORT_DIRPIN, HIGH);
  }
  delay(5);
  steps = abs(steps);
  for (int i = 0; i < steps; i++) {
    digitalWrite(SORT_STEPPIN, HIGH);
    delayMicroseconds(pulsewidth);  //pulse //def 60
    digitalWrite(SORT_STEPPIN, LOW);
    delayMicroseconds(sorterMotorSpeed);  //speed 156 = 1 second per revolution //def 20
  }
}

void runSortMotorManualAcc(int steps) {

  if (steps == 0)
    return;
  bool forward = true;
  if (steps > 0) {
    digitalWrite(SORT_DIRPIN, LOW);
  } else {
    digitalWrite(SORT_DIRPIN, HIGH);
    forward = false;
  }
  delay(5);

  steps = abs(steps);

  int fullspeedsteps = steps - downslope - upslope;
  //accelerate linear
  for (int i = 1; i <= upslope; i++) {

    int msdelay = accelerationFactor - (i * rampFactor);
    if (msdelay < sorterMotorSpeed) {
      msdelay = sorterMotorSpeed;
    }
    digitalWrite(SORT_STEPPIN, HIGH);
    delayMicroseconds(pulsewidth);  //pulse //def 60
    digitalWrite(SORT_STEPPIN, LOW);
    delayMicroseconds(msdelay);  //speed 156 = 1 second per revolution //def 20
  }

  //full speed
  for (int i = 1; i <= fullspeedsteps; i++) {
    digitalWrite(SORT_STEPPIN, HIGH);
    delayMicroseconds(pulsewidth);  //pulse //def 60
    digitalWrite(SORT_STEPPIN, LOW);
    delayMicroseconds(sorterMotorSpeed);  //speed 156 = 1 second per revolution //def 20
  }

  //Deaccelerate linear
  for (int i = 1; i <= downslope; i++) {
    int msdelay = sorterMotorSpeed + (i * rampFactor);
    if (msdelay >= sorterMotorSpeed) {
      msdelay = accelerationFactor;
    }
    digitalWrite(SORT_STEPPIN, HIGH);
    delayMicroseconds(pulsewidth);  //pulse //def 60
    digitalWrite(SORT_STEPPIN, LOW);
    delayMicroseconds(msdelay);  //speed 156 = 1 second per revolution //def 20
  }
}


void setSorterMotorSpeed(int speed) {
  sorterMotorSpeed = setSpeedConversion(speed);
}
void setFeedMotorSpeed(int speed) {
  feedMotorSpeed = setSpeedConversion(speed);
}

int setSpeedConversion(int speed) {
  if (speed < 1 || speed > 100) {
    return 500;
  }

  double proportion = (double)(speed - 1) / 99;       //scale the range to number between 0 and 1;
  int output = (int)(proportion * (1000 - 60)) + 60;  //scale range 0-1 to desired output range

  int finaloutput = 1060 - output;  //reverse the output;

  return finaloutput;
}


//used for debugging queue issues.
void PrintQueue() {
  if (PRINT_QUEUEINFO == false)
    return;
  Serial.print("-------\n");
  Serial.print("QueueStatus:\n");
  for (int i = 0; i < QUEUE_LENGTH; i++) {
    if (i > 0)
      Serial.print(',');
    Serial.print(sorterQueue[i]);
  }
  Serial.print("\nSorting to: ");
  Serial.print(QueueFetch());
  Serial.print("\n-------\n");
}

//adds a position item into the front of the sorting queue. Queue is used in FIFO mode. PUSH
void QueueAdd(int pos) {
  for (int i = QUEUE_LENGTH; i > 0; i--) {
    sorterQueue[i] = sorterQueue[i - 1];
  }
  sorterQueue[0] = pos;
}

//fetches items from back of queue. POP
int QueueFetch() {
  return sorterQueue[QUEUE_LENGTH - 1];
}

//return true if you wish to stop processing and continue to next loop
bool parseSerialInput(String input) {
  if (isDigit(input[0])) {
    return false;
  }

  if (input.startsWith("usefeedsensor:")) {
    input.replace("usefeedsensor:", "");
    useFeedSensor = false;
    if (input == "1") {
      useFeedSensor = FEEDSENSOR_ENABLED;
    }
    Serial.print("ok\n");
    return true;
  }
  //set feed speed. Values 1-100. Def 60
  if (input.startsWith("feedspeed:")) {
    input.replace("feedspeed:", "");
    feedSpeed = input.toInt();
    setFeedMotorSpeed(feedSpeed);
    Serial.print("ok\n");
    return true;
  }


  //used to test tracking on steppers for feed motor. specify the number of feeds to perform: eg:  "test:60" will feed 60 times.
  if (input.startsWith("test:")) {
    input.replace("test:", "");
    int testcount = input.toInt();
    int slot = 0;
    for (int a = 0; a < testcount; a++) {
      slot = random(0, 5);
      Serial.print(a);
      Serial.print(" - ");
      Serial.print(slot);
      runSorterMotor(slot);
      runFeedMotorManual();
      checkFeedHoming(true);
      feedDone();
      Serial.print("\n");
      delay(sortTestDelay);
    }
    Serial.print("ok\n");
    return true;
  }


  //used to test tracking on steppers for feed motor. specify the number of feeds to perform: eg:  "test:60" will feed 60 times.
  if (input.startsWith("sorttest:")) {
    input.replace("sorttest:", "");
    int testcount = input.toInt();
    int slot = 0;
    int cslot = 0;

    for (int a = 0; a <= testcount; a++) {
      if (a == testcount) {
        runSorterMotor(0);
        continue;
      }

      slot = random(0, 8);
      while (slot == cslot) {
        slot = random(0, 8);
      }

      cslot = slot;
      Serial.print(a);
      Serial.print(" - ");
      Serial.print(slot);
      Serial.print("\n");
      runSorterMotor(slot);
      delay(sortTestDelay);
    }


    runSorterMotor(0);
    Serial.print("ok\n");
    return true;
  }

  //set sort speed. Values 1-100. Def 60
  if (input.startsWith("sortspeed:")) {
    input.replace("sortspeed:", "");
    sortSpeed = input.toInt();
    setSorterMotorSpeed(sortSpeed);
    Serial.print("ok\n");
    return true;
  }

  //set sort steps. Values 1-100. Def 20
  if (input.startsWith("sortsteps:")) {
    input.replace("sortsteps:", "");
    sortSteps = input.toInt();
    Serial.print("ok\n");
    return true;
  }

  //set feed steps. Values 1-1000. Def 100
  if (input.startsWith("feedsteps:")) {
    input.replace("feedsteps:", "");
    feedSteps = input.toInt();
    Serial.print("ok\n");
    return true;
  }

  //set feed steps. Values 1-1000. Def 100
  if (input.startsWith("feedpausetime:")) {
    input.replace("feedpausetime:", "");
    feedPauseTime = input.toInt();
    Serial.print("ok\n");
    return true;
  }

  //set feed steps. Values 1-1000. Def 100
  if (input.startsWith("autohome:")) {
    input.replace("autohome:", "");
    input.replace(" ", "");
    autoFeedHoming = input == "1";
    Serial.print("ok\n");
    return true;
  }

  //to change sorter arm position by slot number
  if (input.startsWith("sortto:")) {
    input.replace("sortto:", "");
    int msortsteps = input.toInt();
    runSorterMotor(msortsteps);
    Serial.print("done\n");
    return true;
  }

  //to change sorter arm position by steps.
  if (input.startsWith("sorttosteps:")) {
    input.replace("sorttosteps:", "");
    int msortsteps = input.toInt();
    runSortMotorManual(msortsteps);
    Serial.print("done\n");
    return true;
  }
  //home the feeder
  if (input.startsWith("home")) {
    checkFeedHoming(false);
    Serial.print("done\n");
    return true;
  }
  if (input.startsWith("testhome")) {
    testHomingSensor();
    Serial.print("done\n");
    return true;
  }
  if (input.startsWith("setvar:")) {
    input.replace("setvar:", "");

    return true;
  }

  //to run feeder, send  any string starting with f:
  if (input.startsWith("f")) {
    input.replace("f", "");
    int fs = input.toInt();
    runFeedMotor(fs);
    Serial.print("done\n");
    return true;
  }
  if (input.startsWith("getconfig")) {

    Serial.print("{\"FeedMotorSpeed\":");
    Serial.print(feedSpeed);

    Serial.print(",\"FeedCycleSteps\":");
    Serial.print(feedSteps);

    Serial.print(",\"SortMotorSpeed\":");
    Serial.print(sortSpeed);

    Serial.print(",\"SortSteps\":");
    Serial.print(sortSteps);

    Serial.print("}\n");
    return true;
  }

  return false;  //nothing matched, continue processing the loop at normal
}
