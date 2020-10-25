#ifndef GPXLOGGER_H
#define GPXLOGGER_H

class GpxLogger
{
  public:
    GpxLogger();
    ~GpxLogger();
    void begin();
    void end();
    int open_log_file(void);
    void close_log_file(void);
    void log_trkpoint(float latitude, float longitude, float speed, float course);
    bool is_enabled(void);
    int num_logs(void);

  private:
    bool log_flag=0;
    int cardSelect=4;
    int count_logs=0;
    char filename[10]="log00.gpx";

};

#endif /* GPXLOGGER_H */
