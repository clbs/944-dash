#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoPixel.h>
#include <gfxfont.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"


int pixelPin = 6;
int sensorValue = 0;  // variable to store the value coming from the sensor
int level = 0;
int pixels = 24;
String volts = ("XX.XV");

//Set up ring buffer
char rxData[20];
char rxIndex=0;

Adafruit_7segment matrix = Adafruit_7segment();
Adafruit_AlphaNum4 alpha = Adafruit_AlphaNum4();
Adafruit_7segment matrix2 = Adafruit_7segment();
Adafruit_7segment matrix3 = Adafruit_7segment();


#define I2C_ADDR    0x3F  // Define I2C Address where the SainSmart LCD is
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
#define PIN 6
const int BRIGHTNESS = 50;
const int LED_COUNT = 80;
const int REDLINE = 5000;
const int WARN_RPM = 3500;
const int WARN_INTERVAL = 70;
const int numReadings = 30;     // the number of readings for average brightness
int readings[numReadings];      // the readings array for the analog input
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 3; 
uint8_t previousBrightness = 0;    // typical operation display brightness
uint8_t currentBrightness = 100;   // typical operation diaplay brightness
uint8_t daytimeBrightness =100;   // brightness for when headlampStatus is TRUE
uint8_t nighttimeBrightness = 127;  // brightness for when headlampStatus is FALSE


uint8_t numberOfRingLED = 24; 
int engineSpeed = 0;

String headlampStatusString = ""; // contains "t" if headlamps are on, "f" if they are off
String curHeadlampStatus = "";    // to detect changes in state
String prevHeadlampStatus = "";

int newEngineSpeedData = 0;
int newHeadlampStatusData = 0;

int minRPM = 500;   // RPM where the first LED will light
int maxRPM = 5800;  // RPM where the last LED will light 
int LEDsForRPM = numberOfRingLED;  // the number of LED's used for RPM display - it could be less than the max if you like

int peakHoldLedPos = 0;     // position of the sticky LED
int peakHoldTime = 3000;    // number of milliseconds to keep the peak rpm lit up for
long peakHoldStartMs = 0;   // the millis() value when setting the sticky LED

// we dont need to redraw the display if it is the same as the last time
int RPMpreviousNumLED = 0;  // the number of RPM LED previously lit up
int RPMcurrentNumLED = 0;   // the current number of RPM LED lit up

float incrementRPM = ( maxRPM - minRPM ) / LEDsForRPM;

String stringToParse;



long prevMillis = 0;  // for the sticky LED
long myMillis = 0;

Adafruit_NeoPixel ring = Adafruit_NeoPixel(24, PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C  lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);
uint32_t color = ring.Color(255, 0, 0);

byte fuelLeft[] = {
  B11111,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte fuelRight[] = {
  B00000,
  B00000,
  B10000,
  B01000,
  B00100,
  B00100,
  B11000,
  B00000
};


void setup() {
  Serial.begin(9600);
  lcd.begin(20, 4);
  lcd.createChar(0, fuelLeft);
  lcd.createChar(1, fuelRight);
  matrix.begin(0x70);
  matrix2.begin(0x73);
  ring.setBrightness(BRIGHTNESS);
  matrix3.begin(0x72);
  matrix.println(0);
  matrix.writeDisplay();
  matrix2.println(0);
  matrix2.writeDisplay();
  //matrix3.println("0");
  //matrix3.writeDisplay();
  ring.begin();
  ring.show(); 
  theaterChaseRainbow(5);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  TCCR1A = 0; //Configure hardware counter 
  TCNT1 = 0;  // Reset hardware counter to zero
  lcd.home();
  lcd.print("LOADING ODB i/o");
  ODB_init;
}

char displaybuffer[4] = { ' ', ' ', ' ', ' ' };
//delete
  uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
      return ring.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 5800) {
      WheelPos -= 85;
      return ring.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 5800;
    return ring.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }

void theaterChaseRainbow(uint8_t wait) {
     
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < ring.numPixels(); i = i + 3) {
        ring.setPixelColor(i + q, Wheel((i + j) % 255)); //turn every third pixel on
      }
      ring.show();

      delay(wait);

      for (int i = 0; i < ring.numPixels(); i = i + 3) {
        ring.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }

}



  void werkit(int i) {

    level = 1024 / i;
    checkColor(level);
    for (int i = 0; i < level; i++) {
      ring.setPixelColor(i, color);
    }
    for (int i = 25; i > level; i--) {
      ring.setPixelColor(i, 0, 0, 0);
    }
    ring.show();
    delay(5);
  }

  
  void clear() {
    for (int i = 0; i < pixels; i++) {
      ring.setPixelColor(i, 0, 0, 0);
      ring.show();
    }
  }


  void checkColor(int level) {
    if (level < 5) {
      color = ring.Color(255, 0, 255);
    }
    else if (5 < level && level < 18) {
      color = ring.Color(255, 255, 0);
    }
    else if (level > 18) {
      color = ring.Color(255, 0, 0);
    }
  }


void drawRing( void ){
  
  // if there is valid data then we should update the display.
          
             
             // turn everything off
             for( int i = 0; i < numberOfRingLED; i++ ){
               
                ring.setPixelColor( i, 0, 0, 0 ); 
             
             }
             
             // calculate the number of LEDs to light up           
             RPMcurrentNumLED = ( engineSpeed - minRPM ) / incrementRPM;  // round the number of LEDs to light to the lowest integer

             
             // prepare to turn them on, if the new number of LED to light is different than the old number to light
             if( RPMcurrentNumLED != RPMpreviousNumLED ){
               
               
               
                 // set all LEDs to light to green to start
                 for( int i = 0; i < RPMcurrentNumLED; i++ ){
                    
                    ring.setPixelColor( i, 0, 40, 0 ); // green
                    
                 } 
                 
                 
                 
                 // if the RPM is  high, set some intermediary yellows
                 if( RPMcurrentNumLED >= ( LEDsForRPM - 7 ) ){
                   
                   for( int i = ( LEDsForRPM - 7 ); i < RPMcurrentNumLED; i++ ){
                    
                    ring.setPixelColor( i, 80 , 40, 0 ); // yellow
                    
                   } 
                   
                 }
                 
               
                 // if the RPM is really high, set the last 4 LED's red
                 if( RPMcurrentNumLED >= ( LEDsForRPM - 4 ) ){
                   
                   for( int i = ( LEDsForRPM - 4 ); i < RPMcurrentNumLED; i++ ){
                    
                    ring.setPixelColor( i, 80, 20 , 0 ); // red
                    
                   } 
                   
                 }
                 
                 // also, I want whichever the highest LED that is currently on to be red, like a pointer
                 // this is probably redundant with the sticky led?
                 ring.setPixelColor( ( RPMcurrentNumLED - 1 ), 100 , 10 , 0 ); // red
                 
                 
                 // now figure out the sticky led
                 // if the current RPM LED is higher than the current sticky led position
                 // this continues in the main loop
                 if( RPMcurrentNumLED > peakHoldLedPos ){
                   
                   peakHoldLedPos = RPMcurrentNumLED;  // set the current value as the peak
                   peakHoldStartMs = millis();        // record the start time of the new peak
                   
                 }
                 
                 
                 
                  // if the sticky led has been lit for too long
                  if( millis() - peakHoldStartMs > peakHoldTime ){
                   
                      peakHoldLedPos = RPMcurrentNumLED;  // set the current value as the peak
                      peakHoldStartMs = millis();        // record the start time of the new peak
                   
                  }
                 
                
                 
                 // I also think the sticky LED should be reset if the RPM begins increasing, 
                 // but not if the RPM is steady state or decreasing.
                 
                 if( RPMcurrentNumLED > RPMpreviousNumLED ){
                   
                   peakHoldLedPos = RPMcurrentNumLED;  // set the current value as the peak
                   peakHoldStartMs = millis();        // record the start time of the new peak
                   
                 }
                 
                 // and finally, set the position of the sticky LED
                 ring.setPixelColor( ( peakHoldLedPos - 1 ), 70 , 0 , 5 ); // red
                 
             
                                  
                 ring.show();  // show the new LED display
                 
                 RPMpreviousNumLED = RPMcurrentNumLED;
             
           }
           
             
           
           
  
}
//this


void loop() {
  int fuel;
  int rpm;
  int watertemp;
  int throt;
  int volt;
  float fuelHun;
  String fuelstr;
  

  
  // Read fuel level resistor
   int fuelLevel = analogRead(1);
  // Calculate fuel level from measured resistance values
   fuelHun = (1.00 - ((fuelLevel - 402.00) / 190.00)) * 100.00;
   fuel = (int)fuelHun;
   if (fuel > 99) {
    //show full if resistance is max value
    fuelstr = ("FULL!");
   }
   else if (fuel < 12 && fuel > 0) {
    //Show reserve if fuel level falls below reserve level
    fuelstr = ("RESERVE " + String(fuel) + "%");
   }
   else if  (fuel > 9 && fuel < 99) {
    //print resistance percentage if fuel level is between reserve and full
    fuelstr = (String(fuel) + "%");
   }
    else if  (fuel < 0) {
    //show empty if fuel resistance percentage is 0
    fuelstr = ("EMPTY");
   }
   for (int counter = 0; counter < 300; counter++) {
    //Pull ODB2 coolant temp and write to display
    
    if (counter = 100) {
    watertemp = getTemp();
    matrix2.println(watertemp);
    matrix2.writeDisplay();
    }
    
    //matrix3 ended up being controlled by a second arduino to measure speed
    // matrix3.println(counter );
    //matrix3.writeDisplay();
    
    
    lcd.home();
    lcd.print("IAT: ");
    lcd.setCursor(5, 0);
    lcd.print("           ");
    lcd.setCursor(5, 0);
    
    //Writes throttle % to LCD
    if (counter = 200) {
    throt = getIat();
    lcd.print(throt);
    }
 
    lcd.setCursor(0, 1);
    lcd.print("RPM: ");
    lcd.setCursor(6, 1);
    lcd.print("       ");
    lcd.setCursor(6, 1);
    
    //Writes rpm to displays
    rpm = getRPM();
    lcd.print(rpm);
    matrix.println(rpm);
    matrix.writeDisplay();
    engineSpeed = rpm; //Sets engine speed variable for draw ring function in memory to the odb2 rpm value
    drawRing(); //Sends the RPM value to the neopixel ring
    
    
    lcd.setCursor(0, 2);
    lcd.print("VOLTAGE: ");
    lcd.setCursor(9, 2);
    lcd.print("    ");
    lcd.setCursor(9, 2);

    //Writes voltage to LCD
    if (counter = 300) {
        getVolts();
        lcd.print(volts);
    }
    
    lcd.setCursor(0, 3);
    lcd.write(byte(0));
    lcd.setCursor(1, 3);
    lcd.write(byte(1));
    lcd.setCursor(2, 3);
    lcd.print(": ");
    lcd.setCursor(5, 3);
    lcd.print("            ");
    lcd.setCursor(5, 3);
    //Writes fuel value to LCD
    lcd.print(fuelstr);

   
  }

}   

  
void ODB_init(void)
{
  //Wait for a little while before sending the reset command to the OBD-II-UART
  delay(2000);
  //Reset the OBD-II-UART
  Serial.print("ATZ\r");
  //Wait for a bit before starting to send commands after the reset.
  delay(2000);
  OBD_read();
  Serial.print("ATD\r");
  OBD_read();
  Serial.print("ATE0\r");
  OBD_read();
  Serial.flush();  
}

int getRPM(void)
{
  //Query the OBD-II-UART for the Vehicle rpm
  //Serial.flush();
  Serial.print("010C\r");
  OBD_read();
 
  return ((strtol(&rxData[9],0,16)*256)+strtol(&rxData[12],0,16))/4;
}

int getTemp(void)
{
  //Query the OBD-II-UART for the Engine Coolant Temp
  //Serial.flush();
  Serial.print("0105\r");
  OBD_read();
  return strtol(&rxData[9], 0, 16) - 40;
}

int getIat(void)
{
  //Query the OBD-II-UART for the Engine Coolant Temp
  //Serial.flush();
  Serial.print("010F\r");
  OBD_read();
 
  return (strtol(&rxData[9],0,16)-40);
}

int getVolts(void)
{
  //Query the OBD-II-UART for the Engine Coolant Temp
  //Serial.flush();
  Serial.print("ATRV\r");
  OBD_read();
  volts = rxData;
  volts = volts.substring(4,9);
}

void OBD_read(void)
{
  char c;
  do{
    if(Serial.available() > 0)
    {
      c = Serial.read();
      if((c!= '>') && (c!='\r') && (c!='\n')) //Keep these out of our buffer
      {
        rxData[rxIndex++] = c; //Add whatever we receive to the buffer
      }  
     }     
  }while(c != '>'); //The ELM327 ends its response with this char so when we get it we exit out.
  rxData[rxIndex++] = '\0';//Converts the array into a string
  rxIndex=0; //Set this to 0 so next time we call the read we get a "clean buffer"
}


 

