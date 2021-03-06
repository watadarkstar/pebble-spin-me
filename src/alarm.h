#pragma once
  
#include <pebble.h>
  
#define NUM_ALARMS 1
  
typedef struct Alarm{
  unsigned char hour;
  unsigned char minute;
  bool enabled;
  WakeupId alarm_id;
}Alarm;

void convert_24_to_12(int hour_in, int* hour_out, bool* am);
time_t alarm_get_time_of_wakeup(Alarm *alarm);
void reschedule_wakeup(Alarm *alarm);