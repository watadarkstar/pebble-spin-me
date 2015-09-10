#pragma once

#include <pebble.h>
#include "alarm.h"
  
void settings_window_init(struct Alarm *alarm);
void settings_window_show(void);
