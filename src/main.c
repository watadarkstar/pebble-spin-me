#include <pebble.h>
#include "main.h"
#include "settings.h"
#include "edit.h"

#define TESTING false

// Spin constants
static const int16_t RADIUS = 58;
static const int16_t BORDER = 8;
static const int16_t MAX_SPINS = 2;
  
// Spin window
static Window *s_spin_window;

// Spin Layers
static TextLayer *s_spin_time_layer, *s_spin_heading_text_layer, *s_spin_top_text_layer, *s_spin_spins_text_layer, *s_spin_bottom_text_layer;
static Layer *s_spin_circle_canvas_layer;
static Layer *s_spin_triangle_canvas_layer;

// Spin welcome Layers
static TextLayer *s_welcome_text_layer;
static Layer *s_welcome_canvas_layer;

// Spin paths and points
static GPoint s_center, s_spin_circle_center;
static GPath *s_spin_triangle_path;
static GPath *s_spin_arrow_path;

static const GPathInfo BOLT_PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0, 0}, {0, -80}, {40, -80}}
};

static const GPathInfo SPIN_ARROW_PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0, 0}, {0, -20}, {20, -10}}
};

// Spin welcome paths
static GPath *s_welcome_arrow_path;
static GRect s_welcome_rect;
// static const GPathInfo WELCOME_ARROW_PATH_INFO = {
//   .num_points = 3,
//   .points = (GPoint []) {{0, 0}, {0, -20}, {20, -10}}
// };

// Spin state stuff
static int32_t angle = -1;
static int32_t prev_compass_heading = 0;
static bool s_spinning = false;
static int16_t spins = 0;

int32_t math_abs(int32_t n){
  return n < 0 ? -n : n;
}

static void spin_set_hidden(bool hidden){
  layer_set_hidden((Layer *)s_spin_heading_text_layer, hidden);
  layer_set_hidden((Layer *)s_spin_top_text_layer, hidden);
  layer_set_hidden((Layer *)s_spin_spins_text_layer, hidden);
  layer_set_hidden((Layer *)s_spin_bottom_text_layer, hidden);
}

static void set_spinning(bool spinning){
  s_spinning = spinning;
  spin_set_hidden(!s_spinning);
  if(!spinning) {
    layer_mark_dirty(s_spin_triangle_canvas_layer);
    layer_mark_dirty(s_spin_circle_canvas_layer);
  }
}

static void set_alarm_on(bool on){
  if(!on){
    // off state
    set_spinning(false);
    window_stack_remove(s_spin_window, true);
    return;
  } 
  // on state
  spins = 0;
  set_spinning(false);
}

void set_spin_angle(int32_t compass_heading){
  if(!s_spinning) {
    return;
  }
  
  // Allocate a static output buffer
  static char s_buffer[32];
  static int32_t diff, deg;
  
  diff = math_abs(TRIGANGLE_TO_DEG(prev_compass_heading - compass_heading));
  
  if (TESTING){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "prev compass heading: %d", (int)prev_compass_heading);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "compass heading: %d", (int)compass_heading);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "diff: %d", (int)diff);
  }
    
  if (prev_compass_heading > compass_heading && diff > 5) {
    angle -= 10 * TRIG_MAX_ANGLE / 360;
    if(TESTING) APP_LOG(APP_LOG_LEVEL_DEBUG, "clockwise");
  } else if(diff > 5) {
    angle += 10 * TRIG_MAX_ANGLE / 360;
    if(TESTING) APP_LOG(APP_LOG_LEVEL_DEBUG, "counterclockwise");
  }
  prev_compass_heading = compass_heading;
  
  // Set the number of spins completed
  deg = TRIGANGLE_TO_DEG(angle);
  spins = (int16_t)((math_abs(deg) + 20) / 180);
  
  // Turn off alarm
  if (spins == MAX_SPINS) {
    set_alarm_on(false);
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "deg: %d", (int)deg);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "spins: %d", (int)deg);
  
  // Set spin text
  snprintf(s_buffer, sizeof(s_buffer), "%d", MAX_SPINS - spins);
  text_layer_set_text(s_spin_spins_text_layer, s_buffer);
  
  layer_mark_dirty(s_spin_triangle_canvas_layer);
}

// Compass callback
void compass_handler(CompassHeadingData data) {
  // Determine status of the compass
  switch (data.compass_status) {
    // Compass data is not yet valid
    case CompassStatusDataInvalid:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Compass data invalid, got: %d", (int)TRIGANGLE_TO_DEG(data.true_heading));
      break;

    // Compass is currently calibrating, but a heading is available
    case CompassStatusCalibrating:
      set_spin_angle((int32_t)data.true_heading);
      break;
    // Compass data is ready for use, write the heading in to the buffer
    case CompassStatusCalibrated:
      set_spin_angle((int32_t)data.true_heading);
      break;

    // CompassStatus is unknown
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown CompassStatus: %d", data.compass_status);
      break;
  }
}

static void update_triangle_proc(Layer *layer, GContext *ctx) {
  if(!s_spinning) {
    return;
  }
  
  // Move
  int32_t move_x = (int32_t)(sin_lookup(angle) * (RADIUS - 4) / TRIG_MAX_RATIO);
  int32_t move_y = (int32_t)(-cos_lookup(angle) * (RADIUS - 4) / TRIG_MAX_RATIO);
  gpath_move_to(s_spin_arrow_path, GPoint(s_spin_circle_center.x - move_x, s_spin_circle_center.y + move_y));
  
  if(TESTING){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "move_x: %d", (int)move_x);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "move_y: %d", (int)move_y);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "angle: %d", (int)TRIGANGLE_TO_DEG(angle));
  }
  
  // Rotate
  gpath_rotate_to(s_spin_triangle_path, -angle);
  gpath_rotate_to(s_spin_arrow_path, -angle);
  
  // Fill the path:
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, s_spin_triangle_path);
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, s_spin_arrow_path);
  
  if(TESTING){
    // Stroke the path:
//     graphics_context_set_stroke_color(ctx, GColorWhite);
//     gpath_draw_outline(ctx, s_triangle_path);
  }
}

static void update_welcome_proc(Layer *layer, GContext *ctx){
  // Fill the path:
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, s_welcome_arrow_path);
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, s_welcome_arrow_path);
  graphics_fill_rect(ctx, s_welcome_rect, 0, GCornerNone );

}

static void update_spin_circle_proc(Layer *layer, GContext *ctx) {
  if(!s_spinning){
    return;
  }
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_spin_circle_center, RADIUS + BORDER);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, s_spin_circle_center, RADIUS);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%h:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00 00"), "%I:%M %p", tick_time);
  }
  
  // Remove the leading zero
  if (buffer[0] == '0') {
    memmove(&buffer[0], &buffer[1], sizeof(buffer) - 1);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_spin_time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// ----------------- CLICKS -----------------

static void spin_click_handler(ClickRecognizerRef recognizer, void *context) {
  set_spinning(true);
}

static void spin_release_handler(ClickRecognizerRef recognizer, void *context) {
  set_spinning(false);
}

void start_spin_click_config_provider(Window *window) {
  // Register the ClickHandlers
  window_long_click_subscribe(BUTTON_ID_UP, 100, spin_click_handler, spin_release_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 100, spin_click_handler, spin_release_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 100, spin_click_handler, spin_release_handler);
//   window_single_click_subscribe(BUTTON_ID_BACK, spin_click_handler);
}

// -------------- CLICKS END ---------------


static void main_window_load(Window *window) {
  // Set click config handler
  window_set_click_config_provider(s_spin_window, (ClickConfigProvider) start_spin_click_config_provider);
  
  // Window layer properties
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  // Points
  s_center = grect_center_point(&window_bounds);
  s_spin_circle_center = GPoint(s_center.x, s_center.y + 10);
  
  // Rects
  s_welcome_rect = GRect(112, s_center.y, 10, 10);
  
  // Welcome arrow
  s_welcome_arrow_path = gpath_create(&SPIN_ARROW_PATH_INFO);
  gpath_move_to(s_welcome_arrow_path, GPoint(120, s_center.y + 15));
  s_welcome_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_welcome_canvas_layer, update_welcome_proc);
  layer_add_child(window_layer, s_welcome_canvas_layer);
  
  // Welcome text layer
  s_welcome_text_layer = text_layer_create(GRect(10, s_center.y - 12, 144, 50));
  text_layer_set_text(s_welcome_text_layer, "Press and hold");
  text_layer_set_background_color(s_welcome_text_layer, GColorClear);
  text_layer_set_text_color(s_welcome_text_layer, GColorWhite);
  text_layer_set_font(s_welcome_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_layer, text_layer_get_layer(s_welcome_text_layer));
  
  // Spin circle Layer
  s_spin_circle_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_spin_circle_canvas_layer, update_spin_circle_proc);
  layer_add_child(window_layer, s_spin_circle_canvas_layer);
  
   // Spin triangle / arrow Layer
  s_spin_arrow_path = gpath_create(&SPIN_ARROW_PATH_INFO);
  s_spin_triangle_path = gpath_create(&BOLT_PATH_INFO);
  gpath_move_to(s_spin_triangle_path, s_spin_circle_center);
  s_spin_triangle_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_spin_triangle_canvas_layer, update_triangle_proc);
  layer_add_child(window_layer, s_spin_triangle_canvas_layer);
  
  // Spin heading TextLayer
  s_spin_heading_text_layer = text_layer_create(GRect(0, 5, 144, 50));
  text_layer_set_text(s_spin_heading_text_layer, "To turn off alarm");
  text_layer_set_background_color(s_spin_heading_text_layer, GColorClear);
  text_layer_set_text_color(s_spin_heading_text_layer, GColorWhite);
  text_layer_set_font(s_spin_heading_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_spin_heading_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_spin_heading_text_layer));
  
  // Spin top text TextLayer
  s_spin_top_text_layer = text_layer_create(GRect(0, s_spin_circle_center.y - 30, 144, 50));
  text_layer_set_text(s_spin_top_text_layer, "Spin Around");
  text_layer_set_background_color(s_spin_top_text_layer, GColorClear);
  text_layer_set_text_color(s_spin_top_text_layer, GColorWhite);
  text_layer_set_font(s_spin_top_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_spin_top_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_spin_top_text_layer));
  
  // Spin spins TextLayer
  s_spin_spins_text_layer = text_layer_create(GRect(0, s_spin_circle_center.y - 14, 144, 50));
  text_layer_set_text(s_spin_spins_text_layer, "2");
  text_layer_set_background_color(s_spin_spins_text_layer, GColorClear);
  text_layer_set_text_color(s_spin_spins_text_layer, GColorWhite);
  text_layer_set_font(s_spin_spins_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_spin_spins_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_spin_spins_text_layer));
  
  // Spin bottom text TextLayer
  s_spin_bottom_text_layer = text_layer_create(GRect(0, s_spin_circle_center.y + 34 - 14, 144, 50));
  text_layer_set_text(s_spin_bottom_text_layer, "Times!");
  text_layer_set_background_color(s_spin_bottom_text_layer, GColorClear);
  text_layer_set_text_color(s_spin_bottom_text_layer, GColorWhite);
  text_layer_set_font(s_spin_bottom_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_spin_bottom_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_spin_bottom_text_layer));
  
  // Spin time TextLayer
  s_spin_time_layer = text_layer_create(GRect(0, s_spin_circle_center.y + 38, 144, 50));
  text_layer_set_background_color(s_spin_time_layer, GColorClear);
  text_layer_set_text_color(s_spin_time_layer, GColorWhite);
  text_layer_set_font(s_spin_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_spin_time_layer, GTextAlignmentCenter);
//   layer_add_child(window_layer, text_layer_get_layer(s_spin_time_layer));
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Subscribe to the compass data service when angle changes by 5 degrees
  compass_service_subscribe(compass_handler);
  compass_service_set_heading_filter(5);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // TODO: remove this
  set_alarm_on(true);
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_spin_time_layer);
    
    // Unsubscribe to acc. data service
    accel_data_service_unsubscribe();
}

void spin_window_show(){
  // Show the Window on the watch, with animated=true
  window_stack_push(s_spin_window, true);
}

static void init() {
  // Create spin Window element and assign to pointer
  s_spin_window = window_create();
  window_set_background_color(s_spin_window, GColorBlack);
  
  #ifdef PBL_SDK_2
  window_set_fullscreen(s_spin_window, true);
  #endif

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_spin_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Init the settings window
  settings_window_init();
  
  // Init the edit window
  win_edit_init();
  
  // Show the settings window
  settings_window_show();
//   spin_window_show();
}

static void deinit() {
  // Destroy Window
  window_destroy(s_spin_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}