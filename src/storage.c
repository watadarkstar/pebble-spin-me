#include "storage.h"
#include "alarm.h"

void load_persistent_storage_alarm(Alarm *alarm)
{
    if(persist_exists(ALARMS_KEY))
    {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "alarm stored");
      persist_read_data(ALARMS_KEY,alarm,sizeof(struct Alarm));
      APP_LOG(APP_LOG_LEVEL_DEBUG, "alarm stored end");
    }
    else
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "alarm new");
        alarm->hour=0;
        alarm->minute=0;
        alarm->enabled=false;
        alarm->alarm_id=-1;
    }
}

void write_persistent_storage_alarm(Alarm *alarm)
{
  persist_write_data(ALARMS_KEY,alarm,8*sizeof(Alarm));
}

bool load_persistent_storage_bool(int key, bool default_val)
{
  bool temp = default_val;
  if(persist_exists(key))
    temp = persist_read_bool(key);
  return temp;
}
int load_persistent_storage_int(int key, int default_val)
{
  int temp = default_val;
  if(persist_exists(key))
    temp = persist_read_bool(key);
  return temp;
}