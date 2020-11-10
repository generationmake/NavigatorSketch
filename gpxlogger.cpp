#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h>
#include "gpxlogger.h"
/*-----------------------------
constructor for class, not needed by Arduino but for complete class. does not do anything.
*/
GpxLogger::GpxLogger() 
{
}

/*-----------------------------
destructor for class, not needed by Arduino but for complete class. Calls Arduino end function
*/
GpxLogger::~GpxLogger() 
{
  end();
}

/*-----------------------------
Arduino begin function. Forward data to initialize function
*/
void GpxLogger::begin() 
{
  GpxLogger::cardSelect=4;
  GpxLogger::log_flag=0;
  GpxLogger::count_logs=0;
}

/*-----------------------------
Arduino end function. stop SPI if enabled
*/
void GpxLogger::end() 
{

}

int GpxLogger::open_log_file(void)
{
  uint8_t i=0;
  if (!SD.begin(GpxLogger::cardSelect)) {
    return -3;  // error SD card
  }

  Serial.println("initialization done.");  
  for (i = 0; i < 100; i++)
  {
    GpxLogger::filename[3] = i/10 + '0';
    GpxLogger::filename[4] = i%10 + '0';

    if(!SD.exists(GpxLogger::filename))
    {
      File dataFile = SD.open(GpxLogger::filename, FILE_WRITE);
      // if the file is available, write to it:
      if (dataFile) {
        dataFile.println("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>");
        dataFile.println("<gpx version=\"1.1\" creator=\"NavigatorSketch\">");
        dataFile.println("<trk>");
        dataFile.println("<trkseg>");
        dataFile.close();
        GpxLogger::log_flag=1;
      }
      else
      {
        return -2;    // error file
      }
      break;
    }
  }
  if(i==100) return -1;
  else 
  {
    GpxLogger::count_logs=0;
    return 0;
  }
}

void GpxLogger::close_log_file(void)
{
  if(log_flag)
  {
    File dataFile = SD.open(GpxLogger::filename, FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println("</trkseg>");
      dataFile.println("</trk>");
      dataFile.println("</gpx>");
      dataFile.close();
    }  
    GpxLogger::log_flag=0;
  }
}

void GpxLogger::log_trkpoint(float latitude, float longitude, float speed, float course, time_t timestamp)
{
  if(log_flag)
  {
    File dataFile = SD.open(GpxLogger::filename, FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.print("<trkpt lat=\"");
      dataFile.print(latitude, 6);
      dataFile.print("\" lon=\"");
      dataFile.print(longitude, 6);
      dataFile.print("\"><time>");
      dataFile.print(timestamp_iso8601(timestamp));
      dataFile.print("</time><speed>");
      if(!isnan(speed)) dataFile.print(speed);
      dataFile.print("</speed><course>");
      if(!isnan(course)) dataFile.print(course);
      dataFile.println("</course></trkpt>");
      dataFile.close();
      GpxLogger::count_logs++;
    }  
  }
}

bool GpxLogger::is_enabled(void)
{
  return GpxLogger::log_flag;
}

int GpxLogger::num_logs(void)
{
  return GpxLogger::count_logs;
}

const char *GpxLogger::timestamp_iso8601(time_t timestamp)
{
  static char iso8601[21]="0000-00-00T00:00:00Z";  // create buffer
  iso8601[0]=(year(timestamp)/1000)%10+48;
  iso8601[1]=(year(timestamp)/100)%10+48;
  iso8601[2]=(year(timestamp)/10)%10+48;
  iso8601[3]=year(timestamp)%10+48;
  iso8601[5]=(month(timestamp)/10)%10+48;
  iso8601[6]=month(timestamp)%10+48;
  iso8601[8]=(day(timestamp)/10)%10+48;
  iso8601[9]=day(timestamp)%10+48;
  iso8601[11]=(hour(timestamp)/10)%10+48;
  iso8601[12]=hour(timestamp)%10+48;
  iso8601[14]=(minute(timestamp)/10)%10+48;
  iso8601[15]=minute(timestamp)%10+48;
  iso8601[17]=(second(timestamp)/10)%10+48;
  iso8601[18]=second(timestamp)%10+48;
  return iso8601;
}
