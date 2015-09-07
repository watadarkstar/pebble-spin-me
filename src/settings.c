#include "settings.h"
#include "main.h"
#include <pebble.h>
  
void settings_window_show(){
  // Show the Window on the watch, with animated=true
  window_stack_push(s_settings_window, true);
}

static uint16_t settings_num_sections(struct MenuLayer* menu, void* callback_context) {
  return 1;
}

static void settings_select(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context) {
  bool is_enabled;
  
  switch (cell_index->row) {
    case MENU_ENABLE_DISABLE:
      is_enabled = persist_exists(SETTINGS_IS_ENABLED_KEY) ? persist_read_bool(SETTINGS_IS_ENABLED_KEY) : false;
      persist_write_bool(SETTINGS_IS_ENABLED_KEY, !is_enabled);
      layer_mark_dirty((Layer *)s_settings_menu_layer);
      break;
    case MENU_TUTORIAL:
      spin_window_show();
      break;
  }
}

static uint16_t settings_num_rows (struct MenuLayer *menulayer, uint16_t section_index, void *callback_context) {
  return NUM_MENU;
}

static void settings_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  static char s_buffer[32];

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorWhite);
  GSize size = layer_get_frame(cell_layer).size;
  graphics_fill_rect(ctx,GRect(0,0,size.w,size.h),0,GCornerNone);
  
  
  switch (cell_index->row) {
      case MENU_EDIT:
        snprintf(s_buffer, sizeof(s_buffer), "Edit");
        break;
      case MENU_ENABLE_DISABLE:
        snprintf(s_buffer, sizeof(s_buffer), "Enable");
        if (persist_read_bool(SETTINGS_IS_ENABLED_KEY)){
          snprintf(s_buffer, sizeof(s_buffer), "Disable"); 
        }
        break;
      case MENU_TUTORIAL:
        snprintf(s_buffer, sizeof(s_buffer), "Tutorial");
        break;
      default:
        break;
  }
  
  graphics_draw_text(ctx, s_buffer,
                     fonts_get_system_font(FONT_KEY_GOTHIC_28),
                     GRect(3, 0, size.w, size.h), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
}

static void settings_draw_header(GContext* ctx, const Layer* cell_layer, uint16_t section_index, void* callback_context) {
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx,GRect(0,1,144,14),0,GCornerNone);
  
  graphics_draw_text(ctx, "test",
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(3, -2, 144 - 33, 14), GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
}

static int16_t settings_header_height(struct MenuLayer *menu, uint16_t section_index, void *callback_context) {
  return 16;
}
  
static void settings_window_load(Window *window){
  // Window layer properties
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  s_settings_menu_layer = menu_layer_create(window_bounds);
  
  menu_layer_set_callbacks(s_settings_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = settings_num_sections,
    .get_num_rows = settings_num_rows,
    .draw_row = settings_draw_row,
    .select_click = settings_select,
    .draw_header = settings_draw_header,
    .get_header_height = settings_header_height,
  });
  
  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_settings_menu_layer, window);
  
  layer_add_child(window_layer, menu_layer_get_layer(s_settings_menu_layer));
}

static void settings_window_unload(Window *window){
}
  
void settings_window_init(){
  // Create settings Window element and assign to pointer
  s_settings_window = window_create();
  
  #ifdef PBL_SDK_2
  window_set_fullscreen(s_settings_window, true);
  #endif

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_settings_window, (WindowHandlers) {
    .load = settings_window_load,
    .unload = settings_window_unload
  });
}