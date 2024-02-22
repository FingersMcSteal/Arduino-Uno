#include <Arduino.h>

#include "RTClib.h"
#include <SPI.h>

RTC_DS3231 rtc;
int Year;
int Month;
int Day;
int Hour;
int Minute;
int Second;

// Using 6 seperate numbers to reference the 'datArray' numbering to display the correct digits-> H H : M M : S S
// This is sent to the 595's to display the correct time in the correct format

int HourFirstDigit = 0;     // First digit for the hour display
int HourSecondDigit = 0;    // Second digit for the hour display
int MinuteFirstDigit = 0;   // First digit for the minute display
int MinuteSecondDigit = 0;   // Second digit for the minute display
int SecondFirstDigit = 0;   // First digit for the seond display
int SecondSecondDigit = 0;  // Second digit for the second display

// The seconds Dots are wired on the first Hour digit chip
// This needs to be logical AND to the first HOUR number to turn them on / off

bool DotFlash = false;
unsigned long DotFlashTime = 0;
unsigned long DotFlashDelay = 500; // How fast the dots flash in MS

// Buttons, Hour, Minutes & Seconds Hold (Holds on zero) button in order
const int B1_BUTTON_PIN = 5;
int B1_buttonState = 0;
unsigned long B1_LastDebounceTime =0;
unsigned long B1_DebouceDelay = 50;

const int B2_BUTTON_PIN = 6;
int B2_buttonState = 0;
unsigned long B2_LastDebounceTime =0;
unsigned long B2_DebouceDelay = 50;

const int B3_BUTTON_PIN = 7;
int B3_buttonState = 0;
unsigned long B3_LastDebounceTime =0;
unsigned long B3_DebouceDelay = 50;

// Define Connections to 74HC595
const int latchPin = 10;
const int clockPin = 11;
const int dataPin = 12;
 
// Patterns for characters 0,1,2,3,4,5,6,7,8,9
int datArray[10] = {B11111100, B01100000, B11011010, B11110010, B01100110, B10110110, B10111110, B11100000, B11111110, B11110110};

void UpdateTime()
{
  // Updates: Hour, Minute & Second
  DateTime now = rtc.now();

  Day = now.day();
  Month = now.month();
  Year = now.year();

  Hour = now.hour();
  Minute = now.minute();
  Second = now.second();
}

void UpdateLEDS ()
{
  digitalWrite(latchPin, LOW);
  
  shiftOut(dataPin, clockPin, MSBFIRST, datArray[HourFirstDigit]);

  // The 5th 595 chip (Second Hour digit chip) is connected to the dots, this switches it on or off
  if (!DotFlash)
  {
    shiftOut(dataPin, clockPin, MSBFIRST, datArray[HourSecondDigit]);
  }
  else
  {
    shiftOut(dataPin, clockPin, MSBFIRST, datArray[HourSecondDigit] ^ 0B00000001); // XOR the dots on or off
  }

  shiftOut(dataPin, clockPin, MSBFIRST, datArray[MinuteFirstDigit]);    // Orange Digit
  shiftOut(dataPin, clockPin, MSBFIRST, datArray[MinuteSecondDigit]);   // Orange Digit
  shiftOut(dataPin, clockPin, MSBFIRST, datArray[SecondFirstDigit]);    // Green Digit
  shiftOut(dataPin, clockPin, MSBFIRST, datArray[SecondSecondDigit]);   // Green Digit
  
  digitalWrite(latchPin, HIGH);
}

void SecondsDisplaySetup()
{
  if (Second < 10)
  {
    SecondFirstDigit = 0;
    SecondSecondDigit = Second;
  }
  else
  {
    if (Second >= 10 && Second <= 19)
    {
      SecondFirstDigit = 1;
      SecondSecondDigit = Second - 10;
    }
    else
    {
      if (Second >= 20 && Second <= 29)
      
      {
        SecondFirstDigit = 2;
        SecondSecondDigit = Second - 20;
      }
      else
      {
        if (Second >= 30 && Second <= 39)
        
        {
          SecondFirstDigit = 3;
          SecondSecondDigit = Second - 30;
        
        }
        else
        {
          if (Second >= 40 && Second <= 49)
          {
            SecondFirstDigit = 4;
            SecondSecondDigit = Second - 40;
          }
          else
          {
            if (Second >= 50 && Second <= 59)
            {
              SecondFirstDigit = 5;
              SecondSecondDigit = Second - 50;
            }
          }
        }
      }
    }
  }
}

void MinutesDisplaySetup()
{
  if (Minute < 10)
  {
    MinuteFirstDigit = 0;
    MinuteSecondDigit = Minute;
  }
  else
  {
    if (Minute >= 10 && Minute <= 19)
    {
      MinuteFirstDigit = 1;
      MinuteSecondDigit = Minute - 10;
    }
    else
    {
      if (Minute >= 20 && Minute <= 29)
      
      {
        MinuteFirstDigit = 2;
        MinuteSecondDigit = Minute - 20;
      }
      else
      {
        if (Minute >= 30 && Minute <= 39)
        
        {
          MinuteFirstDigit = 3;
          MinuteSecondDigit = Minute - 30;
        
        }
        else
        {
          if (Minute >= 40 && Minute <= 49)
          {
            MinuteFirstDigit = 4;
            MinuteSecondDigit = Minute - 40;
          }
          else
          {
            if (Minute >= 50 && Minute <= 59)
            {
              MinuteFirstDigit = 5;
              MinuteSecondDigit = Minute - 50;
            }
          }
        }
      }
    }
  }
}

void HoursDisplaySetup()
{
  if (Hour < 10)
  {
    HourFirstDigit = 0;
    HourSecondDigit = Hour;
  }
  else
  {
    if (Hour >=10 && Hour <= 19)
    {
      HourFirstDigit = 1;
      HourSecondDigit = Hour - 10;
    }
    else
    {
      if (Hour >=20 && Hour <=23)
      {
        HourFirstDigit = 2;
        HourSecondDigit = Hour - 20;
      }
    }
  }
  // Flash to dot code in here
}

void UpdateDots()
{
  if ((millis() - DotFlashTime) > DotFlashDelay)
  {
    // Flash delay reached, change dots
    // Flip True / False state
    if (!DotFlash)
    {
      DotFlash = true;
      DotFlashTime = millis();
    }
    else
    {
      DotFlash = false;
      DotFlashTime = millis();
    }
  }
}

void CheckControlButtons()
{
  int B1_Reading = digitalRead (B1_BUTTON_PIN);
  if (B1_Reading != B1_buttonState)
  {
    B1_LastDebounceTime = millis();
  }

  if ((millis() - B1_LastDebounceTime) > B1_DebouceDelay)
  {
    B1_buttonState = B1_Reading;
    if (B1_buttonState == 1)
    {
      // Do This...
      DateTime now = rtc.now();
      Hour = (now.hour());

      Hour = Hour +1;
      if (Hour > 23)
      {
        Hour = 0;
      }
        rtc.adjust(DateTime(Year, Month, Day, Hour, Minute, Second));
    }
  }
  B1_buttonState = B1_Reading;

  int B2_Reading = digitalRead (B2_BUTTON_PIN);
  if (B2_Reading != B2_buttonState)
  {
    B2_LastDebounceTime = millis();
  }

  if ((millis() - B2_LastDebounceTime) > B2_DebouceDelay)
  {
    B2_buttonState = B2_Reading;
    if (B2_buttonState == 1)
    {
      // Do This...
      DateTime now = rtc.now();
      Minute = (now.minute());

      Minute = Minute +1;
      if (Minute > 59)
      {
        Minute = 0;
      }
        rtc.adjust(DateTime(Year, Month, Day, Hour, Minute, Second));
    }
  }
  B2_buttonState = B2_Reading;

  int B3_Reading = digitalRead (B3_BUTTON_PIN);
  if (B3_Reading != B3_buttonState)
  {
    B3_LastDebounceTime = millis();
  }

  if ((millis() - B3_LastDebounceTime) > B3_DebouceDelay)
  {
    B3_buttonState = B3_Reading;
    if (B3_buttonState == 1)
    {
      // Do This...
      rtc.adjust(DateTime(Year, Month, Day, Hour, Minute, 0));
    }
  }
  B3_buttonState = B3_Reading;
  // Serial.println (Hour);
}

void setup ()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

    if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Setup pins as Outputs
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  // Control Buttons as Inputs
  pinMode(B1_BUTTON_PIN, INPUT);            // Pin 5
  pinMode(B2_BUTTON_PIN, INPUT);            // Pin 6
  pinMode(B3_BUTTON_PIN, INPUT);            // Pin 7
}

void loop()
{
  // Grabs RTC values & updates the time
  UpdateTime();
  UpdateDots();
  CheckControlButtons();

  SecondsDisplaySetup();
  MinutesDisplaySetup();
  HoursDisplaySetup();

  UpdateLEDS();

  delay (200);
}