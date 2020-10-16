/******************************************************************************
  Navigation demonstration on Arduino

  Hardware:
  Adafruit Feather M0 Adalogger https://learn.adafruit.com/adafruit-feather-m0-adalogger
  HMIFeatherWing https://github.com/generationmake/HMIFeatherWing
  gps receiver ublox NEO-6M

  used libraries:
  DogGraphicDisplay https://github.com/generationmake/DogGraphicDisplay
  ArduinoNmeaParser https://github.com/107-systems/107-Arduino-NMEA-Parser
  NavPoint https://github.com/wuehr1999/NavigationOnArduino

******************************************************************************/
/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <DogGraphicDisplay.h>
#include "ubuntumono_b_16.h"
#include "dense_numbers_8.h"
#include <ArduinoNmeaParser.h>
#include <NavPoint.h>

#define BACKLIGHTPIN 10

// define some values used by the panel and buttons
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5



/**************************************************************************************
 * FUNCTION DECLARATION
 **************************************************************************************/

void onGprmcUpdate(nmea::RmcData const);

/**************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************/

ArduinoNmeaParser parser(onGprmcUpdate);
DogGraphicDisplay DOG;
volatile unsigned int display_screen=0;
volatile bool nav_flag=0;
volatile float global_longitude=0.0;
volatile float global_latitude=0.0;


/**************************************************************************************
 * SETUP/LOOP
 **************************************************************************************/

const char *maidenhead(float lon, float lat)
{
  static char locator[7]="000000";  // create buffer
  int x, y;
  locator[0]=(int)(lon+180.0)/20+65;
  locator[1]=(int)(lat+90.0)/10+65;
  locator[2]=((int)(lon+180.0)%20)/2+48;
  locator[3]=(int)(lat+90.0)%10+48;
  locator[4]=(int)(((lon/2+90.0)-(int)(lon/2+90.0))*24.0)+65;
  locator[5]=(int)(((lat+90.0)-(int)(lat+90.0))*24.0)+65;
  return locator;
}

// read the buttons
int read_LCD_buttons()
{
  int adc_key_in = analogRead(5);      // read the value from the sensor 
  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // my buttons when read are centered at these values (MKR1010): 0, 11, 162, 354, 531, 763
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  if (adc_key_in < 50)   return btnRIGHT;  
  if (adc_key_in < 250)  return btnUP; 
  if (adc_key_in < 450)  return btnDOWN; 
  if (adc_key_in < 650)  return btnLEFT; 
  if (adc_key_in < 850)  return btnSELECT;  

  return btnNONE;  // when all others fail, return this...
}


void setup() {
  pinMode(BACKLIGHTPIN,  OUTPUT);   // set backlight pin to output
  digitalWrite(BACKLIGHTPIN,  HIGH);  // enable backlight pin

  Serial.begin(9600);
  Serial1.begin(9600);

  DOG.begin(A1,0,0,A3,A2,DOGM132);   //CS = A1, 0,0= use Hardware SPI, A0 = A3, RESET = A2, EA DOGM132-5 (=132x32 dots)

  DOG.clear();  //clear whole display
  DOG.string(0,0,UBUNTUMONO_B_16,"data not valid"); // print "not valid" in line 0 
}

void loop() {
  static NavPoint dest(48.995796, 12.837936);

  while (Serial1.available()) {
    int incomingByte=Serial1.read();
    parser.encode((char)incomingByte);
    Serial.write(incomingByte);   // read it and send it out Serial (USB)
  }
  int lcd_key = read_LCD_buttons();  // read the buttons
  switch (lcd_key)               // depending on which button was pushed, we perform an action
  {
    case btnUP:               // up
      {
        if(display_screen<2) display_screen++;
        else display_screen=0;
        DOG.clear();  //clear whole display
        delay(300);
        break;
      }
    case btnDOWN:               // down
      {
        if(display_screen>0) display_screen--;
        else display_screen=3;
        DOG.clear();  //clear whole display
        delay(300);
        break;
      }
    case btnRIGHT:               // down
      {
        if(display_screen==1)
        {
          dest.setLatitude(global_latitude);
          dest.setLongitude(global_longitude);
        }
        break;
      }
  }
  if(nav_flag)
  {
    char buf[30];
    nav_flag=0;
    if(display_screen==2)
    {
       DOG.string(70,0,UBUNTUMONO_B_16,maidenhead(global_longitude,global_latitude));    
    }
    if(display_screen==1)
    {
      NavPoint pos(global_latitude, global_longitude);
      sprintf(buf, "%03.6f",dest.getLongitude());
      DOG.string(0,2,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.6f",dest.getLatitude());
      DOG.string(0,3,DENSE_NUMBERS_8,buf); // print position in line 0 
      // distance
      float distance = pos.calculateDistance(dest);
      // bearing
      float bearing = pos.calculateBearing(dest);
      if(bearing<0) bearing+=360.0; // bring bearing to 0 to 360 degrees, just like in NMEA dataset
      sprintf(buf, "%03.6f",bearing);
      DOG.string(70,2,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.6f",distance);
      DOG.string(70,3,DENSE_NUMBERS_8,buf); // print position in line 0 
    }
  }
}

/**************************************************************************************
 * FUNCTION DEFINITION
 **************************************************************************************/

void onGprmcUpdate(nmea::RmcData const rmc)
{
  char buf[30];
  Serial.print(rmc.time_utc.hour);
  Serial.print(":");
  Serial.print(rmc.time_utc.minute);
  Serial.print(":");
  Serial.print(rmc.time_utc.second);
  Serial.print(".");
  Serial.print(rmc.time_utc.microsecond);
  if(rmc.time_utc.hour>=0)
  {
    if((display_screen==0)||(display_screen==2))
    {
      sprintf(buf, "%02i:%02i:%02i",rmc.time_utc.hour,rmc.time_utc.minute,rmc.time_utc.second);
      DOG.string(0,2,UBUNTUMONO_B_16,buf); // print time in line 2 left
    }
  }

  if (rmc.is_valid)
  {
    global_longitude=rmc.longitude;
    global_latitude=rmc.latitude;
    nav_flag=1;
    Serial.print(" : LON ");
    Serial.print(rmc.longitude);
    Serial.print(" ° | LAT ");
    Serial.print(rmc.latitude);
    Serial.print(" ° | VEL ");
    Serial.print(rmc.speed);
    Serial.print(" m/s | HEADING ");
    Serial.print(rmc.course);
    Serial.print(" °");
    if(display_screen==0)
    {
      sprintf(buf, "%03.4f-%02.4f",rmc.longitude,rmc.latitude);
      DOG.string(0,0,UBUNTUMONO_B_16,buf); // print position in line 0 
      String speed(rmc.speed);
      DOG.string(70,2,DENSE_NUMBERS_8,speed.c_str()); // print speed in line 2 middle
      String speedkmh(rmc.speed*3.6);
      DOG.string(70,3,DENSE_NUMBERS_8,speedkmh.c_str()); // print speed in km/h in line 3 middle
      String course(rmc.course);
      DOG.string(100,2,DENSE_NUMBERS_8,course.c_str()); // print speed in line 2 right
    }
    if((display_screen==1)||(display_screen==2))
    {
      sprintf(buf, "%03.6f",rmc.longitude);
      DOG.string(0,0,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.6f",rmc.latitude);
      DOG.string(0,1,DENSE_NUMBERS_8,buf); // print position in line 0 
    }
    if(display_screen==1)
    {
      sprintf(buf, "%03.2f",rmc.speed);
      DOG.string(70,0,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.2f",rmc.speed*3.6);
      DOG.string(100,0,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.2f",rmc.course);
      DOG.string(70,1,DENSE_NUMBERS_8,buf); // print position in line 0 
      
    }
  }
  else 
  {
    if(display_screen==0)
    {
      DOG.string(0,0,UBUNTUMONO_B_16,"data not valid  "); // print "not valid" in line 0 
      DOG.string(80,2,UBUNTUMONO_B_16,"    "); // print "    " in line 2 right 
    }
  }

  Serial.println();
}
