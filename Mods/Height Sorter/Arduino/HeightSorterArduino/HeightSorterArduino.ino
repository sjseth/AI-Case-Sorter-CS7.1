///CS7.1 HEIGHT SORTER MOD BY JR
//SJSETH - rev: 1.1

#include <Wire.h>

#define FEED_CYCLE_DELAY 100 // the amount of time in milliseconds to wait in between feeds. If not using feed sensor, recommend set this to 1 second (1000)
#define FEEDSENSOR_ENABLED true // enabled if feedsensor is installed and working;//this is a proximity sensor under the feed tube which tells us a case has dropped completely
#define FEED_HOMING_ENABLED true //enabled feed homing sensor


#if FEED_HOMING_ENABLED == false 
  #define FEED_STEPS 70  // 70 The amount to travel before starting the homing cycle. Should be less than (80 - FEED_HOMING_OFFSET_STEPS)
#else
 #define FEED_STEPS 80  // 80 The amount to travel between holes with no homing
#endif

#define FEED_DIRPIN 5 //maps to the DIRECTION signal for the feed motor
#define FEED_STEPPIN 2 //maps to the PULSE signal for the feed motor

#define FEED_MICROSTEPS 16  //how many microsteps the controller is configured for. 
#define FEED_HOMING_SENSOR 10  //connects to the feed wheel homing sensor
#define FEED_HOMING_SENSOR_TYPE 1 //1=NO (normally open) default switch, 0=NC (normally closed) (optical switches) 
#define FEED_SENSOR 9 

#define FEEDSENSOR_TYPE 0 // NPN = 0, PNP = 1
#define FEED_DONE_SIGNAL 12   // Writes HIGH Signal When Feed is done. Used for mods like AirDrop

#define MOTOR_Enable 8 //maps to the enable pin for the FEED MOTOR (on r3 shield enable is shared by motors)
#define AUTO_MOTORSTANDBY_TIMEOUT 60 // 0 = disabled; The time in seconds to wait after no motor movement before putting motors in standby

//ARDUINO CONFIGURATIONS
#define FEED_HOMING_OFFSET_STEPS 3 //additional steps to continue after homing sensor triggered

#define FEED_OVERSTEP_THRESHOLD 90 // 90 if we have gone this many steps without hitting a homing node, something is wrong. Throw an overstep error

//FEED MOTOR ACCELLERATION SETTINGS (DISABLED BY DEFAULT)
#define FEED_ACC_SLOPE 32  //2 steps * 16 MicroStes
#define ACC_FACTOR 1200 //acceleration factor
#define FEED_MOTOR_SPEED 90 //range of 1-100
#define ACC_FEED_ENABLED false //enabled or disables feed motor accelleration. 


///END OF USER CONFIGURATIONS ///
///DO NOT EDIT BELOW THIS LINE ///
int feedCycleDelay = FEED_CYCLE_DELAY;

long autoMotorStandbyTimeout = AUTO_MOTORSTANDBY_TIMEOUT;

int feedSpeed = FEED_MOTOR_SPEED; //represents a number between 1-100
int feedSteps = FEED_STEPS;
int feedMotorSpeed = 500;//this is default and calculated at runtime. do not change this value

int homingSteps = 0;

int feedMicroSteps = feedSteps * FEED_MICROSTEPS;

const int feedramp = (ACC_FACTOR + (FEED_MOTOR_SPEED /2)) / FEED_ACC_SLOPE;

int feedOverTravelSteps = feedMicroSteps - (FEED_OVERSTEP_THRESHOLD * FEED_MICROSTEPS);
int feedOffsetSteps = FEED_HOMING_OFFSET_STEPS;
int feedHomingOffset = feedOffsetSteps * FEED_MICROSTEPS;

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
bool feedOne = false;


String input = "";
int qPos1 = 0;
int qPos2 = 0;

int slotDelayCalc = 0;
bool IsRunning = false;
bool IsTestCycle=false;
bool IsSortTestCycle=false;
int testCycleInterval=0;
int testsCompleted=0;
int sortToSlot=0;
unsigned long theTime;
unsigned long timeSinceLastSortMove;
unsigned long timeSinceLastMotorMove;
unsigned long msgResetTimer;


void setup() {
  Serial.begin(9600);
  Serial.print("Ready\n");
  
  setFeedMotorSpeed(FEED_MOTOR_SPEED);

  pinMode(MOTOR_Enable, OUTPUT);
  pinMode(FEED_DIRPIN, OUTPUT);
  pinMode(FEED_STEPPIN, OUTPUT);
  pinMode(FEED_DONE_SIGNAL, OUTPUT);
  pinMode(FEED_HOMING_SENSOR, INPUT);
  pinMode(FEED_SENSOR, INPUT);

  digitalWrite(MOTOR_Enable, LOW);
  digitalWrite(FEED_DIRPIN, LOW);

  IsFeedHoming=true;

  msgResetTimer = millis();
}

// 'Ready' from Setup above
void loop() {
   checkSerial();
   scheduleRun(); 
   checkFeedErrors();
   runFeedMotor();
   homeFeedMotor();
   serialMessenger();
   onFeedComplete();
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
  if(FeedCycleInProgress==false && Serial.available()>0){
   
      //input = Serial.readStringUntil('\n');
       recvWithEndMarker();
       
       if(!commandReady){
        return;
       }
      // Serial.print(input);
       
     //this should be most cases
   
      if (input.startsWith("start")) {
         IsRunning=true;
         Serial.print("Started\n");
         resetCommand();
         return;
      } 
       if (input.startsWith("feed")) {
         feedOne = true;
         Serial.print("Started\n");
         resetCommand();
         return;
      } 
    
    
      if (input.startsWith("stop")) {
          resetCommand();
          IsRunning=false;

          IsFeedHoming=false;
          IsFeedHomingOffset = false;
          FeedCycleComplete=true;
          FeedCycleInProgress = false;
         // Serial.print("ok\n");
         return;
      } 

      resetCommand();
      Serial.print("ok\n");
  }
}




void checkFeedErrors(){
 if(FeedCycleComplete == false && FeedSteps < feedOverTravelSteps){

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
   
    timeSinceLastMotorMove = millis();
   //this allows some time for the brass to start dropping before generating the airblast

   
    delay(feedCycleDelay);
    Serial.print("done\n");
    //Serial.flush();
    FeedCycleComplete=false;

    return;
  }
  
}

void scheduleRun(){
 
  if(IsFeeding==false && (IsRunning==true || feedOne==true)){
    
    if(digitalRead(FEED_SENSOR) == FEEDSENSOR_TYPE || FEEDSENSOR_ENABLED==false){
      //set run variables
      IsFeedError=false;
      FeedSteps = feedMicroSteps;

      FeedCycleInProgress = true;
      FeedCycleComplete=false;
      IsFeeding=true;
     feedOne=false;
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
    
    if (digitalRead(FEED_HOMING_SENSOR) == FEED_HOMING_SENSOR_TYPE) {
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


void stepFeedMotor(){
    digitalWrite(MOTOR_Enable, LOW);
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


void setFeedMotorSpeed(int speed) {
  feedMotorSpeed = setSpeedConversion(speed);
}

int setSpeedConversion(int speed) {
  if (speed < 1 || speed > 100) {
    return 500;
  }

  return 1060 - ((int)(((double)(speed - 1) / 99) * (1000 - 60)) + 60);
}

void MotorStandByCheck(){
  if(IsFeeding)
    return;
  
  if(autoMotorStandbyTimeout==0)
    return;

  theTime = millis();

  if(theTime - timeSinceLastMotorMove > (autoMotorStandbyTimeout*1000) ) 
     digitalWrite(MOTOR_Enable, HIGH);
}



