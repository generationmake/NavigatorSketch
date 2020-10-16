/******************************************************************************
  Navigation demonstration on Arduino

  Hardware:
  Adafruit Feather M0 Adalogger https://learn.adafruit.com/adafruit-feather-m0-adalogger
  HMIFeatherWing https://github.com/generationmake/HMIFeatherWing
  gps receiver ublox NEO-6M

  used libraries:
  DogGraphicDisplay https://github.com/generationmake/DogGraphicDisplay
  ArduinoNmeaParser https://github.com/107-systems/107-Arduino-NMEA-Parser
  

******************************************************************************/
/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <DogGraphicDisplay.h>
#include "ubuntumono_b_16.h"
#include <ArduinoNmeaParser.h>

#define BACKLIGHTPIN 10

/**************************************************************************************
 * FUNCTION DECLARATION
 **************************************************************************************/

void onGprmcUpdate(nmea::RmcData const);

/**************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************/

ArduinoNmeaParser parser(onGprmcUpdate);
DogGraphicDisplay DOG;

/**************************************************************************************
 * SETUP/LOOP
 **************************************************************************************/


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
  while (Serial1.available()) {
    int incomingByte=Serial1.read();
    parser.encode((char)incomingByte);
    Serial.write(incomingByte);   // read it and send it out Serial (USB)
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
    sprintf(buf, "%02i:%02i:%02i",rmc.time_utc.hour,rmc.time_utc.minute,rmc.time_utc.second);
    DOG.string(0,2,UBUNTUMONO_B_16,buf); // print time in line 2 left
  }

  if (rmc.is_valid)
  {
    Serial.print(" : LON ");
    Serial.print(rmc.longitude);
    Serial.print(" ° | LAT ");
    Serial.print(rmc.latitude);
    Serial.print(" ° | VEL ");
    Serial.print(rmc.speed);
    Serial.print(" m/s | HEADING ");
    Serial.print(rmc.course);
    Serial.print(" °");
    sprintf(buf, "%03.4f-%02.4f",rmc.longitude,rmc.latitude);
    DOG.string(0,0,UBUNTUMONO_B_16,buf); // print position in line 0 
    String speed(rmc.speed);
    DOG.string(80,2,UBUNTUMONO_B_16,speed.c_str()); // print speed in line 2 right
  }
  else 
  {
    DOG.string(0,0,UBUNTUMONO_B_16,"data not valid  "); // print "not valid" in line 0 
    DOG.string(80,2,UBUNTUMONO_B_16,"    "); // print "    " in line 2 right 
  }

  Serial.println();
}
