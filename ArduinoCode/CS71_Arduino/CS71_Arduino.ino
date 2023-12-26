/// VERSION CS 7.1.231225.1 ///
/// REQUIRES AI SORTER SOFTWARE VERSION 1.1.35 or newer

#include <Wire.h>
#include <SoftwareSerial.h>


//PIN CONFIGURATIONS
//ARDUINO UNO WITH 4 MOTOR CONTROLLER
//Stepper controller is set to 16 Microsteps (3 jumpers in place)

#define FEED_DIRPIN 5 //maps to the DIRECTION signal for the feed motor
#define FEED_STEPPIN 2 //maps to the PULSE signal for the feed motor
#define FEED_Enable 8 //maps to the enable pin for the FEED MOTOR (on r3 shield enable is shared by motors)
#define FEED_MICROSTEPS 16  //how many microsteps the controller is configured for. 
#define FEED_HOMING_SENSOR 10  //connects to the feed wheel homing sensor
#define FEED_SENSOR 9 //the proximity sensor under the feed wheel 
#define FEEDSENSOR_ENABLED true //enabled if feedsensor is installed and working;//this is a proximity sensor under the feed tube which tells us a case has dropped completely
#define FEEDSENSOR_TYPE 0 // NPN = 0, PNP = 1
#define FEED_DONE_SIGNAL 12   // Writes HIGH Signal When Feed is done. Used for mods like AirDrop
#define FEED_HOMING_ENABLED true //enabled feed homing sensor

#define SORT_DIRPIN 6 //maps to the DIRECTION signal for the sorter motor
#define SORT_STEPPIN 3 //maps to the PULSE signal for the sorter motor
#define SORT_Enable 8 //maps to the enable pin for the FEED MOTOR (on r3 shield enable is shared by motors)
#define SORT_MICROSTEPS 16 //how many microsteps the controller is configured for. 
#define SORT_HOMING_SENSOR 11  //connects to the sorter homing sensor
#define AIR_DROP_ENABLED false //enables airdrop

//ARDUINO CONFIGURATIONS

//number of steps between chutes. With the 8 and 10 slot attachments, 20 is the default. 
//If you have customized sorter output drops, you will need to change this setting to meet your needs. 
//Note there are 200 steps in 1 revolution of the sorter motor. 
#define SORTER_CHUTE_SEPERATION 20 


#define FEED_HOMING_OFFSET_STEPS 3 //additional steps to continue after homing sensor triggered
#define FEED_STEPS 70  //The amount to travel before starting the homing cycle. Should be less than (80 - FEED_HOMING_OFFSET_STEPS)
#define FEED_OVERSTEP_THRESHOLD 90 //if we have gone this many steps without hitting a homing node, something is wrong. Throw an overstep error

//FEED MOTOR ACCELLERATION SETTINGS (DISABLED BY DEFAULT)
#define FEED_ACC_SLOPE 32  //2 steps * 16 MicroStes

#define FEED_MOTOR_SPEED 94 //range of 1-100
#define ACC_FEED_ENABLED false //enabled or disables feed motor accelleration. 

//SORT MOTOR ACCELLERATION SETTINGS (ENABLED BY DEFAULT)
#define ACC_SORT_ENABLED true
#define SORT_MOTOR_SPEED 94 //range of 1-100
#define SORT_ACC_SLOPE 64 //this is the number of microsteps to accelerate and deaccellerate in a sort. 
#define ACC_FACTOR 1200
#define SORT_HOMING_ENABLED true
//FEED DELAY SETTINGS

// Used to send signal to add-ons when feed cycle completes (used by airdrop mod). 
// IF NOT USING MODS, SET TO 0. With Airdrop set to 60-100 (length of the airblast)
#define FEED_CYCLE_COMPLETE_SIGNALTIME 50 

// The amount of time to wait after the feed completes before sending the FEED_CYCLE_COMPLETE SIGNAL
// IF NOT USING MODS, SET TO 0. with Airdrop set to 30-50 which allows the brass to start falling before sending the blast of air. 
#define FEED_CYCLE_COMPLETE_PRESIGNALDELAY 30

// Time in milliseconds to wait before sending "done" response to serialport (allows for everything to stop moving before taking the picture): runs after the feed_cycle_complete signal
// With AirDrop mod enabled, it needs about 20-30MS. If airdrop is not enabled, it should be closer to 50-70. 
// If you are getting blurred pictures, increase this value. 
#define FEED_CYCLE_NOTIFICATION_DELAY 90 

//when airdrop is enabled, this value is used instead of SLOT_DROP_DELAY but does the same thing
//Usually can be 100 or lower, increase value if brass not clearing the tube before it moves to next slot. 
#define FEED_CYCLE_COMPLETE_POSTDELAY 0

// number of MS to wait after feedcycle before moving sort arm.
// Prevents slinging brass. 
// This gives time for the brass to clear the sort tube before moving the sort arm. 
#define SLOT_DROP_DELAY 400


///END OF USER CONFIGURATIONS ///
///DO NOT EDIT BELOW THIS LINE ///

int notificationDelay = FEED_CYCLE_NOTIFICATION_DELAY;
bool airDropEnabled = AIR_DROP_ENABLED;
int feedCycleSignalTime = FEED_CYCLE_COMPLETE_SIGNALTIME;
int feedCyclePreDelay = FEED_CYCLE_COMPLETE_PRESIGNALDELAY;
int feedCyclePostDelay = FEED_CYCLE_COMPLETE_POSTDELAY;
int slotDropDelay = SLOT_DROP_DELAY;
int dropDelay =  airDropEnabled ? feedCyclePostDelay : slotDropDelay;


int feedSpeed = FEED_MOTOR_SPEED; //represents a number between 1-100
int feedSteps = FEED_STEPS;
int feedMotorSpeed = 500;//this is default and calculated at runtime. do not change this value

int sortSpeed = SORT_MOTOR_SPEED; //represents a number between 1-100
int sortSteps = SORTER_CHUTE_SEPERATION;
int sortMotorSpeed = 500;//this is default and calculated at runtime. do not change this value
int homingSteps = 0;

int feedMicroSteps = feedSteps * FEED_MICROSTEPS;

const int feedramp = (ACC_FACTOR + (FEED_MOTOR_SPEED /2)) / FEED_ACC_SLOPE;
const int sortramp = (ACC_FACTOR + (SORT_MOTOR_SPEED /2)) / SORT_ACC_SLOPE;
int feedOverTravelSteps = feedMicroSteps - (FEED_OVERSTEP_THRESHOLD * FEED_MICROSTEPS);
const int feedHomingOffset = FEED_HOMING_OFFSET_STEPS * FEED_MICROSTEPS;

bool FeedScheduled = false;
bool IsFeeding = false;
bool IsFeedHoming = false;
bool IsFeedHomingOffset = false;
bool FeedCycleInProgress = false;
bool FeedCycleComplete = false;
bool IsFeedError = false;

int FeedSteps = feedMicroSteps;
int FeedHomingOffsetSteps = feedHomingOffset;
int feedDelayMS = 150;
int sortDelayMS = 400;

bool forceFeed=false;
String input = "";
int qPos1 = 0;
int qPos2 = 0;
int sortStepsToNextPosition = 0;
int sortStepsToNextPositionTracker=0;
bool SortInProgress = false;
bool SortComplete = false;
bool IsSorting = false;
bool IsSortHoming = false;
int slotDelayCalc = 0;

bool IsTestCycle=false;
bool IsSortTestCycle=false;
int testCycleInterval=0;
int testsCompleted=0;
int sortToSlot=0;
unsigned long theTime;
unsigned long timeSinceLastSortMove;
unsigned long msgResetTimer;


void setup() {
  Serial.begin(9600);
  Serial.print("Ready\n");
  
  setSorterMotorSpeed(SORT_MOTOR_SPEED);
  setFeedMotorSpeed(FEED_MOTOR_SPEED);

  pinMode(FEED_Enable, OUTPUT);
  pinMode(SORT_Enable, OUTPUT);
  pinMode(FEED_DIRPIN, OUTPUT);
  pinMode(FEED_STEPPIN, OUTPUT);
  pinMode(SORT_DIRPIN, OUTPUT);
  pinMode(SORT_STEPPIN, OUTPUT);

  pinMode(FEED_DONE_SIGNAL, OUTPUT);
  pinMode(FEED_HOMING_SENSOR, INPUT);
  pinMode(SORT_HOMING_SENSOR, INPUT);
  pinMode(FEED_SENSOR, INPUT);


  digitalWrite(FEED_Enable, HIGH);
  digitalWrite(SORT_Enable, HIGH);
  digitalWrite(FEED_DIRPIN, LOW);

  IsFeedHoming=true;
  IsSortHoming=true;
  msgResetTimer = millis();
}


void loop() {
   checkSerial();
   runSortMotor();
   onSortComplete();
   scheduleRun();
   checkFeedErrors();
   runFeedMotor();
   homeFeedMotor();
   homeSortMotor();
   serialMessenger();
   onFeedComplete();
   runAux();

}

int FreeMem(){
  extern int __heap_start, *__brkval;
  int v;
  return(int) &v - (__brkval ==0 ? (int) &__heap_start : (int) __brkval);
}

bool commandReady = false;
char endMarker = '\n';
char rc;

void recvWithEndMarker() {
    while (Serial.available() > 0 ) {
        rc = Serial.read();
        delay(1);
        if (rc != endMarker) {
            input += rc;
        }
        else {
            commandReady=true; 
            return;
        }
    }
 }
void resetCommand(){
  input="";
  commandReady=false;
}

void checkSerial(){
  if(FeedCycleInProgress==false && SortInProgress==false && Serial.available()>0){
   
      //input = Serial.readStringUntil('\n');
       recvWithEndMarker();
       
       if(!commandReady){
        return;
       }
      // Serial.print(input);
       
     //this should be most cases
      if (isDigit(input[0])) {
        moveSorterToNextPosition(input.toInt());
        FeedScheduled = true;
        IsFeeding = false;
        scheduleRun();
        resetCommand();
        return;
      }
      if (input.startsWith("homefeeder")) {
        feedDelayMS=400;
          IsFeedHoming=true;
         Serial.print("ok\n");
         resetCommand();
         return;
      } 
      if (input.startsWith("homesorter")) {
        sortDelayMS=400;
          IsSortHoming=true;
          Serial.print("ok\n");
          resetCommand();
         return;
      } 
      if (input.startsWith("stop")) {
      resetCommand();
          FeedScheduled=false;
          IsFeedHoming=false;
          IsFeedHomingOffset = false;
          FeedCycleComplete=true;
          FeedCycleInProgress = false;
         // Serial.print("ok\n");
         return;
      } 

      if (input.startsWith("sortto:")) {
          input.replace("sortto:", "");
          moveSorterToPosition(input.toInt());
           Serial.print("ok\n");
           resetCommand();
         return;
      } 

      if (input.startsWith("xf:")) {
          input.replace("xf:", "");
          forceFeed = true;
          moveSorterToNextPosition(input.toInt());
          resetCommand();
          FeedScheduled = true;
          IsFeeding = false;
          scheduleRun();
          return;
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

        Serial.print(",\"NotificationDelay\":");
        Serial.print(notificationDelay);

        Serial.print(",\"SlotDropDelay\":");
        Serial.print(slotDropDelay);

        Serial.print(",\"AirDropEnabled\":");
        Serial.print(airDropEnabled);

        Serial.print(",\"AirDropPostDelay\":");
        Serial.print(feedCyclePostDelay);

        Serial.print(",\"AirDropPreDelay\":");
        Serial.print(feedCyclePreDelay);

        Serial.print(",\"AirDropSignalTime\":");
        Serial.print(feedCycleSignalTime);

        Serial.print("}\n");
        resetCommand();
        return;      
      }

       //set feed speed. Values 1-100. Def 60
      if (input.startsWith("feedspeed:")) {
        input.replace("feedspeed:", "");
        feedSpeed = input.toInt();
        setFeedMotorSpeed(feedSpeed);
        Serial.print("ok\n");
        resetCommand();
        return;
      }

       if (input.startsWith("sortspeed:")) {
        input.replace("sortspeed:", "");
        sortSpeed = input.toInt();
        setSorterMotorSpeed(sortSpeed);
        Serial.print("ok\n");
        resetCommand();
        return;
      }

      //set sort steps. Values 1-100. Def 20
      if (input.startsWith("sortsteps:")) {
        input.replace("sortsteps:", "");
        sortSteps = input.toInt();
        Serial.print("ok\n");
        resetCommand();
        return;
      }

      //set feed steps. Values 1-1000. Def 100
      if (input.startsWith("feedsteps:")) {
        input.replace("feedsteps:", "");
        feedSteps = input.toInt();
        feedMicroSteps = feedSteps * FEED_MICROSTEPS;
        feedOverTravelSteps = feedMicroSteps - (FEED_OVERSTEP_THRESHOLD * FEED_MICROSTEPS);
        Serial.print("ok\n");
        resetCommand();
        return;
      }
      if (input.startsWith("notificationdelay:")) {
        input.replace("notificationdelay:", "");
        notificationDelay = input.toInt();
        Serial.print("ok\n");
        resetCommand();
        return;
      }
      if (input.startsWith("slotdropdelay:")) {
        input.replace("slotdropdelay:", "");
        slotDropDelay = input.toInt();
        dropDelay =  airDropEnabled ? feedCyclePostDelay: slotDropDelay;
        Serial.print("ok\n");
        resetCommand();
        return;
      }
      if (input.startsWith("airdropenabled:")) {
        input.replace("airdropenabled:", "");
        airDropEnabled = stringToBool(input);
        dropDelay =  airDropEnabled ? feedCyclePostDelay: slotDropDelay;
        Serial.print("ok\n");
        resetCommand();
        return;
      }

      if (input.startsWith("airdroppostdelay:")) {
        input.replace("airdroppostdelay:", "");
        feedCyclePostDelay = input.toInt();
        dropDelay =  airDropEnabled ? feedCyclePostDelay: slotDropDelay;
        Serial.print("ok\n");
        resetCommand();
        return;
      }
       if (input.startsWith("airdroppredelay:")) {
        input.replace("airdroppredelay:", "");
        feedCyclePreDelay = input.toInt();
        Serial.print("ok\n");
        resetCommand();
        return;
      }
      if (input.startsWith("airdropdsignalduration:")) {
        input.replace("airdropdsignalduration:", "");
        feedCycleSignalTime = input.toInt();
        Serial.print("ok\n");
        resetCommand();
        return;
      }

      if (input.startsWith("test:")) {
        input.replace("test:", "");
        IsTestCycle=true;
        testCycleInterval=input.toInt();
        testsCompleted=0;
        FeedScheduled=false;
        FeedCycleInProgress=false;
        Serial.print("testing started\n");
        resetCommand();
        return;
      }

       if (input.startsWith("sorttest:")) {
        input.replace("sorttest:", "");
        IsSortTestCycle=true;
        testCycleInterval=input.toInt();
        testsCompleted=0;
        Serial.print("testing started\n");
        resetCommand();
        return;
      }
       if (input.startsWith("ping")) {
        Serial.print(FreeMem());
        resetCommand();
        Serial.print(" ok\n");
        return;
      }
      resetCommand();
      Serial.print("ok\n");
  }
}

bool stringToBool(String str) {
  str.toLowerCase();
  // Compare the string to "true" and return true if they match
  if(str=="true"){
    return true;
  }
  if(str == "1"){
    return true;
  }
  return false;
  
}

//this method is to run all "other" routines not in the main duty cycles (such as tests)
void runAux(){

  //This runs the feed and sort test if scheduled
  if(IsTestCycle==true&&FeedScheduled==false&&FeedCycleInProgress==false){
    if(testsCompleted<testCycleInterval){
       int slot = random(0,6);
         moveSorterToNextPosition(slot);
        
        FeedScheduled = true;
        IsFeeding = false;
        scheduleRun();
        
        Serial.print(testsCompleted);
        Serial.print(" - ");
        Serial.print(slot);
        Serial.print("\n");
        testsCompleted++;

            
    }else{
      IsTestCycle=false;
      testCycleInterval=0;
      testsCompleted=0;
    }
  }

  //this runs the sorter only test cycles if scheduled
  if(IsSortTestCycle==true&&SortInProgress==false){
    if(testsCompleted<testCycleInterval){
       int slot = random(0,8);
       delay(40);
       Serial.print(testsCompleted);
       Serial.print(" - Sorting to: ");
       Serial.println(slot);
       moveSorterToPosition(slot);
        testsCompleted++;
    }else{
      moveSorterToPosition(0);
      Serial.println("Sort Test Completed");
      IsSortTestCycle=false;
      testsCompleted=0;
      testCycleInterval=0;
    }
  }
}


void moveSorterToNextPosition(int position){
    sortToSlot=position;
    sortStepsToNextPosition = (qPos1 * sortSteps * SORT_MICROSTEPS) - (qPos2 * sortSteps * SORT_MICROSTEPS);
    sortStepsToNextPositionTracker = sortStepsToNextPosition;
    if(sortStepsToNextPosition !=0){
      theTime = millis();
       slotDelayCalc = (dropDelay - (theTime - timeSinceLastSortMove));
       slotDelayCalc = slotDelayCalc > 0? slotDelayCalc : 1;
       if(slotDelayCalc > dropDelay){
         slotDelayCalc=dropDelay;
       }

      // Serial.println(slotDelayCalc);
      delay(slotDelayCalc);
    }
    qPos1 = qPos2;
    qPos2 =position;
    SortInProgress = true;
    SortComplete = false;
    IsSorting = true;
}

void moveSorterToPosition(int position){
    sortToSlot=position;
    sortStepsToNextPosition = (qPos1 * sortSteps * SORT_MICROSTEPS) - (position * sortSteps * SORT_MICROSTEPS);
    sortStepsToNextPositionTracker = sortStepsToNextPosition;
   
  // Serial.println(position);
   //Serial.println(sortStepsToNextPosition);
    qPos1 =position;
    qPos2 =position;
    SortInProgress = true;
    SortComplete = false;
    IsSorting = true;
}

void runSortMotor(){
  if(IsSorting==true){
    
    if(sortStepsToNextPosition==0){
     
      if(qPos1==0){
        homingSteps=0;
         IsSortHoming=true;
      }else{
         IsSorting=false;
         SortComplete = true;
      }
      return;
    }
    setAccSortDelay();
    
    if(sortStepsToNextPosition > 0){
      stepSortMotor(true);
      sortStepsToNextPosition--;
    }
    else {
      stepSortMotor(false);
      sortStepsToNextPosition++;
    }
  }
}
void setAccSortDelay(){
    if(ACC_SORT_ENABLED == false){
      sortDelayMS=sortMotorSpeed;
      return;
    }
    //up ramp
    if((abs(sortStepsToNextPositionTracker) - abs(sortStepsToNextPosition)) < SORT_ACC_SLOPE ){
      sortDelayMS = ACC_FACTOR - ((abs(sortStepsToNextPositionTracker) - abs(sortStepsToNextPosition)) * sortramp);
      if (sortDelayMS < sortMotorSpeed) {
         sortDelayMS = sortMotorSpeed;
      }
     
      return;
    }
  
    //down ramp
    if (abs(sortStepsToNextPositionTracker) - abs(sortStepsToNextPosition) > (abs(sortStepsToNextPositionTracker) - SORT_ACC_SLOPE)){
        sortDelayMS = (SORT_ACC_SLOPE - abs(sortStepsToNextPosition)) * sortramp;
         if (sortDelayMS < sortMotorSpeed) {
             sortDelayMS = sortMotorSpeed;
         }
         if (sortDelayMS > ACC_FACTOR) {
             sortDelayMS = ACC_FACTOR;
         }
         
    }else{
      //normal run speed
      sortDelayMS = sortMotorSpeed;
    }
    
}
void stepSortMotor(bool forward){
    
     if(forward==true){
       digitalWrite(SORT_DIRPIN, HIGH);
     }else{
      digitalWrite(SORT_DIRPIN, LOW);
    }
    digitalWrite(SORT_STEPPIN, HIGH);
    delayMicroseconds(1);  //pulse width
    digitalWrite(SORT_STEPPIN, LOW);
    delayMicroseconds(sortDelayMS); //controls motor speed
}
void onSortComplete(){
  if(SortInProgress==true && SortComplete==true){
        SortInProgress=false;
        timeSinceLastSortMove = millis();
       // Serial.println("runscheduled");
  }
}

void checkFeedErrors(){
 if(FeedCycleComplete == false && FeedSteps < feedOverTravelSteps){
      FeedScheduled=false;
      FeedCycleComplete=true;
      IsFeeding=false;
      IsFeedHoming= false;
      IsFeedHomingOffset=false;
      IsFeedError = true;
      FeedCycleInProgress = false;
      Serial.println("error:feed overtravel detected");
 }
}
void serialMessenger(){
  
 return; 

}

void onFeedComplete(){
  if(FeedCycleComplete==true&& IsFeedError==false){
   
    
   //this allows some time for the brass to start dropping before generating the airblast

    if(airDropEnabled)
    {
      delay(feedCyclePreDelay);
      digitalWrite(FEED_DONE_SIGNAL, HIGH);
      delay(feedCycleSignalTime);
      digitalWrite(FEED_DONE_SIGNAL,LOW);
     
    }
    delay(notificationDelay);
    Serial.print("done\n");
    //Serial.flush();
    FeedCycleComplete=false;
    forceFeed= false;
    return;
  }
  
}

void scheduleRun(){
 
  if(FeedScheduled==true && IsFeeding==false){
    if(digitalRead(FEED_SENSOR) == FEEDSENSOR_TYPE || forceFeed==true || FEEDSENSOR_ENABLED==false){
      //set run variables
      IsFeedError=false;
      FeedSteps = feedMicroSteps;
      FeedScheduled=false;
      FeedCycleInProgress = true;
      FeedCycleComplete=false;
      IsFeeding=true;
    }else{
      theTime = millis();
      if(theTime - msgResetTimer > 1000){
         // Serial.flush();
          Serial.println("waiting for brass");
         
          msgResetTimer = millis();
      }
    // Serial.flush();
     
    }
  }
}
void runFeedMotor() {
  if(SortInProgress){
    return;
  }
  
  if(IsFeeding==true && FeedSteps > 0 )
  {
    setAccFeedDelay();
    stepFeedMotor();
    FeedSteps--;
    return;
  }
  if(IsFeeding==true){
    IsFeeding=false;
    IsFeedHoming = true;
  }
  return;

}

void homeFeedMotor(){
  
  if(IsFeedHoming==true ){
   
    if(FEED_HOMING_ENABLED == false){
      IsFeedHoming=false;
       IsFeedHomingOffset = false;
      FeedCycleComplete=true;
      FeedCycleInProgress = false;
      return;
    }
    
    if (digitalRead(FEED_HOMING_SENSOR) == 1) {
      IsFeedHoming=false;
      
     // Serial.println("homed!");
      if(FeedCycleInProgress){ //if we are homing initially, we don't need to apply offsets
          IsFeedHomingOffset = true;
          FeedHomingOffsetSteps = feedHomingOffset;
      }
      return;
    }
    stepFeedMotor();
    FeedSteps--;
    return;
  }

  if(IsFeedHomingOffset == true){
    if(feedHomingOffset == 0)
    {
      IsFeedHomingOffset = false;
      FeedCycleComplete=true;
      FeedCycleInProgress = false;
      return;
    }
    if(IsFeedHomingOffset == true && FeedHomingOffsetSteps > 0){
      stepFeedMotor();
    
     // Serial.println(FeedHomingOffsetSteps);
      FeedHomingOffsetSteps--;
      FeedSteps--;
    }
    else if(IsFeedHomingOffset == true && FeedHomingOffsetSteps<=0){
      IsFeedHomingOffset = false;
      FeedCycleComplete=true;
      FeedCycleInProgress = false;
    }
  }
}

void homeSortMotor(){
  if(IsSortHoming==true && SORT_HOMING_ENABLED == false){
     IsSorting=false;
         SortComplete = true;
         IsSortHoming =false;
         return;
    }
  if(IsSortHoming==true){
     
    if(IsSorting==true){
        //code for running sort
         if(digitalRead(SORT_HOMING_SENSOR)==0){
          if(homingSteps < (200*SORT_MICROSTEPS)){
              stepSortMotor(true);  
              homingSteps++;          
          }
          return;
         }
         IsSorting=false;
         SortComplete = true;
         IsSortHoming =false;
    }
    else{
      if(digitalRead(SORT_HOMING_SENSOR)==0){
          if(homingSteps < (200*SORT_MICROSTEPS)){
              stepSortMotor(true);  
              homingSteps++;          
          }
      }else{
        IsSortHoming=false;
       // Serial.println("homed!");
      }
    }
  }
}

void stepFeedMotor(){
    digitalWrite(FEED_STEPPIN, HIGH);
    delayMicroseconds(1);  //pulse width
    digitalWrite(FEED_STEPPIN, LOW);
    delayMicroseconds(feedDelayMS); //controls motor speed
}

void setAccFeedDelay(){
    if(ACC_FEED_ENABLED == false){
      feedDelayMS=feedMotorSpeed;
      return;
    }
    //up ramp
    if((feedMicroSteps - FeedSteps) < FEED_ACC_SLOPE ){
      feedDelayMS = ACC_FACTOR - ((feedMicroSteps - FeedSteps) * feedramp);
      if (feedDelayMS < feedMotorSpeed) {
         feedDelayMS = feedMotorSpeed;
      }
      //Serial.println(msdelay);
      return;
    }
  
    //down ramp
    if (feedMicroSteps - FeedSteps > (feedMicroSteps - FEED_ACC_SLOPE)){
        feedDelayMS = (FEED_ACC_SLOPE - FeedSteps) * feedramp;
         if (feedDelayMS < feedMotorSpeed) {
             feedDelayMS = feedMotorSpeed;
         }
         if (feedDelayMS > ACC_FACTOR) {
             feedDelayMS = ACC_FACTOR;
         }
         //Serial.println(msdelay);
    }else{
      //normal run speed
      feedDelayMS = feedMotorSpeed;
    }
    
}

void setSorterMotorSpeed(int speed) {
  sortMotorSpeed = setSpeedConversion(speed);
}
void setFeedMotorSpeed(int speed) {
  feedMotorSpeed = setSpeedConversion(speed);
}

int setSpeedConversion(int speed) {
  if (speed < 1 || speed > 100) {
    return 500;
  }

  return 1060 - ((int)(((double)(speed - 1) / 99) * (1000 - 60)) + 60);
}
