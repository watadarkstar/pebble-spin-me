#pragma once

#include <pebble.h>

#define SETTINGS_IS_ENABLED_KEY 5
  
static Window *s_settings_window;
static MenuLayer *s_settings_menu_layer;

enum MENU_ITEM
{
  MENU_EDIT=0,
  MENU_ENABLE_DISABLE=1,
  MENU_TUTORIAL=2,
  NUM_MENU
};

typedef struct Alarm{
  unsigned char hour;
  unsigned char minute;
  bool enabled;
  WakeupId alarm_id;
}Alarm;

struct Alarm alarm;
  
void settings_window_init(void);
void settings_window_show(void);
