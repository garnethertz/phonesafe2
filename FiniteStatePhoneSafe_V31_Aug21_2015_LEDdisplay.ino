//http://www.noveldevices.co.uk/ardl/timer.ino

#include <EEPROM.h>

#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#include <SimpleTimer.h>
#include <SM.h>
#include <SoftwareSerial.h>

#include <Servo.h> // **** added GDH Aug 11 ****

#define MAX_OUT_CHARS 16  //max nbr of characters to be sent on any one serial command

SM Sequence(credits);
SM Machine(unlock);

SimpleTimer timer;

//--------Global Variables-----------//

// 16x2 Display specific variables:
SoftwareSerial LCD(8,9); // pin 7 = RX (unused), pin 8 = TX // Maybe issues?
// LED alphanumeric display will be 10 (CLOCK) and 11 (DATA)
// LED alphanumeric display will have 16 characters, not 12 characters
int numgen = 0;
char tempstring[10];

const int phoneSwitch = 6;
boolean psCurrentState = LOW; //Stores phoneswitch state
boolean psPrevious = LOW; //Stores phoneswitch previous state

const int timeButton = 7;

const int drum = 11;
boolean dCurrentState = LOW; //Stores phoneswitch state
boolean dPrevious = LOW; //Stores phoneswitch previous state

const int door = 10;
boolean dsCurrentState = LOW; //Stores phoneswitch state
boolean dsPrevious = LOW; //Stores phoneswitch previous state


const int lockPin = 3; // was used for solenoid, repurposed for servo below
//const int ledPin =  13;
const int timeInc = 1;

Servo myservo;  // create servo object to control a servo - **** added GDH Aug 11 ****



//not constant

int timeLeft = 0; // stores simple timer value added by push button.

boolean dsHits = 0;
int psHits = 0;
int rotations = 0;

int overrideCount = 0;  // Used to time the button override on the insert phone state

int timeCount = 0;
int buttonIn = 0;
boolean timeButtonState = LOW;
boolean timePreviousButtonState = LOW;
//int timeLeft = 0;
int timeSpacer = 0;  //spacer placeholder to decrement time slower

boolean psToggleState = 0;

//int startProgram = 0;
int incTimeBut = 50; // Increment time: every 500ms
int decTime = 1000; // Decrease time: every 1000ms 


String message = "     STARTUP     ";
String FXmessage = "     STARTUP     ";
const int mLength = 17;// display size

int creditState = 1;
unsigned long creditInterval = 2500; // Changed from 3000 to 1000 on 20 Aug 2015
unsigned long previousMillis = 0;

int totalPhones = 0;
int addr = 0;
int previousPhones = 0;
boolean resetTotal = false;

boolean unlocked = false;

Adafruit_AlphaNum4 alpha1 = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 alpha2 = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 alpha3 = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();


char   buffer[MAX_OUT_CHARS + 1];  // buffer used to format a line (+1 is for trailing 0)

//------Setup------//


void setup() 
{ 
  if (resetTotal == false)
  {
    previousPhones = EEPROM.read(addr);
    totalPhones = previousPhones+totalPhones;
  }
  else
  {
    EEPROM.write(addr, 0);
  }
  
  delay(1000); // wait for display to boot up
  Serial.begin(9600);
  
  //LCD Startup
  LCD.begin(9600);
  backlight(140);
  delay(1000);
  clearScreen();
  
  Startup();
  
  //Pin Config
  pinMode(phoneSwitch, INPUT); // sets phoneswitch as input
  pinMode(timeButton, INPUT);
  pinMode(lockPin, OUTPUT);
  //digitalWrite(lockPin, LOW); 
  
  alpha1.begin(0x70); // pass in the address
  alpha2.begin(0x71);
  alpha3.begin(0x72);
  alpha4.begin(0x73);// new display 20 May 2015

  myservo.attach(3);  // attaches the servo on pin 3 to the servo object - **** added GDH Aug 11 ****
  
  //When case first starts up make door open then close as a failsafe
  myservo.write(0);
  delay(1000);
  myservo.write(80);
  Machine.Set(lock);
  
  
  Serial.println("Starting up");
  
  
}


void loop()
{
  EXEC(Sequence);
  EXEC(Machine);
  LEDmessenger();  
}


//------Finite State------//

State insertPhone()
{
    //Machine.Set(unlock);
    clearScreen();
    selectLineOne();  
    LCD.print("Insert Phone");
    message = ("  INSERT PHONE  ");  
    phoneInserted();
    doorCheck();
    
    

    //if phone is removed during this stage rotation must not occur!!!!
    //similar to door shutout...
    
    //invert phone switch logic...
    
    if (psHits >= 1 && dsHits == 1)
    { 
      Sequence.Set(rotateDrum);
      //Machine.Set(lock);
    }
    else if (dsHits == 0)
    {
      Sequence.Set(shutDoor);
    }
}

State shutDoor() 
{
  clearScreen();
  selectLineOne();  
  LCD.print("Shut Door");
  
  message = ("   SHUT DOOR    ");  
  
  doorCheck();
  
  if (dsHits == 1)
  { 
    Sequence.Set(insertPhone);
  }  
}

State rotateDrum()
{
    clearScreen();
    selectLineOne();  
    LCD.print("Rotate Drum");
    message = ("  ROTATE DRUM   ");  

    hopperRotated();
    doorCheck();
   
    if (rotations >= 1 && dsHits == 1)
    {
      totalPhones = totalPhones +1;
      Sequence.Set(addTime);
    }
    
    if (digitalRead(phoneSwitch) == LOW)
    {
      Sequence.Set(insertPhone);
    }
    
    psHits = 0;
}

State addTime()
{
    clearScreen();
    selectLineOne();
    timerPressed();  
    LCD.print("Add Time");
    message = ("    ADD TIME    ");
    if (buttonIn > 0)
    {
      Sequence.Set(countdown);
    }
    rotations = 0;
}

State countdown()
{
  timerPressed();
  increaseTime();
  decreaseTime();
  
  if (timeLeft == 0)
  {
    EEPROM.write(addr, totalPhones);
    buttonIn = 0; 
    clearScreen();
    selectLineOne();  
    LCD.print("THANK YOU");
     if ((unsigned long) (millis()-previousMillis) >= 1000)
     {
       previousMillis = millis();
       creditState = creditState+1;
     }
  
    switch (creditState)
    { 
      case 1:
        clearScreen();
        selectLineOne();  
        LCD.print("THANK YOU!");
        message = ("    THANK YOU    ");
        Machine.Set(unlock);  //Unlock on thankyou
        //creditState = 2;
        break;  

  
      case 2:
        clearScreen();
        selectLineOne();  
        clearScreen();
        LCD.print("TAKE PHONE");
        message = ("   TAKE PHONE   "); 
        //creditState = 3;
        break;
       
      case 3:
        Sequence.Set(credits);
        //Machine.Set(unlock);
        creditState = 0;
        break;
      
    }
  }
}

State credits()
{ 
   if ((unsigned long) (millis()-previousMillis) >= creditInterval)
   {
     previousMillis = millis();
     creditState = creditState+1;
   }
   
   if (creditState > 6)
   {
     creditState = 0;
   }
  
  switch (creditState)
  { 
    case 1:
      clearScreen();
      selectLineOne();  
      LCD.print("Phone Safe 2");
      message = ("  PHONE SAFE 2  ");
      //creditState = 2;
      Machine.Set(lock);  //Lock safe at start of sequence
      
      //Check phone state and skip credits
      phoneInserted();
      if (psHits >= 1)
      { 
        Sequence.Set(insertPhone);
        creditState = 0;
        break;
      }
      break;  

    case 2:
    
      //Skip this case
      creditState = 3;
      
      clearScreen();
      selectLineOne();  
      LCD.print("Phone Safe 2");
      message = ("  PHONE SAFE 2  ");
      //creditState = 2;
      
      //Check phone state and skip credits
      phoneInserted();
      if (psHits >= 1)
      { 
        Sequence.Set(insertPhone);
        creditState = 0;
        break;
      }
      break;  



/*  removing all named credits for now

    case 2:
      clearScreen();
      selectLineOne();  
      clearScreen();
      LCD.print("Garnet Hertz");
      message = ("  GARNET HERTZ  "); 
      //creditState = 3;
      break; 


    case 3:
      clearScreen();
      selectLineOne();  
      LCD.print("Peter Orlowsky");
      message =( "    CODE PJO    "); 
      //creditState = 5;
      break;
 */     
 
    case 3:
      clearScreen();
      selectLineOne();  
      LCD.print("Version 20150821"); // Please change this to reflect the current date
      message = ("VERSION 20150821"); 
      //creditState = 4;
      
      //Check phone state and skip credits
      phoneInserted();
      if (psHits >= 1)
      { 
        Sequence.Set(insertPhone);
        creditState = 0;
        break;
      }
      break;     

 
 
    case 5:
      clearScreen();
      selectLineOne();  
      LCD.print("xxx Phones Eaten"); // This should be updated to reflect stats of how many drum rotations there have been, I think
      sprintf(buffer, "%03d PHONES EATEN", totalPhones);
      message =( buffer); // This should be updated to reflect stats of how many drum rotations there have been, I think
      //creditState = 5;
      
      //Check phone state and skip credits
      phoneInserted();
      if (psHits >= 1)
      { 
        Sequence.Set(insertPhone);
        creditState = 0;
        break;
      }
      break;
    
    
    case 6:
      creditState = 0;
      Sequence.Set(insertPhone);
      break;
  }
}



//-----Subroutines------//

State lock()
{
 //selectLineTwo();  
 // digitalWrite(lockPin, 80); // This solenoid code was readapted 11 Aug 2015
 
 // servo lock added here
 if(unlocked == true) {
 myservo.write(80); // 80 is the locked servo position - **** added GDH Aug 11 ****
 Serial.println("locked w servo");
 unlocked = false;
 }
 //delay(50 ); // this was added to reduce jittery drift due to (apparent) noise
}

State unlock()
{
 
 //selectLineTwo();  
 // digitalWrite(lockPin, 0); // This solenoid code was readapted 11 Aug 2015
 
 // servo unlock added here
 if(unlocked == false) {
 myservo.write(0); // 0 is the unlocked servo position - **** added GDH Aug 11 ****
 Serial.println("unlocked w servo");
 unlocked = true;
 }
 //delay(50); // this was added to reduce jittery drift due to (apparent) noise
  
}

State error()
{
  clearScreen();
  selectLineOne();  
  LCD.print("YOU MUST RESET NOW");
  Machine.Set(unlock);
  Sequence.Finish();
  //Machine.Finish();  
 // jamming check to see if phone chute jammed. 60 sec alert activation 5min error engaged
}


//------Sensor Atributes------//

void doorCheck()
{
  dsCurrentState = digitalRead(door);
  if (dsCurrentState == HIGH) // if switch has just been engaged
  {
    Serial.println("DoorEngaged");
    delay(1);//button debouncing
    dsHits = 1;
  }
  
  else if (psCurrentState == LOW)
  {
    Serial.println("DoorDisengaged");
    delay(1);
    dsHits = 0;
  }
}



void timerPressed()
{
  timeButtonState = digitalRead(timeButton);
  if (timeButtonState == HIGH && timePreviousButtonState == LOW) // if switch has just been engaged
  {
    Serial.println("TimerEngaged");
    delay(1);//button debouncing
    buttonIn = 1;
  }
  
  else if (psCurrentState == LOW && psPrevious == HIGH)
  {
    Serial.println("TimerDisengaged");
    delay(1);
  }
  
  psPrevious = psCurrentState;
}



// decreaseTime() decreases time and calls openSafe() if time equals zero
void decreaseTime() 
{
  if((timeButtonState == LOW) && (buttonIn > 0) && (timeLeft >= 0))
  {      
    //Instead of proper seconds, time spacer increments every run to space out countdown
    if(timeSpacer > 7) {
      timeLeft = timeLeft - (timeInc);
      timeSpacer = 0;
    }
    else  {
      timeSpacer++;
    }
    
    RunCheck();
    Serial.print("Safe Time (s): ");
    Serial.println(timeLeft);
    sprintf(tempstring,"%4d",timeLeft);
    clearScreen();
    selectLineOne();
    LCD.print("Time left: "); 
    LCD.print(tempstring);
    
    sprintf(buffer, " TIME LEFT  %03d ", timeLeft);
    
    message = buffer;
    
  }
}

// when the button is pressed, increment the timer every incTimeBut ms
void increaseTime() 
{
  if((timeButtonState == HIGH) && (timeLeft < 999) && (timeLeft >= 0))
  {
    Serial.print("Safe Time (with button): ");
    Serial.println(timeLeft);
    sprintf(tempstring,"%4d",timeLeft);
    clearScreen();
    selectLineOne();
    LCD.print("Time left: ");  
    LCD.print(tempstring);
    
    timeLeft = timeLeft + (timeInc);
    
    //"%03d"
    
    sprintf(buffer, " TIME LEFT  %03d ", timeLeft);
    
    message = buffer;
  }
}



void phoneInserted()
{
  psCurrentState = digitalRead(phoneSwitch);
  if (psCurrentState == HIGH && psPrevious == LOW) // if switch has just been engaged
  {
    delay(1);//button debouncing
    psHits = psHits+1;
  }
  
  else if (psCurrentState == LOW && psPrevious == HIGH)
  {
    delay(1);
  }
  
  psPrevious = psCurrentState;
}



void hopperRotated()
{
  dCurrentState = digitalRead(drum);
  
  if (dCurrentState == HIGH && dPrevious == LOW) // if switch has just been engaged
  {
    delay(1);//button debouncing
    rotations = rotations+1;
  }
  
  else if (dCurrentState == LOW && dPrevious == HIGH)
  {
    delay(1);
  }
  
  dPrevious = dCurrentState;
}

//------LED Display Specifics-------//

void LEDmessenger()
{
  for (uint8_t i=0; i<mLength; i++) 
  {
    
    alpha1.writeDigitAscii(0, message[0]);
    alpha1.writeDigitAscii(1, message[1]);
    alpha1.writeDigitAscii(2, message[2]);
    alpha1.writeDigitAscii(3, message[3]);
    alpha1.writeDisplay();
    
    alpha2.writeDigitAscii(0, message[4]);
    alpha2.writeDigitAscii(1, message[5]);
    alpha2.writeDigitAscii(2, message[6]);
    alpha2.writeDigitAscii(3, message[7]);
    alpha2.writeDisplay();
    
        
    alpha3.writeDigitAscii(0, message[8]);
    alpha3.writeDigitAscii(1, message[9]);
    alpha3.writeDigitAscii(2, message[10]);
    alpha3.writeDigitAscii(3, message[11]);
    alpha3.writeDisplay();
    
    alpha4.writeDigitAscii(0, message[12]);
    alpha4.writeDigitAscii(1, message[13]);
    alpha4.writeDigitAscii(2, message[14]);
    alpha4.writeDigitAscii(3, message[15]);
    alpha4.writeDisplay();
   
  }
}

void LEDmessengerFX()
{
  for (uint8_t i=0; i<mLength; i++) 
  {
    
    alpha1.writeDigitAscii(0, FXmessage[0]);
    alpha1.writeDigitAscii(1, FXmessage[1]);
    alpha1.writeDigitAscii(2, FXmessage[2]);
    alpha1.writeDigitAscii(3, FXmessage[3]);
    alpha1.writeDisplay();
    
    alpha2.writeDigitAscii(0, FXmessage[4]);
    alpha2.writeDigitAscii(1, FXmessage[5]);
    alpha2.writeDigitAscii(2, FXmessage[6]);
    alpha2.writeDigitAscii(3, FXmessage[7]);
    alpha2.writeDisplay();
    
        
    alpha3.writeDigitAscii(0, FXmessage[8]);
    alpha3.writeDigitAscii(1, FXmessage[9]);
    alpha3.writeDigitAscii(2, FXmessage[10]);
    alpha3.writeDigitAscii(3, FXmessage[11]);
    alpha3.writeDisplay();
    
    alpha4.writeDigitAscii(0, FXmessage[12]);
    alpha4.writeDigitAscii(1, FXmessage[13]);
    alpha4.writeDigitAscii(2, FXmessage[14]);
    alpha4.writeDigitAscii(3, FXmessage[15]);
    alpha4.writeDisplay();
   
  }
}



//------LCD Specifics------//

void Startup() //Wake up, sheeple.
{
  
// the code that was here was moved to the credits section

  //More credits to come.  
}


void selectLineOne()
{ 
  //puts the cursor at line 0 char 0.
  LCD.write(0xFE); //command flag
  LCD.write(128); //position
}

void selectLineTwo()
{  
  //puts the cursor at line 0 char 0.
   //Serial.write(0xFE);   //command flag
   //Serial.write(192);    //position
}

void clearScreen()
{
  //clears the screen, you will use this a lot!
  LCD.write(0xFE);
  LCD.write(0x01); 
}

int backlight(int brightness) // 128 = OFF, 157 = Fully ON, everything inbetween = varied brightnbess 
{
  // this function takes an int between 128-157 and turns the backlight on accordingly
  LCD.write(0x7C); // NOTE THE DIFFERENT COMMAND FLAG = 124 dec
  LCD.write(brightness); // any value between 128 and 157 or 0x80 and 0x9D
}

void writeData(int data)
{
    RunCheck();
    sprintf(tempstring,"%4d", data);
    clearScreen();
    selectLineOne();
    LCD.write(254);
    LCD.write(138);
    LCD.print("Time left: ");   
    LCD.print(tempstring);
}

void RunCheck()
{
  if(timeLeft < 0)
  {
    timeLeft = 0;
  }
}
