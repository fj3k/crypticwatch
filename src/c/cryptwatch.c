#include <pebble.h>
#include "cryptwatch.h"

static Window *s_window;
static Layer *s_simple_bg_layer, *s_hands_layer;
static GPoint s_points[19];
static GColor s_pallette[4];

static int s_forecast[8];
static int s_temperature[8];
static time_t s_last_update;

long a_rand() {
  static long seed = 100;
  seed = (((seed * 214013L + 2531011L) >> 16) & 32767);

  return (seed % 1000);
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  GRect bounds = layer_get_bounds(layer);
  GPoint centre = grect_center_point(&bounds);

  graphics_context_set_antialiased(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  for (int i = 0; i <= 18; i++) {
    graphics_draw_circle(ctx, s_points[i], 9);
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_antialiased(ctx, 1);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  if (t->tm_hour >= 12) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[0], 10);
  }
  if (t->tm_hour % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[1], 10);
  }
  if (t->tm_hour % 12 / 2 % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[2], 10);
  }
  if (t->tm_hour % 12 / 4 % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[11], 10);
  }
  if (t->tm_hour % 12 / 8 % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[12], 10);
  }
  if (t->tm_min % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[7], 10);
  }
  if (t->tm_min / 2 % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[8], 10);
  }
  if (t->tm_min / 4 % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[5], 10);
  }
  if (t->tm_min / 8 % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[6], 10);
  }
  if (t->tm_min / 16 % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[3], 10);
  }
  if (t->tm_min / 32 % 2) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[4], 10);
  }

  int weatherstart = 0;
  if (s_last_update) {
    struct tm *wt = localtime(&s_last_update);
    //Find midnight at the start of the last update day.
    time_t dayOfUpdate = s_last_update - wt->tm_hour * 3600 + wt->tm_min * 60 + wt->tm_sec;
    weatherstart = (now - dayOfUpdate) / 86400;
  }
  if (s_temperature[weatherstart] != -40 && (s_temperature[weatherstart] < 15 || s_temperature[weatherstart] > 35)) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 2 + (s_temperature[weatherstart] > 25 ? 2 : 0)]);
    graphics_fill_circle(ctx, s_points[9], 10);
  }
  if (s_forecast[weatherstart] == WEATHER_RAIN || s_forecast[weatherstart] == WEATHER_SHOWERS || s_forecast[weatherstart] == WEATHER_STORM || s_forecast[weatherstart] == WEATHER_SNOW) {
    graphics_context_set_fill_color(ctx, s_pallette[a_rand() % 4]);
    graphics_fill_circle(ctx, s_points[10], 10);
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));

  //Ask for weather update. Should be removed when the app does this.
  if(tick_time->tm_hour % 6 == 0 && tick_time->tm_min == 30) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  for (int i = 0; i < 8; i++) {
    Tuple *notif_forecast_t = dict_find(iter, MESSAGE_KEY_Forecast + i);
    if (notif_forecast_t) {
      switch (notif_forecast_t->value->int32) {
        case 1:  //Sunny
        case 2:  //Clear
          s_forecast[i] = WEATHER_SUN;
          break;
        case 3:  //Partly cloudy
          s_forecast[i] = WEATHER_PARTCLOUD;
          break;
        case 4:  //Cloudy
          s_forecast[i] = WEATHER_CLOUD;
          break;
        case 10: //Fog
          s_forecast[i] = WEATHER_FOG;
          break;
        case 6:  //Hazy
          s_forecast[i] = WEATHER_HAZE;
          break;
        case 8:  //Light rain
        case 12: //Rain
          s_forecast[i] = WEATHER_RAIN;
          break;
        case 11: //Shower
        case 17: //Light shower
          s_forecast[i] = WEATHER_SHOWERS;
          break;
        case 16: //Storm
          s_forecast[i] = WEATHER_STORM;
          break;
        case 9:  //Windy
          s_forecast[i] = WEATHER_WINDY;
          break;
        case 13: //Dusty
          s_forecast[i] = WEATHER_DUSTSTORM;
          break;
        case 14: //Frost
        case 15: //Snow
          s_forecast[i] = WEATHER_SNOW;
          break;
        default:
          s_forecast[i] = WEATHER_NONE;
      }
      if (i == 0) {
        s_last_update = time(NULL);
      }
    }
    Tuple *notif_temperature_t = dict_find(iter, MESSAGE_KEY_Temperature + i);
    if (notif_temperature_t) {
      s_temperature[i] = notif_temperature_t->value->int32;
    }
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  int spacing = (bounds.size.w - (5 * 20)) / 7 + 20;
  s_points[0] = grect_center_point(&bounds);
  for (int i = 1; i < 12; i += 2) {
    int32_t tick_angle = TRIG_MAX_ANGLE * i / 12;
    s_points[i] = GPoint(
      (int16_t)(sin_lookup(tick_angle) * spacing / TRIG_MAX_RATIO) + s_points[0].x,
      (int16_t)(-cos_lookup(tick_angle) * spacing / TRIG_MAX_RATIO) + s_points[0].y
    );
    s_points[i+1] = GPoint(
      (int16_t)(sin_lookup(tick_angle) * spacing * 2 / TRIG_MAX_RATIO) + s_points[0].x,
      (int16_t)(-cos_lookup(tick_angle) * spacing * 2 / TRIG_MAX_RATIO) + s_points[0].y
    );
  }
  for (int i = 0; i < 6; i++) {
    int32_t tick_angle = TRIG_MAX_ANGLE * i / 6;
    s_points[i+13] = GPoint(
      (int16_t)(sin_lookup(tick_angle) * spacing * 2 / TRIG_MAX_RATIO) + s_points[0].x,
      (int16_t)(-cos_lookup(tick_angle) * spacing * 2 / TRIG_MAX_RATIO) + s_points[0].y
    );
  }

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_hands_layer);
}

static void init() {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  // Open AppMessage connection
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(128, 128);

  if (PBL_IF_COLOR_ELSE(true, false)) {
    s_pallette[0] = GColorVeryLightBlue;
    s_pallette[1] = GColorInchworm;
    s_pallette[2] = GColorSunsetOrange;
    s_pallette[3] = GColorLavenderIndigo;
  } else {
    s_pallette[0] = GColorWhite;
    s_pallette[1] = GColorWhite;
    s_pallette[2] = GColorWhite;
    s_pallette[3] = GColorWhite;
  }
  Layer *window_layer = window_get_root_layer(s_window);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
