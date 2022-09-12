#include <Arduino.h>

#include <Wire.h>
#include <math.h>
#include <DFRobot_RGBLCD1602.h> // #include <Waveshare_LCD1602_RGB.h>, Switched LCD Header file from the original one for functionality
#include <Adafruit_BME280.h>    // Sensor file
#include <Adafruit_Sensor.h>    // Sensor file
#include <SD.h>                 // MicroSD file

// Hardware & Variable Setup
DFRobot_RGBLCD1602 lcd(16,2);           // Display on I2C Bus
Adafruit_BME280 bme;                    // Sensor running on I2C Bus
#define SEALEVELPRESSURE_HPA (1013.25)  // 1013.25

int led_LCD_Update_Blink = 8;   // OUTPUT - Green LED blink when LCD is updated ALSO, used in initial setup to display an 'Everything OK state'
int led_SD_Card_Activity = 9;   // OUTPUT - Orange LED blink when SD card data written or constant on state when theres no SD card
int led_SD_ERROR = 7;           // OUTPUT - Red LED set to ON if there's a file related error

bool lcd_BG_Light = true;   // LCD backlight, different when running with / without the SD card
bool CARD_PRESENT = false;  // Set false until we know if theres a MicroSD card installed or not

String data_String;         // Used in the main loop if the SD card is installed for holding the string to send to the SD card once it's built up
int log_CounterS = 0;        // Counter for file logging: Seconds, Minutes, Hours, Days
int log_CounterM = 0;
int log_CounterH = 0;
int log_CounterD = 0;

float t;  // Used globally to store temperature rather than polling the sensor whenever i need these figures
float h;  // Humidity (same as above)
float p;  // Pressure (same as above)
float a;  // Altitude (same as above)

unsigned bme_Status;    // Used in the setup to determine if the sensor starts up (BME280)

//     SD card attached to SPI bus as follows:
// ** MOSI - pin 11 on Arduino Uno/Duemilanove/Diecimila
// ** MISO - pin 12 on Arduino Uno/Duemilanove/Diecimila
// ** CLK - pin 13 on Arduino Uno/Duemilanove/Diecimila
// ** CS - depends on your SD card shield or module.

// Pin 4 used here for consistency with other Arduino examples
// change this to match your SD shield or module
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN

const int chipSelect = 4; // See above remarks, i originally placed an LED into pin 10, OK for my setup but might not be for others

void lcd_MSG_Startup (int lcd_X, int lcd_Y, String msg) // Function that displays LCD data from passing variables to it
{
  lcd.clear ();
  lcd.setCursor(lcd_X, lcd_Y);
  lcd.print(msg);
}

void lcd_MSG_Setup()  // Function to setup the LCD ready for the loop, trying to avoid flicker by only updating the numbers when needed
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.setCursor(7,0);
  lcd.print("P:");
  lcd.setCursor(0,1);
  lcd.print("H:");
  lcd.setCursor(7,1);
  lcd.print("A:");
}

// void Read_Sensor_Values(float, float, float, float)
void Read_Sensor_Values(float, float, float, float) // Function to read the BME280 and store results into variables t, h, p and a
{
  t = bme.readTemperature();
  h = bme.readHumidity();
  p = (bme.readPressure() / 100.0F);
  a = (bme.readAltitude(SEALEVELPRESSURE_HPA));
}

void Send_To_Serial(float, float, float, float) //Function to display sensor information in the serial window, Blinks the green LED when this happens
{
  digitalWrite(led_LCD_Update_Blink, HIGH);
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
  digitalWrite(led_LCD_Update_Blink, LOW);
}

void Update_LCD_Display(float, float, float, float) // Function to update only the values on the LCD, avoiding constantly clearing the LCD and flickering
{
  lcd.setCursor(2,0);
  lcd.print(t,1);       // Display Temperature to 1 decimal point 'xx.x'
  lcd.setCursor(2,1);
  lcd.print(h,1);       // Display Humidity to 1 decimal point 'xx.x'
  lcd.setCursor(9,0);
  lcd.print(p,3);       // Display Pressure to 3 decimal points 'xxxx.xxx'
  lcd.setCursor(9,1);
  lcd.print(a,2);       // Display Altitude to 2 decimal points 'xxxx.xx'
}

String DATA_TO_STRING() // Function to build up the data string thats sent to the SD card, called from the main loop
{
  String the_Data = ""; // Internal to this function to make a string thats eventually sent to the SD card

  log_CounterS++;        // General counter, there will be a limit but for general use nothing to worry about

  if (log_CounterS >=30)
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

void setup ()
{
  // put your setup code here, to run once:
  // Setup Sequence...
  // Set all LED's to correct states
  pinMode(led_LCD_Update_Blink, OUTPUT);    // pin 8
  digitalWrite(led_LCD_Update_Blink, LOW);
  pinMode(led_SD_Card_Activity, OUTPUT);    // pin 9
  digitalWrite(led_SD_Card_Activity, LOW);
  pinMode(led_SD_ERROR, OUTPUT);            // pin 7
  digitalWrite(led_SD_ERROR, LOW);

  // Init Serial
  Serial.begin (9600);
  while (!Serial);     // Wait for serial
  Serial.println();
  Serial.println("Seial OK.");
  
  // Init LCD
  lcd.init();
  lcd.clear();
  lcd.setCursor (0, 1);
  lcd.setRGB (0,255,0);

  Serial.println("LCD OK.");
  lcd_MSG_Startup (0,0, "Serial OK LCD OK");  // Sends a message to the 'swanky posh' LCD message function (i remembered how to do it ;)

  // Init Sensor BME280
  bme_Status = bme.begin(); // Sensor startup
  if (!bme_Status)
  {
    // If the sensor startup fails...
    Serial.println();
    Serial.println("Could not find a valid BME280 sensor");

    // The lines below were 'remarked' out due to hitting the Arduino's storage capacity (i think), Tip: keep serial messages short, they use RAM !!!
    // I've left these in for additional error code information
    // **********************************************************************************
    // **********************************************************************************
    // Serial.println("Check your wiring, address or sensor ID...");
    // Serial.print("SensorID was: 0x");
    // Serial.println(bme.sensorID(),16);
    // Serial.print("  ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    // Serial.print("  ID of 0x56-0x58 represents a BMP 280,\n");
    // Serial.print("  ID of 0x60 represents a BME 280.\n");
    // Serial.print("  ID of 0x61 represents a BME 680.\n");
    // ***********************************************************************************
    // ***********************************************************************************

    lcd.setCursor(0, 1);
    lcd.setRGB(255,0,0);
    lcd.print("Sensor ERROR !!!");
    while (1) delay (10);  // STOP HERE !!! Sensor not working so we can't go on.
  }

  lcd.setCursor(0, 1);
  Serial.println("Sensor OK.");
  lcd.print ("Sensor OK");

  // Init SD card
  if (!SD.begin(chipSelect))
  {
    CARD_PRESENT = false;
    lcd_BG_Light = false; // Used to determine what the LCD's backlight colour was going to be

    Serial.println("MicroSD NO CARD!");
    lcd.setCursor(10, 1);
    lcd.setRGB(255,0,0);
    lcd.print("SD ERR");

    // IF we get here, light SD Card LED (Orange on pin 9)
    // SD card LED will remain illuminated because there's no MicroSD to save data to
    digitalWrite(led_SD_Card_Activity, HIGH);
  }
  else
  {
    CARD_PRESENT = true;
    lcd_BG_Light = true;

    lcd.setCursor (10, 1);
    lcd.setRGB (0,255,0);
    lcd.print ("SD OK");
    Serial.println("MicroSD OK.");
    Serial.println();
  }

  // Good startup to here, brief pause to allow the user to view LCD information
  delay (2000);

  // A quick LED check for the user to check BOTH led's are working
  digitalWrite(led_LCD_Update_Blink, HIGH);
  digitalWrite(led_SD_Card_Activity, HIGH);
  digitalWrite(led_SD_ERROR, HIGH);

  Serial.println("x3 LED's ON.");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Are The LED's ON?");
  lcd.setCursor(7,1);
  lcd.print("???");

  delay (5000);  // STOP HERE, 5 second delay to allow user to read LCD screen message, check ALL led's are working
  digitalWrite(led_LCD_Update_Blink, LOW);
  digitalWrite(led_SD_Card_Activity, LOW);
  digitalWrite(led_SD_ERROR, LOW);
  Serial.println("LED's OFF.");

  // Set LCD background colour depending on SD card in/out ?
  if(!lcd_BG_Light) // FALSE if theres no SD card
  {
    digitalWrite(led_SD_Card_Activity, HIGH);
    lcd.setRGB(200,0,200);  // Card is OUT colour
  }
  else
  {
    // Serial.println("Orange Set LOW...\n");
    lcd.setRGB(255,200,0);    // Card is IN colour
  }

  Serial.print("Card SETUP...\n");
  lcd_MSG_Startup(0,0,"--MicroSD Card--");

  if(CARD_PRESENT)  // If the SD cards IN
  {
    lcd.setCursor(3,1);
    lcd.print("Setting Up...");

    digitalWrite(led_SD_Card_Activity, HIGH);

    // This next part will either, CREATE the file (if it's not there), or OPEN an exsisting file then close it
    File data_File = SD.open("LogData.txt", FILE_WRITE);
    data_File.close();
    Serial.println("File OK.");

    digitalWrite(led_SD_Card_Activity, LOW);
  }
  else
  {
    // If there wasn't an SDcard installed this code runs
    lcd.setCursor(0,1);
    lcd.print("No Card, No Logs");
    Serial.println("NO LOGGING!");
  }

  delay(2000);
  // Setup LCD for continuous sensor outputs
  lcd_MSG_Setup();
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay (2000); // This is the delay between each read of the sensor and update of the LCD display
  Read_Sensor_Values(t,h,p,a);
  Send_To_Serial(t,h,p,a);
  Update_LCD_Display(t,h,p,a);

  // log_counter++;
  // Serial.println(log_counter);

  if(CARD_PRESENT)
  {
    data_String = DATA_TO_STRING();
    Serial.print(data_String);
    Serial.println();

    digitalWrite(led_SD_Card_Activity, HIGH);
    File data_File = SD.open("LogData.txt", FILE_WRITE);
    if (data_File)
    {
      data_File.println(data_String);
      data_File.close();
    }
    else
    {
      digitalWrite(led_SD_ERROR, HIGH);
      Serial.println("Error opening the file.");
    }
    digitalWrite(led_SD_Card_Activity, LOW);
  }
}