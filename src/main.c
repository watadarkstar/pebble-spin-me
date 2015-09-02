#include <pebble.h>

#define ANTIALIASING true
#define TESTING true
  
static Window *s_main_window;
static TextLayer *s_time_layer;
// static TextLayer *s_acc_layer;
static TextLayer *s_compass_heading_layer;
static Layer *s_circle_canvas_layer;
static Layer *s_triangle_canvas_layer;
static GPoint s_center;
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

static const int RADIUS = 62;
static const int BORDER = 8;
static int rotation = 0;
static int full_rotations = 0;
static int full_rotations_state = 0;

// Compass callback
void compass_handler(CompassHeadingData data) {
  // Allocate a static output buffer
  static char s_buffer[32];
  
  // Determine status of the compass
  switch (data.compass_status) {
    // Compass data is not yet valid
    case CompassStatusDataInvalid:
      text_layer_set_text(s_compass_heading_layer, "Compass data invalid");
      break;

    // Compass is currently calibrating, but a heading is available
    case CompassStatusCalibrating:
      rotation = (int)data.true_heading;
      snprintf(s_buffer, sizeof(s_buffer), "Heading: %d", TRIGANGLE_TO_DEG(rotation));
//       snprintf(s_buffer, sizeof(s_buffer), "Compass calibrating\nHeading: %d", TRIGANGLE_TO_DEG(rotation));
      text_layer_set_text(s_compass_heading_layer, s_buffer);
      layer_mark_dirty(s_triangle_canvas_layer);
      break;
    // Compass data is ready for use, write the heading in to the buffer
    case CompassStatusCalibrated:
      rotation = (int)data.true_heading;
      snprintf(s_buffer, sizeof(s_buffer), "Compass calibrated\nHeading: %d", TRIGANGLE_TO_DEG(rotation));
//       snprintf(s_buffer, sizeof(s_buffer), "Compass calibrated\nHeading: %d", TRIGANGLE_TO_DEG((int)data.true_heading));
      text_layer_set_text(s_compass_heading_layer, s_buffer);
      layer_mark_dirty(s_triangle_canvas_layer);  
      break;

    // CompassStatus is unknown
    default:
      snprintf(s_buffer, sizeof(s_buffer), "Unknown CompassStatus: %d", data.compass_status);
      text_layer_set_text(s_compass_heading_layer, s_buffer);
      break;
  }
}

static void update_triangle_proc(Layer *layer, GContext *ctx) {
  // Move
  int move_x = (-cos_lookup(rotation) / TRIG_MAX_RATIO * RADIUS);
  int move_y = (sin_lookup(rotation) / TRIG_MAX_RATIO * RADIUS);
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "cos_lookup: %d", (int)TRIGANGLE_TO_DEG(cos_lookup(rotation)));
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "sin_lookup: %d", (int)TRIGANGLE_TO_DEG(sin_lookup(rotation)));  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "move_x: %d", move_x);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "move_y: %d", move_y);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "angle: %d", TRIGANGLE_TO_DEG(rotation));
  gpath_move_to(s_arrow_path, GPoint(s_center.x + move_x, s_center.y + move_y));
  
  // Rotate
  gpath_rotate_to(s_triangle_path, -rotation);
  gpath_rotate_to(s_arrow_path, -rotation);
  
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
  graphics_fill_circle(ctx, s_center, RADIUS + BORDER);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, s_center, RADIUS);
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
  s_circle_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_circle_canvas_layer, update_circle_proc);
  layer_add_child(window_layer, s_circle_canvas_layer);
  
   // --- Triangle / Arrow Layer ---
  s_arrow_path = gpath_create(&ARROW_PATH_INFO);
  s_triangle_path = gpath_create(&BOLT_PATH_INFO);
  gpath_move_to(s_triangle_path, s_center);
  s_triangle_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_triangle_canvas_layer, update_triangle_proc);
  layer_add_child(window_layer, s_triangle_canvas_layer);
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 65, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Create compass heading TextLayer
  s_compass_heading_layer = text_layer_create(GRect(0, 100, 144, 30));
  if (TESTING) {
    text_layer_set_font(s_compass_heading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text(s_compass_heading_layer, "No data yet.");
    text_layer_set_background_color(s_compass_heading_layer, GColorClear);
    text_layer_set_overflow_mode(s_compass_heading_layer, GTextOverflowModeWordWrap);
    text_layer_set_text_alignment(s_compass_heading_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_compass_heading_layer, GColorWhite);
    layer_add_child(window_layer, text_layer_get_layer(s_compass_heading_layer));
  }
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_compass_heading_layer);
    
    // Unsubscribe to acc. data service
    accel_data_service_unsubscribe();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init() {
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  
  #ifdef PBL_SDK_2
  window_set_fullscreen(s_main_window, true);
  #endif

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Subscribe to the compass data service when angle changes by 5 degrees
  compass_service_subscribe(compass_handler);
  compass_service_set_heading_filter(5);
  
  // Make sure the time is displayed from the start
  update_time();
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}