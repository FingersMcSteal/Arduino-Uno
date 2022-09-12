#include <Arduino.h>

#include <DFRobot_RGBLCD1602.h>
#include <SD.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Hardware & Variable Setup
int LED_SD_ERROR = 5;           // OUTPUT, Red LED on if theres SD or file errors
int LED_LCD_UPDATE = 6;         // OUTPUT, Green blink each time the LCD is updated
int LED_SD_CARD_ACTIVITY = 7;   // OUTPUT, Blue blink when update to SD Card or Blue always on with NO SD Card inserted

String data_String;     // Used in the main loop if the SD card is installed for holding the string to send to the SD card once it's built up
int log_CounterS = 0;   // Counter for file logging: Seconds, Minutes, Hours, Days
int log_CounterM = 0;
int log_CounterH = 0;
int log_CounterD = 0;

// Light sensor & LCD brightness variables
int lcdColour = 0;
int lcd_Display_State =0;
int LCD_Brightness = 128;

const int B1_BUTTON_PIN = 8;    // INPUT, Button to cycle throught LCD background colours
const int B2_BUTTON_PIN = 9;    // INPUT, Button to LCD display modes

int B1_buttonState = 0;
int B2_buttonState = 0;

DFRobot_RGBLCD1602 lcd(16,2);   // Display on I2C Bus
Adafruit_BME280 bme;            // Sensor on I2C Bus
#define SEALEVELPRESSURE_HPA (1013.25)  // 1013.25 is a standard setting for the pressure

unsigned bme_Status;    // Used in the setup to determine if the sensor starts up (BME280)

float t;  // Used globally to store temperature rather than polling the sensor whenever i need these figures
float h;  // Humidity (same as above)
float p;  // Pressure (same as above)
float a;  // Altitude (same as above)

const int chipSelect = 4;   // SD Card CS Pin
bool CARD_PRESENT = false;  // Set false until we know if theres a MicroSD card installed or not

void Light_Sensor()
{
  LCD_Brightness = analogRead(A0);
  LCD_Brightness = map(LCD_Brightness, 0, 1023, 20, 255);

  if (lcdColour ==0)
  {
    lcd.setRGB(LCD_Brightness, 0, 0);                 // Red background, LDR controlled brightness
  }
  if (lcdColour == 1)
  {
    lcd.setRGB(0,LCD_Brightness,0);                   // Green background, LDR controlled brightness
  }
  if (lcdColour == 2)
  {
    lcd.setRGB(LCD_Brightness,0,LCD_Brightness);      // Blue background (Red Text), LDR controlled brightness... ewwww !
  }

  if (lcdColour == 3)
  {
    lcd.setRGB(200, 128, 64);                         // 4th background colour option (Fix your eyes after option 3)
  }
}

void Read_Sensor_Values(float, float, float, float)   // Function to read the BME280 and store results into variables t, h, p and a
{
  t = bme.readTemperature();
  h = bme.readHumidity();
  p = (bme.readPressure() / 100.0F);
  a = (bme.readAltitude(SEALEVELPRESSURE_HPA));
}

void Send_To_Serial(float, float, float, float)       //Function to display sensor information in the serial window, Blinks the green LED when this happens
{
  digitalWrite(LED_LCD_UPDATE, HIGH);                 // Green LED on
  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.println(" °C");

  Serial.print("Pressure = ");
  Serial.print(p);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(a);
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(h);
  Serial.println(" %");

  Serial.println();
  digitalWrite(LED_LCD_UPDATE, LOW);                  // Green LED off
}

void Update_LCD_Display(float, float, float, float)     // Function to update only the values on the LCD, avoiding constantly clearing the LCD and flickering
{
  if (lcd_Display_State == 0)
  {
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.setCursor(2,0);
    lcd.print(t,1);       // Display Temperature to 1 decimal point 'xx.x'

    lcd.setCursor(7,0);
    lcd.print("P:");
    lcd.setCursor(9,0);
    lcd.print(p,3);       // Display Pressure to 3 decimal points 'xxxx.xxx'

    lcd.setCursor(0,1);
    lcd.print("H:");
    lcd.setCursor(2,1);
    lcd.print(h,1);       // Display Humidity to 1 decimal point 'xx.x'

    lcd.setCursor(7,1);
    lcd.print("A:");
    lcd.setCursor(9,1);
    lcd.print(a,2);       // Display Altitude to 2 decimal points 'xxxx.xx'
  }

  if (lcd_Display_State == 1)
  {
    lcd.setCursor (0, 0);
    lcd.print("- Temperature -");
    lcd.setCursor(5, 1);
    lcd.print(t);
    lcd.setCursor(11,1);
    lcd.print("C");
  }

  if (lcd_Display_State == 2)
  {
    lcd.setCursor (0, 0);
    lcd.print("--- Humidity ---");
    lcd.setCursor(5, 1);
    lcd.print(h);
    lcd.setCursor(11,1);
    lcd.print("%");
  }

  if (lcd_Display_State == 3)
  {
    lcd.setCursor (0, 0);
    lcd.print("--- Pressure ---");
    lcd.setCursor(3, 1);
    lcd.print(p);
    lcd.setCursor(11,1);
    lcd.print("hPa");
  }

  if (lcd_Display_State == 4)
  {
    lcd.setCursor (0, 0);
    lcd.print("--- Altitude ---");
    lcd.setCursor(5, 1);
    lcd.print(a);
    lcd.setCursor(11,1);
    lcd.print("m");
  }

  if (lcd_Display_State == 5)   // Displays the SD Cards logging counters, no card in will not be counting
  {
    lcd.setCursor(0, 0);
    lcd.print("D:");
    lcd.setCursor(2,0);
    lcd.print(log_CounterD);

    lcd.setCursor(0, 1);
    lcd.print("H:");
    lcd.setCursor(2,1);
    lcd.print(log_CounterH);

    lcd.setCursor(6, 0);
    lcd.print("M:");
    lcd.setCursor(8,0);
    lcd.print(log_CounterM);

    lcd.setCursor(6, 1);
    lcd.print("S:");
    lcd.setCursor(8,1);
    lcd.print(log_CounterS);
  }
}

String DATA_TO_STRING()           // Function to build up the data string thats sent to the SD card, called from the main loop
{
  String the_Data = "";           // Internal to this function to make a string thats eventually sent to the SD card

  log_CounterS++;                 // General counter, there will be a limit but for general use nothing to worry about

  if (log_CounterS >=60)
  {
    log_CounterS = 0;
    log_CounterM++;
    if (log_CounterM >=60)
    {
      log_CounterM = 0;
      log_CounterH++;
      if (log_CounterH >=24)
      {
        log_CounterH = 0;
        log_CounterD++;
        if (log_CounterD >=365)
        {
          log_CounterD = 0;
        }
      }
    }
  }

  the_Data = String (String (log_CounterD) + ":" + String (log_CounterH) + ":" + String (log_CounterM) + ":" + String (log_CounterS));

  the_Data += (", T-H-P-A, ");
  the_Data += String(t);
  the_Data += (", ");
  the_Data += String(h);
  the_Data += (", ");
  the_Data += String (p);
  the_Data += (", ");
  the_Data += (a);
  the_Data += (", END");

  return(the_Data);     // Sends a string back to where it was called from (main loop)
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);      // Wait for serial to start
  Serial.println();
  Serial.println("Seial OK.");
  
  // LED start, all off
  pinMode (LED_SD_ERROR, OUTPUT);           // Pin 5
  pinMode (LED_LCD_UPDATE, OUTPUT);         // Pin 6
  pinMode (LED_SD_CARD_ACTIVITY, OUTPUT);   // Pin 7

  pinMode(B1_BUTTON_PIN, INPUT);            // Pin 8
  pinMode(B2_BUTTON_PIN, INPUT);            // Pin 9

  digitalWrite (LED_SD_ERROR, LOW);
  digitalWrite (LED_LCD_UPDATE, LOW);
  digitalWrite (LED_SD_CARD_ACTIVITY, LOW);

  // Init LCD
  lcd.init();
  lcd.clear();
  lcd.setCursor (0, 1);
  lcd.setRGB (200,128,64);
  Serial.println("LCD OK.");

  // Init Sensor BME280
  bme_Status = bme.begin(); // Sensor startup
  if (!bme_Status)
  {
    // If the sensor startup fails...
    Serial.println();
    Serial.println("Could not find a valid BME280 sensor");

    lcd.setCursor(0, 1);
    lcd.setRGB(255,0,0);
    lcd.print("Sensor ERROR !!!");
    digitalWrite(LED_SD_ERROR, HIGH);
    while (1) delay (10);  // STOP HERE !!! Sensor not working so we can't go on.
  }

  // Quick blink of ALL LED's just to check there working
  digitalWrite(LED_LCD_UPDATE, HIGH);
  digitalWrite(LED_SD_CARD_ACTIVITY, HIGH);
  digitalWrite(LED_SD_ERROR, HIGH);
  delay (500);
  digitalWrite(LED_LCD_UPDATE, LOW);
  digitalWrite(LED_SD_CARD_ACTIVITY, LOW);
  digitalWrite(LED_SD_ERROR, LOW);

  // Init SD card
  if (!SD.begin(chipSelect))
  {
    CARD_PRESENT = false;
    Serial.println("MicroSD NO CARD!");
    digitalWrite(LED_SD_CARD_ACTIVITY, HIGH);   // Blue LED is set to ON, it remains ON to show there is no SD card installed
  }
  else
  {
    CARD_PRESENT = true;
    Serial.println("MicroSD OK.");
    Serial.println();
  }

  if(CARD_PRESENT)  // If the SD cards IN
  {
    digitalWrite(LED_SD_CARD_ACTIVITY, HIGH);   // Blue LED on to Signal SD card activity

    // This next part will either, CREATE the file (if it's not there), or OPEN an exsisting file then close it
    File data_File = SD.open("LogData.txt", FILE_WRITE);
    data_File.close();
    Serial.println("File OK.");
    digitalWrite(LED_SD_CARD_ACTIVITY, LOW);    // Blue LED off, finished SD card activity
  }
  else
  {
    // If there wasn't an SDcard installed this code runs
    Serial.println("NO LOGGING!");
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  Light_Sensor();               // Reads the LDR on Pin A0 and sets the LCD brightness (bright room = bright LCD / dark room = dark LCD)
  Read_Sensor_Values(t,h,p,a);  // Reads the BME280 and stores data in variables t, h, p, a
  Send_To_Serial(t,h,p,a);      // Sends the sensor read data to the serial window (also blinks the green LED)
  Update_LCD_Display(t,h,p,a);

  if(CARD_PRESENT)              // If theres a SD card installed this runs, if not it's skipped (Blue LED will remain ON with no SD card installed)
  {
    data_String = DATA_TO_STRING();
    Serial.print(data_String);
    Serial.println();

    digitalWrite(LED_SD_CARD_ACTIVITY, HIGH);
    File data_File = SD.open("LogData.txt", FILE_WRITE);
    if (data_File)
    {
      data_File.println(data_String);
      data_File.close();
    }
    else
    {
      digitalWrite(LED_SD_ERROR, HIGH);           // If the SD card is removed it triggers a red LED, Press the RESET button to start over
      Serial.println("Error opening the file.");
    }
    digitalWrite(LED_SD_CARD_ACTIVITY, LOW);
  }

  B1_buttonState = digitalRead(B1_BUTTON_PIN);
  if(B1_buttonState == HIGH)
  {
    lcdColour++;
    if (lcdColour == 4)           // LCD background colours
    {
      lcdColour = 0;
    }
  }

  B2_buttonState = digitalRead(B2_BUTTON_PIN);
  if(B2_buttonState == HIGH)
  {
    lcd.clear();
    lcd_Display_State++;
    if (lcd_Display_State == 6)   // LCD mode cycle control
    {
      lcd_Display_State = 0;
    }
  }
  delay (1000);
}