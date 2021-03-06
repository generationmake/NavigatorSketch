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
#include "gpxlogger.h"
#include "ringbuffer.h"
#include <TimeLib.h>

#define BACKLIGHTPIN 10
#define VBATPIN A7

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

void onRmcUpdate(nmea::RmcData const);
void onGgaUpdate(nmea::GgaData const);

/**************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************/

ArduinoNmeaParser parser(onRmcUpdate, onGgaUpdate);
DogGraphicDisplay DOG;
GpxLogger logger;
RingBufferX ring;
volatile unsigned int display_screen=0;
volatile bool nav_flag=0;
volatile float global_longitude=0.0;
volatile float global_latitude=0.0;
volatile float global_speed=0.0;
volatile float global_course=0.0;
volatile time_t global_timestamp=0;
volatile float gga_longitude=0.0;
volatile float gga_latitude=0.0;
volatile int gga_num_satellites=0;
volatile float gga_hdop=0.0;
volatile float gga_altitude=0.0;
volatile float gga_height=0.0;
volatile float gga_geoidal_separation=0.0;


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

  Serial.begin(115200);
  Serial1.begin(9600);   // receiver u-blox NEO-7M NEO-6M
//  Serial1.begin(38400);   // receiver u-blox NEO-M8N

  DOG.begin(A1,0,0,A3,A2,DOGM132);   //CS = A1, 0,0= use Hardware SPI, A0 = A3, RESET = A2, EA DOGM132-5 (=132x32 dots)

  DOG.clear();  //clear whole display
  DOG.string(0,0,UBUNTUMONO_B_16,"data not valid"); // print "not valid" in line 0 
  DOG.createCanvas(32, 32, 100, 0, 1);  // Canvas in buffered mode
  logger.begin();
  ring.begin();
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
        if(display_screen<7) display_screen++;
        else display_screen=0;
        DOG.clear();  //clear whole display
        delay(300);
        break;
      }
    case btnDOWN:               // down
      {
        if(display_screen>0) display_screen--;
        else display_screen=7;
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
        if(display_screen==3)
        {
          if(logger.is_enabled()==0)
          {
            if(logger.open_log_file()<0) DOG.string(0,2,UBUNTUMONO_B_16,"LOG error!"); // print time in line 2 left
            else DOG.string(0,2,UBUNTUMONO_B_16,"LOG started"); // print time in line 2 left
          }
        }
        break;
      }
    case btnLEFT:               // down
      {
        if(display_screen==3)
        {
          if(logger.is_enabled()==1)
          {
            logger.close_log_file();
            DOG.string(0,2,UBUNTUMONO_B_16,"LOG stopped"); // print time in line 2 left
          }
        }
        break;
      }
  }
  if(nav_flag)
  {
    char buf[30];
    nav_flag=0;
    ring.store(gga_height);
    if(logger.is_enabled()==1) logger.log_trkpoint(global_latitude,global_longitude,global_speed,global_course,gga_height,global_timestamp);
    if(display_screen==7) // GGA diagram
    {
      sprintf(buf, "%03.3f",ring.max());
      DOG.string(0,0,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.3f",gga_height);
      DOG.string(0,1,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.3f",ring.min());
      DOG.string(0,3,DENSE_NUMBERS_8,buf); // print position in line 0 

      DOG.clearCanvas();
      for(int x=0;x<32;x++)
      {
        float diff=ring.max()-ring.min();
        DOG.drawLine(x, 32-(int)((ring.getat(31-x)-ring.min())/diff*32.0), x, 32);
      }
      DOG.flushCanvas();
    }
    if(display_screen==6) // GGA values
    {
      sprintf(buf, "%03.6f",gga_longitude);
      DOG.string(0,0,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.6f",gga_latitude);
      DOG.string(0,1,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%05.2f",gga_height);
      DOG.string(0,2,UBUNTUMONO_B_16,buf); // print position in line 0 
      sprintf(buf, "%02i",gga_num_satellites);
      DOG.string(70,0,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%f  ",gga_hdop);
      DOG.string(70,1,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%f  ",gga_altitude);
      DOG.string(70,2,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%f  ",gga_geoidal_separation);
      DOG.string(70,3,DENSE_NUMBERS_8,buf); // print position in line 0 
    }
    if(display_screen==5) // course with arrow
    {
      NavPoint pos(global_latitude, global_longitude);

      // distance
      float distance = pos.calculateDistance(dest);
      // bearing
      float bearing = pos.calculateBearing(dest);
      if(bearing<0) bearing+=360.0; // bring bearing to 0 to 360 degrees, just like in NMEA dataset

      sprintf(buf, "%05.2f",distance);
      DOG.string(0,2,UBUNTUMONO_B_16,buf); // print position in line 0 
      sprintf(buf, "%03.2f",global_speed*3.6);
      DOG.string(0,0,UBUNTUMONO_B_16,buf); // print position in line 0 

      const int circle2_x=16;
      const int circle2_y=16;
      const int circle2_radius=16;
      DOG.clearCanvas();
      DOG.drawCircle(circle2_x, circle2_y, circle2_radius, false);
      if(!isnan(global_course))
      {
        float deltaangle = pos.calculateDeltaAngle(global_course,dest);
        float diff2_x=(circle2_radius-1)*sin(deltaangle*DEG_TO_RAD);
        float diff2_y=(circle2_radius-1)*cos(deltaangle*DEG_TO_RAD);
        DOG.drawArrow(circle2_x-diff2_x, circle2_y+diff2_y, circle2_x+diff2_x, circle2_y-diff2_y);
        sprintf(buf, "%03.2f",deltaangle);
        DOG.string(70,1,DENSE_NUMBERS_8,buf); // print position in line 0 
      }
      else DOG.drawCross(16,16,8,8);
      DOG.flushCanvas();

      sprintf(buf, "%03.2f",bearing);
      DOG.string(70,3,DENSE_NUMBERS_8,buf); // print position in line 0 
      sprintf(buf, "%03.2f",global_course);
      DOG.string(70,2,DENSE_NUMBERS_8,buf); // print position in line 0 
    }
    if(display_screen==4) // battery voltage
    {
      DOG.string(0,0,UBUNTUMONO_B_16,"Battery");
      float measuredvbat = analogRead(VBATPIN);
      measuredvbat *= 2;    // we divided by 2, so multiply back
      measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
      measuredvbat /= 1024; // convert to voltage
      sprintf(buf, "%1.2f V",measuredvbat);
      DOG.string(80,0,UBUNTUMONO_B_16,buf); // print position in line 0 
    }
    if(display_screen==3) // logging
    {
      DOG.string(0,0,UBUNTUMONO_B_16,"LOG status");    
      if(logger.is_enabled()==1) 
      {
        sprintf(buf, "%04d",logger.num_logs());
        DOG.string(100,0,UBUNTUMONO_B_16,buf); // print position in line 0 
        DOG.string(0,2,UBUNTUMONO_B_16,"enabled    ");    
      }
      else DOG.string(0,2,UBUNTUMONO_B_16,"disabled   ");    
    }
    if(display_screen==2) // maidenhead
    {
      DOG.string(70,0,UBUNTUMONO_B_16,maidenhead(global_longitude,global_latitude));    
      if(logger.is_enabled()==1) 
      {
        sprintf(buf, "%04d",logger.num_logs());
        DOG.string(112,3,DENSE_NUMBERS_8,buf); // print position in line 0 
        DOG.string(70,3,DENSE_NUMBERS_8,"enabled 1");    
      }
      else DOG.string(70,3,DENSE_NUMBERS_8,"disabled0");    
    }
    if(display_screen==1) // destination setting
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

void onRmcUpdate(nmea::RmcData const rmc)
{
  char buf[30];

  time_t posix_timestamp = nmea::toPosixTimestamp(rmc.date, rmc.time_utc);
  if      (rmc.source == nmea::RmcSource::GPS)     Serial.print("GPS");
  else if (rmc.source == nmea::RmcSource::GLONASS) Serial.print("GLONASS");
  else if (rmc.source == nmea::RmcSource::Galileo) Serial.print("Galileo");
  else if (rmc.source == nmea::RmcSource::GNSS)    Serial.print("GNSS");

  Serial.print(" ");
  Serial.print(rmc.time_utc.hour);
  Serial.print(":");
  Serial.print(rmc.time_utc.minute);
  Serial.print(":");
  Serial.print(rmc.time_utc.second);
  Serial.print(".");
  Serial.println(rmc.time_utc.microsecond);
  Serial.println(logger.timestamp_iso8601(posix_timestamp));
  if(rmc.time_utc.hour>=0)
  {
    global_timestamp=posix_timestamp;
    if((display_screen==0)||(display_screen==2))
    {
      sprintf(buf, "%02i:%02i:%02i",rmc.time_utc.hour,rmc.time_utc.minute,rmc.time_utc.second);
      DOG.string(0,2,UBUNTUMONO_B_16,buf); // print time in line 2 left
    }
    if((display_screen==2))
    {
      sprintf(buf, "%02i.%02i.%02i",rmc.date.day,rmc.date.month,rmc.date.year);
      DOG.string(70,2,DENSE_NUMBERS_8,buf); // print time in line 2 left
    }
  }

  if (rmc.is_valid)
  {
    global_longitude=rmc.longitude;
    global_latitude=rmc.latitude;
    global_speed=rmc.speed;
    global_course=rmc.course;
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

void onGgaUpdate(nmea::GgaData const gga)
{
  Serial.print("GGA ");

  if      (gga.source == nmea::GgaSource::GPS)     Serial.print("GPS");
  else if (gga.source == nmea::GgaSource::GLONASS) Serial.print("GLONASS");
  else if (gga.source == nmea::GgaSource::Galileo) Serial.print("Galileo");
  else if (gga.source == nmea::GgaSource::GNSS)    Serial.print("GNSS");

  Serial.print(" ");
  Serial.print(gga.time_utc.hour);
  Serial.print(":");
  Serial.print(gga.time_utc.minute);
  Serial.print(":");
  Serial.print(gga.time_utc.second);
  Serial.print(".");
  Serial.print(gga.time_utc.microsecond);

  if (gga.fix_quality != nmea::FixQuality::Invalid)
  {
    gga_longitude=gga.longitude;
    gga_latitude=gga.latitude;
    gga_num_satellites=gga.num_satellites;
    gga_hdop=gga.hdop;
    gga_altitude=gga.altitude;
    gga_geoidal_separation=gga.geoidal_separation;
    gga_height=gga_altitude-gga_geoidal_separation;
    Serial.print(" : LON ");
    Serial.print(gga.longitude);
    Serial.print(" ° | LAT ");
    Serial.print(gga.latitude);
    Serial.print(" ° | Num Sat. ");
    Serial.print(gga.num_satellites);
    Serial.print(" | HDOP =  ");
    Serial.print(gga.hdop);
    Serial.print(" m | Altitude ");
    Serial.print(gga.altitude);
    Serial.print(" m | Geoidal Separation ");
    Serial.print(gga.geoidal_separation);
    Serial.print(" m");
  }

  Serial.println();
}
