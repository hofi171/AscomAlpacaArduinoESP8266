#ifndef A5BDF2AE_8CDD_4DFA_B4AE_E98D3FB81323
#define A5BDF2AE_8CDD_4DFA_B4AE_E98D3FB81323

#include <Arduino.h>
#include <EEPROM.h>
#include "ArduinoStepper_Types.h"

class ArduinoStepper
{
private:
    /* data */
eSTEPMODE StepperMode = FullStep2Phase;

#define POSSAVETOEEPROMTIME 2000

int pulseWidthMicrosULN2003 = 2000;  //100 microseconds
int pulseWidthMicros = 100;  //100 microseconds
int millisbetweenSteps = 5; // milliseconds - or try 1000 for sfalseer steps
#define EEADDRESS_POS 0
#define EEPROM_POS_ADDRESS 0
#define EEPROM_MODE_ADDRESS 4
#define EEPROM_PINS_ADDRESS 149
#define EEPROM_PINS_VALID_MARKER 153

// Default pin values (used if EEPROM is invalid/empty)
#define DEFAULT_ULN2003_Pin1 5
#define DEFAULT_ULN2003_Pin2 14
#define DEFAULT_ULN2003_Pin3 12
#define DEFAULT_ULN2003_Pin4 13

// Actual pin variables (loaded from EEPROM or defaults)
int ULN2003_Pin1;
int ULN2003_Pin2;
int ULN2003_Pin3;
int ULN2003_Pin4;

#define STEPSPERROUND 2048

#define SPEED_FAST 60
#define SPEED_false 10

uint32_t LastEEPROMPosition=0;

uint32_t DrivePosition=0; // Drive Position in Steps
uint32_t TargetPosition=0; // Target Position 
uint32_t MoveToTargetSteps=1;
bool bMoveToPos=false;
uint32_t position = 0;
uint32_t target = 0;
bool releaseMove = false;
bool bStop=false;

int StepPosition;
int MaxPositions;

unsigned long ActTime=0;
unsigned long ActMicros=0;
unsigned long LastPosTime=0;
unsigned long LastTime=0;
unsigned long VoltageLastTime=0;
unsigned long LastStepTime=0;
unsigned long LastPosSaveToEEPROMTime=0;

public:


ArduinoStepper(/* args */)
{    
  EEPROM.begin(512); // Initialize EEPROM with 512 bytes of storage
    
    // Load position from EEPROM
    position = readFromEEPROM(EEPROM_POS_ADDRESS);
    if(position < 0 || position > 20000){ // Sanity check for position value
      position = 0;
    }
    LastEEPROMPosition=position;
    
    // Load stepper mode from EEPROM
    int modeValue = readFromEEPROM(EEPROM_MODE_ADDRESS);
    if(modeValue >= FullStep && modeValue <= FullStep2Phase) {
      StepperMode = (eSTEPMODE) modeValue;
    } else {
      StepperMode = FullStep2Phase; // Default to FullStep2Phase if invalid
      writeToEEPROM(EEPROM_MODE_ADDRESS, StepperMode);
    }
    
    // Load pins from EEPROM with validation
    loadPinsFromEEPROM();

    // Configure pins as outputs
    pinMode(ULN2003_Pin1, OUTPUT);
    pinMode(ULN2003_Pin2, OUTPUT);
    pinMode(ULN2003_Pin3, OUTPUT);
    pinMode(ULN2003_Pin4, OUTPUT);

    setStepMode(StepperMode);

    LOG_INFO("ArduinoStepper initialized with position: " + String(position) + " and step mode: " + String(StepperMode));
    LOG_INFO("Stepper pins - Pin1: " + String(ULN2003_Pin1) + " Pin2: " + String(ULN2003_Pin2) + 
             " Pin3: " + String(ULN2003_Pin3) + " Pin4: " + String(ULN2003_Pin4));
}


~ArduinoStepper()
{   
}


void Init()
{
delay(500);
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
          delay(500);
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
          delay(500);
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
         delay(500);
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, true);
delay(500);
}

void setTarget(uint32_t newpos)
{
  target = newpos;
  bMoveToPos=true;
  
      bStop=false;
  LOG_INFO("Target position set to: " + String(target) + " from current position: " + String(position));
}

void setActualPosition(uint32_t newpos)
{
  position = newpos;
  saveToEEPROM(EEPROM_POS_ADDRESS,position); // Save position for later
  LOG_INFO("Actual position set to: " + String(position));
}

int getTarget()
{
  return target;
}

int getPosition()
{
  return position;
}

bool isMoving()
{
  return bMoveToPos;
  LOG_INFO("isMoving called - returning: " + String(bMoveToPos));
}

void halt()
{
  bStop=true;
}   

void Update() {
 //LOG_INFO("Update called - Current position: " + String(position) + " Target position: " + String(target) + " Is moving: " + String(bMoveToPos));
    // move to target if it differs from position and move to pos is enabled
    if(target < position && bMoveToPos && !bStop){
        position-=1;
        //delayMicroseconds(100);
        step(-1);
        //Serial.println("-1 Step");
    }else if(target > position && bMoveToPos && !bStop) {
        position+=1;
        //delayMicroseconds(100);
        step(1);
    }else if(!bStop && bMoveToPos){
      // release drive            
      saveToEEPROM(EEPROM_POS_ADDRESS,position); // Save position for later
      //delay(50);
      releaseDrive();
      bMoveToPos=false;
    }else if(bMoveToPos){      
      saveToEEPROM(EEPROM_POS_ADDRESS,position); // Save position for later
      //delay(50);
      releaseDrive();
      bMoveToPos=false;
      bStop=false;
    }

    if(LastPosSaveToEEPROMTime+POSSAVETOEEPROMTIME < ActTime){
      LastPosSaveToEEPROMTime=ActTime;
      if(LastEEPROMPosition !=position){
      saveToEEPROM(EEPROM_POS_ADDRESS,position); // Save position for later
      LastEEPROMPosition=position;
      }
    }

}

void setStepMode(eSTEPMODE mode){
  StepPosition=1;
  StepperMode=mode;
  // Save mode to EEPROM
  writeToEEPROM(EEPROM_MODE_ADDRESS, mode);
  LOG_INFO("Stepper mode set to: " + String(mode));
  }
  
  eSTEPMODE getStepMode() {
    return StepperMode;
  }
  
  void setPins(int pin1, int pin2, int pin3, int pin4) {
    // Reconfigure old pins as inputs to release them
    pinMode(ULN2003_Pin1, INPUT);
    pinMode(ULN2003_Pin2, INPUT);
    pinMode(ULN2003_Pin3, INPUT);
    pinMode(ULN2003_Pin4, INPUT);
    
    // Set new pins
    ULN2003_Pin1 = pin1;
    ULN2003_Pin2 = pin2;
    ULN2003_Pin3 = pin3;
    ULN2003_Pin4 = pin4;
    
    // Configure new pins as outputs
    pinMode(ULN2003_Pin1, OUTPUT);
    pinMode(ULN2003_Pin2, OUTPUT);
    pinMode(ULN2003_Pin3, OUTPUT);
    pinMode(ULN2003_Pin4, OUTPUT);
    
    // Save to EEPROM
    savePinsToEEPROM();
    
    LOG_INFO("Stepper pins updated - Pin1: " + String(pin1) + " Pin2: " + String(pin2) + 
             " Pin3: " + String(pin3) + " Pin4: " + String(pin4));
  }
  
  int getPin1() { return ULN2003_Pin1; }
  int getPin2() { return ULN2003_Pin2; }
  int getPin3() { return ULN2003_Pin3; }
  int getPin4() { return ULN2003_Pin4; }


void releaseDrive(){
    //digitalWrite(PIN_ENABLE, true);
  digitalWrite(ULN2003_Pin1, false);
  digitalWrite(ULN2003_Pin2, false);
  digitalWrite(ULN2003_Pin3, false);
  digitalWrite(ULN2003_Pin4, false);
};


private:

void saveToEEPROM(int addr, long pos){
  EEPROM.put(addr, pos);
  EEPROM.commit(); // Ensure data is written to EEPROM
};

long readFromEEPROM(int addr ){
 long value;
 EEPROM.get(addr, value);
 return value;
};

void writeToEEPROM(int addr, long value){
  EEPROM.put(addr, value);
  EEPROM.commit();
};

void loadPinsFromEEPROM() {
  // Check validation marker
  byte validMarker = EEPROM.read(EEPROM_PINS_VALID_MARKER);
  
  if(validMarker == 0xAA) {
    // Valid pins stored in EEPROM
    ULN2003_Pin1 = EEPROM.read(EEPROM_PINS_ADDRESS);
    ULN2003_Pin2 = EEPROM.read(EEPROM_PINS_ADDRESS + 1);
    ULN2003_Pin3 = EEPROM.read(EEPROM_PINS_ADDRESS + 2);
    ULN2003_Pin4 = EEPROM.read(EEPROM_PINS_ADDRESS + 3);
    
    // Validate pin numbers (ESP8266 GPIO 0-16)
    if(ULN2003_Pin1 > 16 || ULN2003_Pin2 > 16 || ULN2003_Pin3 > 16 || ULN2003_Pin4 > 16) {
      LOG_WARN("Invalid pin values in EEPROM, using defaults");
      setDefaultPins();
      savePinsToEEPROM();
    } else {
      LOG_INFO("Loaded stepper pins from EEPROM");
    }
  } else {
    LOG_INFO("No valid pins in EEPROM, using defaults");
    setDefaultPins();
    savePinsToEEPROM();
  }
}

void setDefaultPins() {
  ULN2003_Pin1 = DEFAULT_ULN2003_Pin1;
  ULN2003_Pin2 = DEFAULT_ULN2003_Pin2;
  ULN2003_Pin3 = DEFAULT_ULN2003_Pin3;
  ULN2003_Pin4 = DEFAULT_ULN2003_Pin4;
}

void savePinsToEEPROM() {
  EEPROM.write(EEPROM_PINS_ADDRESS, ULN2003_Pin1);
  EEPROM.write(EEPROM_PINS_ADDRESS + 1, ULN2003_Pin2);
  EEPROM.write(EEPROM_PINS_ADDRESS + 2, ULN2003_Pin3);
  EEPROM.write(EEPROM_PINS_ADDRESS + 3, ULN2003_Pin4);
  EEPROM.write(EEPROM_PINS_VALID_MARKER, 0xAA);
  EEPROM.commit();
  LOG_INFO("Saved stepper pins to EEPROM");
}

void move(long val)
{
  if(val > 0){
    step(SPEED_FAST);
  }else{
    step(-SPEED_FAST);  
  }
};


void step(int val)
{
  if (StepperMode == HalfStep)
  {
    MaxPositions = 8;
  }
  else if (StepperMode == EighthStep)
  {
    MaxPositions = 8;
  }
  else if (StepperMode == QuaterStep)
  {
    MaxPositions = 16;
  }
  else 
  {
    MaxPositions = 4;
  }

  for(int n = 0; n < abs(val)+1; n++) {

    switch (StepperMode)
    {
      case FullStep:
          //Serial.println("Fullstep");
          SetOutputToDriveFullSteps(StepPosition);
          break;
      case FullStep2Phase:
          SetOutputToDriveFullSteps2Phase(StepPosition);
          break;
      case HalfStep:
          SetOutputToDriveHalfSteps(StepPosition);
          break;
      case QuaterStep:
          SetOutputToDriveQuaterSteps(StepPosition);
          break;
      case EighthStep:
          SetOutputToDriveEighthSteps(StepPosition);
          break;
      case SixteenthStep:
          SetOutputToDriveSixteenthSteps(StepPosition);
          break;
    }

    if(val>0){
    StepPosition++;
    }else{
      StepPosition--;
    }
    if(StepPosition > MaxPositions)
    {
      StepPosition = 1;
    }else if(StepPosition < 1){
      StepPosition=MaxPositions;
    }
    delayMicroseconds(pulseWidthMicrosULN2003);
  }
};

void SetOutputToDriveFullSteps(int step)
{
  switch (step)
     {
         case 1:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 2:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 3:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 4:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, true);
             break;
     }
};


 void SetOutputToDriveHalfSteps(int step)
 {
     switch (step)
     {
         case 1:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 2:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 3:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 4:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 5:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 6:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, true);
             break;
         case 7:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, true);
             break;
         case 8:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, true);
             break;
         default:
            // Console.WriteLine("SetOutputToDriveHalfSteps -> Unknown Step.");
             break;

     }

 };

 void SetOutputToDriveFullSteps2Phase(int step)
 {
     switch (step)
     {
         case 1:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 2:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 3:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, true);
             break;
         case 4:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, true);
             break;
         default:
            // Console.WriteLine("SetOutputToDriveFullSteps2Phase -> Unknown Step.");
             break;

     }

 };

 void SetOutputToDriveQuaterSteps(int step)
 {
     // Quarter stepping: 16 positions per full cycle
     switch (step)
     {
         case 1:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 2:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 3:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 4:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 5:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 6:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 7:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 8:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, true);
             break;
         case 9:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, true);
             break;
         case 10:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, true);
             break;
         case 11:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, true);
             break;
         case 12:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, true);
             break;
         case 13:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, false);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 14:
             digitalWrite(ULN2003_Pin1, true);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 15:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, false);
             digitalWrite(ULN2003_Pin4, false);
             break;
         case 16:
             digitalWrite(ULN2003_Pin1, false);
             digitalWrite(ULN2003_Pin2, true);
             digitalWrite(ULN2003_Pin3, true);
             digitalWrite(ULN2003_Pin4, false);
             break;
         default:
             break;
     }
 };

 void SetOutputToDriveEighthSteps(int step)
 {
     // Eighth stepping: uses same 8-position sequence as half step
     // ULN2003 cannot do true microstepping beyond this
     SetOutputToDriveHalfSteps(step);
 };

 void SetOutputToDriveSixteenthSteps(int step)
 {
     // Sixteenth stepping: uses full step sequence
     // ULN2003 binary driver limitation
     SetOutputToDriveFullSteps(step);
 };

};

#endif /* A5BDF2AE_8CDD_4DFA_B4AE_E98D3FB81323 */
