#include "alarm.h"

#define SECOND 60
#define MINUTE 60
#define HOUR 24
  
time_t clock_to_timestamp_precise(WeekDay day, int hour, int minute)
{
  return (clock_to_timestamp(day, hour, minute)/60)*60;
}

void convert_24_to_12(int hour_in, int* hour_out, bool* am)
{
  *hour_out=hour_in%12;
  if (*hour_out==0) {
    *hour_out=12;
  }
  *am=hour_in<12;
}

time_t alarm_get_time_of_wakeup(Alarm *alarm)
{
  if(alarm->enabled)
  {
    // Calculate time to wake up
    time_t now = time(NULL);
    time_t timestamp = clock_to_timestamp_precise(TODAY,alarm->hour,alarm->minute);
    
    if((now-timestamp)<=0) {
      timestamp = (time_t)(HOUR*MINUTE*SECOND) + clock_to_timestamp_precise(TODAY,alarm->hour,alarm->minute);
    }
    return timestamp;
  }
  return -1;
}

void reschedule_wakeup(Alarm *alarm)
{
  wakeup_cancel_all();
  
  time_t timestamp = time(NULL)+(60*60*24*7); // now + 1 week
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Now has timestamp %d",(int)timestamp);
//   int alarm_id=-1;
  time_t alarm_time = alarm_get_time_of_wakeup(alarm);
  if(alarm_time<0)
    return;
  if(alarm_time<timestamp)
  {
    timestamp = alarm_time;
    alarm->alarm_id = wakeup_schedule(timestamp,0,true);
//     APP_LOG(APP_LOG_LEVEL_DEBUG,"Scheduling Alarm %d",alarm_id);
    struct tm *t = localtime(&timestamp);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Scheduled at %d.%d %d:%d",t->tm_mday, t->tm_mon+1,t->tm_hour,t->tm_min);
  }
}