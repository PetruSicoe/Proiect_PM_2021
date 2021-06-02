#include <LiquidCrystal_I2C.h>

#include <OakOLED.h>

#include <Adafruit_SSD1306.h>

#include <Fonts/FreeMonoBold9pt7b.h>  // Custom font for OLED

#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS     1000
#define BEGIN_TIME_MS       2000

//global variables necessary for implementation

//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

//the sensor
PulseOximeter pox;

//variables necessary for loop function
uint32_t tsLastReport = 0;
uint32_t tsBegin = 0;

//OLED display
OakOLED oled;

//ports for leds and the buzzer
int buzzer = 8;
int red = 9;
int green = 7;

//variables which store the measured values
float heartrate = 0;
int spo2 = 0;

//0=finger not detected, 1=finger detected
int isGreen = 0;

//bitmap image for the OLED
static const unsigned char PROGMEM logo2_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00,  };


//function which helps make the buzz noise
void buzz()
{
    int numbuzz = 0;
    int ishigh = 0;
    uint32_t last_beep = 0;
    while (numbuzz < 80)
    {
      pox.update();
      if( millis() - last_beep > 1)
      {
        if(ishigh == 0)
        {
          digitalWrite (buzzer, HIGH) ; //send tone
          ishigh = 1;
        }
        else
        {
          digitalWrite (buzzer, LOW) ; //no tone
          ishigh = 0;
        }
        last_beep = millis();
        numbuzz++;
      }
    }
}

//function which is called whenever a heart beat is detected
void onBeatDetected()
{
    Serial.println("Beat!");
    //we do something only if the finger is detected. If so, the heart bitmap
    //will appear on the screen and the buzzer will beep once
    if (isGreen == 1)
    {
      oled.drawBitmap(16, 5, logo2_bmp, 24, 21, WHITE);
      oled.display();
      buzz();
    }
    
}
 
void setup()
{
    Serial.begin(115200);
    //initialize LCD and OLED and print project title on the LCD
    lcd.begin();
    lcd.print("PM Oximeter");
    oled.begin();
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);
    //initialize Pulseoximeter
    oled.println("Initializing pulse oximeter..");
    oled.display();
    Serial.print("Initializing pulse oximeter..");

    //If the initialization fails, a message will be printed on the screen and
    //we'll enter an infinite loop
    if (!pox.begin()) {
      Serial.println("FAILED");
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(1);
      oled.setCursor(0, 0);
      oled.println("FAILED");
      oled.display();
    for(;;);
    } else {
      //if the sensor is successfully initialized, we'll setup the OLED and the sensor
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(1);
      
      oled.setCursor(0, 0);
      oled.println("SUCCESS");
      oled.display();
      oled.setFont(&FreeMonoBold9pt7b);  // Set a custom font
      
    }
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);

    //7 is for green, 8 for the buzzer and 9 for red
    pinMode(red, OUTPUT);
    pinMode(buzzer, OUTPUT);
    pinMode(green, OUTPUT);
    //buzz one time after all the setup is ready
    buzz();
    tsBegin = millis();
    
}
 
void loop()
{

   
    // Make sure to call update as fast as possible
    pox.update();
    //Measurement begins after 2 secs
    if(millis() - tsBegin > BEGIN_TIME_MS)
    {
        //Each seconds the heartrate and so2 values will be updated
        if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    
            heartrate = pox.getHeartRate();
            spo2 = pox.getSpO2();
            //Prints for serial monitor
            Serial.print("Heart BPM:");
            Serial.print(pox.getHeartRate());
            Serial.print("-----");
            Serial.print("Oxygen Percent:");
            Serial.print(pox.getSpO2());
            Serial.println("\n");
            //handle the LED states
            if(spo2 == 0)
            {
              digitalWrite(red, HIGH);
              digitalWrite(green, LOW);
              isGreen = 0;
            }
            else
            {
              digitalWrite(red, LOW);
              digitalWrite(green, HIGH);
              isGreen = 1;
            }

            //if finger is detected both values will be printed
            if(isGreen == 1)
            {
                oled.setCursor(60, 20);  // (x,y)
                oled.clearDisplay();  // Clear the display so we can refresh
                
                oled.print(heartrate);
                oled.setCursor(60, 57);  // (x,y)
                oled.print(spo2);
                // Draw rounded rectangle:
                oled.drawRoundRect(55, 0, 62, 27, 8, WHITE); // Draw rounded rectangle (x,y,width,height,radius,color)
                                                                // It draws from the location to down-right
                // Draw rounded rectangle:
                oled.drawRoundRect(55, 37, 49, 27, 8, WHITE);  // Draw rounded rectangle (x,y,width,height,radius,color)
                                                                  // It draws from the location to down-rig
                oled.setCursor(0, 60);  // (x,y)
                oled.print("SPO2");  // Text or value to print
                oled.display();
            }
            else
            {
                //if the finger is not detected, a message is printed on the OLED screen
                oled.setCursor(20, 20);  // (x,y)
                oled.clearDisplay();  // Clear the display so we can refresh
                oled.print("SPO2...");
                oled.display();
            }
            

            tsLastReport = millis();
        }
    }
    
  
}
