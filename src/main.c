#include <pebble.h>

#define ANTIALIASING true
#define TESTING false
  
static Window *s_spin_window;
static TextLayer *s_time_layer, *s_top_text_layer, *s_spins_text_layer, *s_bottom_text_layer;
static TextLayer *s_heading_text_layer;
static TextLayer *s_status_layer;
static Layer *s_circle_canvas_layer;
static Layer *s_triangle_canvas_layer;
static GPoint s_center, s_circle_center;
static GPath *s_triangle_path;
static GPath *s_arrow_path;

static const GPathInfo BOLT_PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0, 0}, {0, -80}, {40, -80}}
};

static const GPathInfo ARROW_PATH_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0, 0}, {0, -20}, {20, -10}}
};

static const int16_t RADIUS = 58;
static const int16_t BORDER = 8;

static int16_t angle = -1;
static int16_t prev_compass_heading = 0;

static bool spinning = false;
static const int16_t MAX_SPINS = 2;
static int16_t spins = 0;

int16_t math_abs(int n){
  return n < 0 ? -n : n;
}

void set_angle(int16_t compass_heading){
  // Allocate a static output buffer
  static char s_buffer[32];
  static int16_t diff, deg;
  
  diff = math_abs(TRIGANGLE_TO_DEG(prev_compass_heading - compass_heading));
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "prev compass heading: %d", prev_compass_heading);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "compass heading: %d", compass_heading);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "diff: %d", diff);
  
  if (prev_compass_heading > compass_heading && diff > 8) {
    angle += 360 * 4 * TRIG_MAX_RATIO;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "clockwise");
  } else if(diff > 8) {
    angle -= 360 * 4 * TRIG_MAX_RATIO;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "counterclockwise");
  }
  prev_compass_heading = compass_heading;
  
  if (TESTING) {
    snprintf(s_buffer, sizeof(s_buffer), "Compass calibrated\nHeading: %d", TRIGANGLE_TO_DEG(compass_heading));
    text_layer_set_text(s_status_layer, s_buffer);
  }
  
  // figure this shit out
  deg = TRIGANGLE_TO_DEG(angle);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "deg: %d", deg);
  if(spins == 2 && deg > 0) {
    spins = 1;
  } else if(spins == 1 && deg <= 0) {
    spins = 2; // DONE
  } 
  
  // Set spin text
  snprintf(s_buffer, sizeof(s_buffer), "%d", MAX_SPINS - spins);
  text_layer_set_text(s_spins_text_layer, s_buffer);
  
  layer_mark_dirty(s_triangle_canvas_layer);
}

// Compass callback
void compass_handler(CompassHeadingData data) {
  if(!spinning) {
    return;
  }
  
  // Allocate a static output buffer
  static char s_buffer[32];
  
  // Determine status of the compass
  switch (data.compass_status) {
    // Compass data is not yet valid
    case CompassStatusDataInvalid:
      text_layer_set_text(s_status_layer, "Compass data invalid");
      break;

    // Compass is currently calibrating, but a heading is available
    case CompassStatusCalibrating:
      set_angle((int16_t)data.true_heading);
      break;
    // Compass data is ready for use, write the heading in to the buffer
    case CompassStatusCalibrated:
      set_angle((int16_t)data.true_heading);
      break;

    // CompassStatus is unknown
    default:
      snprintf(s_buffer, sizeof(s_buffer), "Unknown CompassStatus: %d", data.compass_status);
      text_layer_set_text(s_status_layer, s_buffer);
      break;
  }
}

static void update_triangle_proc(Layer *layer, GContext *ctx) {
  if(!spinning) {
    return;
  }
  
  // Move
  int16_t move_x = (int16_t)(sin_lookup(angle) * (RADIUS - 4) / TRIG_MAX_RATIO);
  int16_t move_y = (int16_t)(-cos_lookup(angle) * (RADIUS - 4) / TRIG_MAX_RATIO);
  gpath_move_to(s_arrow_path, GPoint(s_circle_center.x - move_x, s_circle_center.y + move_y));
  
  if(TESTING){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "move_x: %d", move_x);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "move_y: %d", move_y);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "angle: %d", TRIGANGLE_TO_DEG(angle));
  }
  
  // Rotate
  gpath_rotate_to(s_triangle_path, -angle);
  gpath_rotate_to(s_arrow_path, -angle);
  
   // Fill the path:
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, s_triangle_path);
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, s_arrow_path);
  
  if(TESTING){
    // Stroke the path:
//     graphics_context_set_stroke_color(ctx, GColorWhite);
//     gpath_draw_outline(ctx, s_triangle_path);
  }
}

static void update_circle_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_circle_center, RADIUS + BORDER);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, s_circle_center, RADIUS);
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
  text_layer_set_text(s_time_layer, buffer);
}

static void main_window_load(Window *window) {
  // Window layer properties
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  // Circle Layer
  s_center = grect_center_point(&window_bounds);
  s_circle_center = GPoint(s_center.x, s_center.y + 10);
  s_circle_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_circle_canvas_layer, update_circle_proc);
  layer_add_child(window_layer, s_circle_canvas_layer);
  
   // --- Triangle / Arrow Layer ---
  s_arrow_path = gpath_create(&ARROW_PATH_INFO);
  s_triangle_path = gpath_create(&BOLT_PATH_INFO);
  gpath_move_to(s_triangle_path, s_circle_center);
  s_triangle_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_triangle_canvas_layer, update_triangle_proc);
  layer_add_child(window_layer, s_triangle_canvas_layer);
  
  // Create heading TextLayer
  s_heading_text_layer = text_layer_create(GRect(0, 5, 144, 50));
  text_layer_set_text(s_heading_text_layer, "To turn off alarm");
  text_layer_set_background_color(s_heading_text_layer, GColorClear);
  text_layer_set_text_color(s_heading_text_layer, GColorWhite);
  text_layer_set_font(s_heading_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_heading_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_heading_text_layer));
  
  // Create top text TextLayer
  s_top_text_layer = text_layer_create(GRect(0, s_circle_center.y - 30, 144, 50));
  text_layer_set_text(s_top_text_layer, "Spin Around");
  text_layer_set_background_color(s_top_text_layer, GColorClear);
  text_layer_set_text_color(s_top_text_layer, GColorWhite);
  text_layer_set_font(s_top_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_top_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_top_text_layer));
  
  // Create spin text TextLayer
  s_spins_text_layer = text_layer_create(GRect(0, s_circle_center.y - 14, 144, 50));
  text_layer_set_text(s_spins_text_layer, "2");
  text_layer_set_background_color(s_spins_text_layer, GColorClear);
  text_layer_set_text_color(s_spins_text_layer, GColorWhite);
  text_layer_set_font(s_spins_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_spins_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_spins_text_layer));
  
  // Create bottom text TextLayer
  s_bottom_text_layer = text_layer_create(GRect(0, s_circle_center.y + 34 - 14, 144, 50));
  text_layer_set_text(s_bottom_text_layer, "Times!");
  text_layer_set_background_color(s_bottom_text_layer, GColorClear);
  text_layer_set_text_color(s_bottom_text_layer, GColorWhite);
  text_layer_set_font(s_bottom_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_bottom_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_bottom_text_layer));
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 65, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
//   layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Status heading TextLayer
  s_status_layer = text_layer_create(GRect(0, 100, 144, 30));
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_status_layer, "No data yet.");
  text_layer_set_background_color(s_status_layer, GColorClear);
  text_layer_set_overflow_mode(s_status_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_status_layer, GColorWhite);
  if(TESTING) layer_add_child(window_layer, text_layer_get_layer(s_status_layer));
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_status_layer);
    
    // Unsubscribe to acc. data service
    accel_data_service_unsubscribe();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// ----------------- CLICKS -----------------

static void spin_click_handler(ClickRecognizerRef recognizer, void *context) {
  spinning = true;
}

static void spin_release_handler(ClickRecognizerRef recognizer, void *context) {
  spinning = false;
}

void start_spin_click_config_provider(Window *window) {
  // Register the ClickHandlers
  window_long_click_subscribe(BUTTON_ID_UP, 100, spin_click_handler, spin_release_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 100, spin_click_handler, spin_release_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 100, spin_click_handler, spin_release_handler);
//   window_single_click_subscribe(BUTTON_ID_BACK, spin_click_handler);
}

// -------------- CLICKS END ---------------

static void init() {
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
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

  // Show the Window on the watch, with animated=true
  window_stack_push(s_spin_window, true);
  
  // Set click config handler
  window_set_click_config_provider(s_spin_window, (ClickConfigProvider) start_spin_click_config_provider);
  
  // Subscribe to the compass data service when angle changes by 5 degrees
  compass_service_subscribe(compass_handler);
  compass_service_set_heading_filter(5);
  
  // Make sure the time is displayed from the start
  update_time();
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