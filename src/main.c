#include <pebble.h>
#include "main.h"
#include "storage.h"
#include "wakeup.h"

struct Alarm alarm;
static bool snooze;
  
static void init() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "main init - called");
  load_persistent_storage_alarm(&alarm);
  perform_wakeup_tasks(&alarm,&snooze);
}

static void deinit() {
  if(!snooze)
    reschedule_wakeup(&alarm);
  write_persistent_storage_alarm(&alarm);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}