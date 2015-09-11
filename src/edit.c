#include "edit.h"
#include "settings.h"
#include "alarm.h"

#define PIN_WINDOW_SPACING 24
  
static Window *s_time_window;
static Layer *s_canvas_layer;
static TextLayer *s_input_layers[3];

static const GPathInfo PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0, -5}, {5,5}, {-5, 5}}
};
static GPath *s_my_path_ptr;

static char s_value_buffers[3][3];
static char s_digits[3];
static char s_max[3];
static char s_min[3];
static bool s_withampm;
static bool s_is_am;
static int s_selection;

static Alarm temp_alarm;
static Alarm *current_alarm;

void win_edit_show(Alarm *alarm){
  memcpy(&temp_alarm,alarm,sizeof(Alarm));
  current_alarm = alarm;
//   s_select_all = false;
  s_max[2]=1;s_min[2]=0;
  s_max[1]=59;s_min[1]=0;
  int hour;
  if(clock_is_24h_style())
  {
    s_withampm=false;
    s_max[0]=23;s_min[0]=0;
  }
  else
  {
    s_withampm=true;
    s_max[0]=12;s_min[0]=1;
    convert_24_to_12(temp_alarm.hour, &hour, &s_is_am);
    temp_alarm.hour = hour;
    s_digits[2] = s_is_am;
  }
  s_selection = 0;
  if(temp_alarm.hour==0 && temp_alarm.minute==0)
  {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    convert_24_to_12(t->tm_hour, &hour, &s_is_am);
    s_digits[0] = clock_is_24h_style()?t->tm_hour:hour;
    s_digits[1] = t->tm_min;
    s_digits[2] = s_is_am;
  } else
  {
    s_digits[0] = temp_alarm.hour;
    s_digits[1] = temp_alarm.minute;
  }
  window_stack_push(s_time_window,true);
}

static void update_ui(Layer *layer, GContext *ctx) {
  
  for(int i = 0; i < 3; i++) {
#ifdef PBL_COLOR
    text_layer_set_background_color(s_input_layers[i], (i == s_selection) ? GColorDukeBlue : GColorDarkGray);
    if(i==s_selection)
    {
      GPoint selection_center = {
        .x = (int16_t) (s_withampm?23:50) + i * (PIN_WINDOW_SPACING + PIN_WINDOW_SPACING),
        .y = (int16_t) 50,
      };
      gpath_rotate_to(s_my_path_ptr, 0);
      gpath_move_to(s_my_path_ptr, selection_center);
      graphics_context_set_fill_color(ctx,GColorDukeBlue);
      gpath_draw_filled(ctx, s_my_path_ptr);
      gpath_rotate_to(s_my_path_ptr, TRIG_MAX_ANGLE/2);
      selection_center.y = 110;
      gpath_move_to(s_my_path_ptr, selection_center);
      gpath_draw_filled(ctx, s_my_path_ptr);
    }
#else
    text_layer_set_background_color(s_input_layers[i], (i == s_selection) ? GColorBlack : GColorWhite);
    text_layer_set_text_color(s_input_layers[i], (i == s_selection) ? GColorWhite : GColorBlack);
    if(i==s_selection)
    {
      GPoint selection_center = {
        .x = (int16_t) (s_withampm?23:50) + i * (PIN_WINDOW_SPACING + PIN_WINDOW_SPACING),
        .y = (int16_t) 50,
      };
      gpath_rotate_to(s_my_path_ptr, 0);
      gpath_move_to(s_my_path_ptr, selection_center);
      graphics_context_set_fill_color(ctx,GColorBlack);
      gpath_draw_filled(ctx, s_my_path_ptr);
      gpath_rotate_to(s_my_path_ptr, TRIG_MAX_ANGLE/2);
      selection_center.y = 110;
      gpath_move_to(s_my_path_ptr, selection_center);
      gpath_draw_filled(ctx, s_my_path_ptr);
    }
#endif
    if(i<2)
      snprintf(s_value_buffers[i], sizeof("00"), "%02d", s_digits[i]);
    else
      snprintf(s_value_buffers[i], sizeof("AM"), s_digits[i]?"AM":"PM");
    text_layer_set_text(s_input_layers[i], s_value_buffers[i]);
  }
  layer_set_hidden(text_layer_get_layer(s_input_layers[2]),!s_withampm);
  // draw the :
#ifdef PBL_COLOR
  graphics_context_set_text_color(ctx,GColorDukeBlue);
#else
  graphics_context_set_text_color(ctx,GColorBlack);
#endif
  graphics_draw_text(ctx,":",fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),GRect(s_withampm?144/2-27:144/2,58,40,20),
                     GTextOverflowModeWordWrap,GTextAlignmentLeft,NULL);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Next column
  s_selection++;
  
  if(s_selection == (s_withampm? 3:2)) {
    temp_alarm.hour = s_digits[0];
    temp_alarm.minute = s_digits[1];
    s_is_am = s_digits[2];
    if(!clock_is_24h_style()){
      if(s_is_am) {
        int hour = temp_alarm.hour;
        hour -= 12;
        if(hour<0) hour+=12;
        temp_alarm.hour = hour;
      } else {
        temp_alarm.hour = ((temp_alarm.hour+12)%12) + 12;
      }
    } 
    memcpy(current_alarm,&temp_alarm,sizeof(Alarm));      
    window_stack_pop(true);
    s_selection--;
  }
  else
    layer_mark_dirty(s_canvas_layer);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Previous column
  s_selection--;
  
  if(s_selection == -1) {
    window_stack_pop(true);
  }
  else
    layer_mark_dirty(s_canvas_layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_selection == 0 && s_withampm && s_digits[s_selection] == s_max[s_selection] - 1)
    s_digits[2] = !s_digits[2];
  
  s_digits[s_selection] += s_digits[s_selection] == s_max[s_selection] ? -s_max[s_selection] : 1;

  if(s_selection == 0 && s_withampm && s_digits[0] == 0)
    s_digits[0] = 1;
	
  layer_mark_dirty(s_canvas_layer);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_selection == 0 && s_withampm && s_digits[s_selection] == s_max[s_selection])
    s_digits[2] = !s_digits[2];
	  
  s_digits[s_selection] -= (s_digits[s_selection] == 0) ? -s_max[s_selection] : 1;
	
  if(s_selection == 0 && s_withampm && s_digits[0] == 0)
    s_digits[0] = s_max[0];
  
  layer_mark_dirty(s_canvas_layer);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 70, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 70, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void time_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // init hands
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_ui);
  layer_add_child(window_layer, s_canvas_layer);
  
  for(int i = 0; i < 3; i++) {
    s_input_layers[i] = text_layer_create(GRect((s_withampm?3:30) + i * (PIN_WINDOW_SPACING + PIN_WINDOW_SPACING), 60, 40, 40));
#ifdef PBL_COLOR
    text_layer_set_text_color(s_input_layers[i], GColorWhite);
    text_layer_set_background_color(s_input_layers[i], GColorDarkGray);
#else
    text_layer_set_text_color(s_input_layers[i], GColorBlack);
    text_layer_set_background_color(s_input_layers[i], GColorWhite);
#endif
    text_layer_set_text(s_input_layers[i], "00");
    text_layer_set_font(s_input_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(s_input_layers[i], GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_input_layers[i]));
  }
  window_set_click_config_provider(window, click_config_provider);
  layer_mark_dirty(s_canvas_layer);
}

static void time_window_unload(Window *window) {
  for(int i = 0; i < 3; i++) {
    text_layer_destroy(s_input_layers[i]);
  }
  layer_destroy(s_canvas_layer);
  //window_destroy(window);
}

void win_edit_init(void)
{
  s_time_window = window_create();
  window_set_window_handlers(s_time_window, (WindowHandlers) {
    .load = time_window_load,
    .unload = time_window_unload,
  });
  
//   tertiary_text_init();
#ifdef PBL_COLOR
//   check_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK_INV);
//   check_icon_inv = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK);
#else
//   check_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK);
#endif
  s_my_path_ptr= gpath_create(&PATH_INFO);
}
