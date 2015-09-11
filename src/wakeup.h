#pragma once

#include "alarm.h"

void spin_window_init(Alarm *alarm);
void spin_window_show();
  
void perform_wakeup_tasks(Alarm* alarm, bool* snooze);
void reschedule_wakeup(Alarm *alarm);