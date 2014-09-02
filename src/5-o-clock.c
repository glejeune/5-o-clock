#include <pebble.h>

static Window *window;
static InverterLayer *main_layer;
static GPath *minute_arrow;
static GPath *hour_arrow;
bool bluetooth_connected = true;

static const GPathInfo MINUTE_HAND_POINTS =
{
  32, (GPoint []) {
    { 0, 0 },
    { 18, -68 },
    { 17, -68 },
    { 16, -68 },
    { 15, -68 },
    { 13, -69 },
    { 12, -69 },
    { 11, -69 },
    { 10, -69 },
    { 9, -69 },
    { 7, -70 },
    { 6, -70 },
    { 5, -70 },
    { 4, -70 },
    { 2, -70 },
    { 1, -70 },
    { 0, -70 },
    { -1, -70 },
    { -2, -70 },
    { -4, -70 },
    { -5, -70 },
    { -6, -70 },
    { -7, -70 },
    { -9, -69 },
    { -10, -69 },
    { -11, -69 },
    { -12, -69 },
    { -13, -69 },
    { -15, -68 },
    { -16, -68 },
    { -17, -68 },
    { -18, -68 }
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  32, (GPoint []){
    {0, 0},
    { 13, -48 },
    { 12, -49 },
    { 11, -49 },
    { 10, -49 },
    { 10, -49 },
    { 9, -49 },
    { 8, -49 },
    { 7, -50 },
    { 6, -50 },
    { 5, -50 },
    { 4, -50 },
    { 3, -50 },
    { 3, -50 },
    { 2, -50 },
    { 1, -50 },
    { 0, -50 },
    { -1, -50 },
    { -2, -50 },
    { -3, -50 },
    { -3, -50 },
    { -4, -50 },
    { -5, -50 },
    { -6, -50 },
    { -7, -50 },
    { -8, -49 },
    { -9, -49 },
    { -10, -49 },
    { -10, -49 },
    { -11, -49 },
    { -12, -49 },
    { -13, -48 }
  }
};

static void handle_bluetooth(bool connected) {
  if (bluetooth_connected != connected) {
    vibes_long_pulse();
    bluetooth_connected = connected;
  }
}

void main_layer_update_callback(Layer *me, GContext* ctx) {
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  int min = current_time->tm_min;
  int min_approx = ((min + (-1 * ((min - 3) % 5))) % 60) + 2;
  int hour = current_time->tm_hour;
  int sec = current_time->tm_sec;

  GRect bounds = layer_get_bounds(me);
  const GPoint center = grect_center_point(&bounds);
  const int16_t secondHandLength = (bounds.size.w / 2) - 4;

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(minute_arrow, TRIG_MAX_ANGLE * min_approx / 60);
  gpath_draw_filled(ctx, minute_arrow);
  gpath_draw_outline(ctx, minute_arrow);

  gpath_rotate_to(hour_arrow, TRIG_MAX_ANGLE * (hour % 12) / 12);
  gpath_draw_filled(ctx, hour_arrow);
  gpath_draw_outline(ctx, hour_arrow);

  if (bluetooth_connected) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorBlack);
  } else {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }

  GPoint secondHand;
  int32_t second_angle = TRIG_MAX_ANGLE * sec / 60;
  secondHand.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.y;
  secondHand.x = (int16_t)(sin_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.x;
  graphics_fill_circle(ctx, secondHand, 3);
  graphics_draw_circle(ctx, secondHand, 4);
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  layer_mark_dirty(inverter_layer_get_layer(main_layer));
}

static void init(void) {
  handle_bluetooth(bluetooth_connection_service_peek());
  bluetooth_connection_service_subscribe(&handle_bluetooth);

  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);

  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);

  main_layer = inverter_layer_create(frame);  
  layer_set_update_proc(inverter_layer_get_layer(main_layer), main_layer_update_callback);
  layer_add_child(root_layer, inverter_layer_get_layer(main_layer));

  minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  GRect bounds = layer_get_bounds(inverter_layer_get_layer(main_layer));
  const GPoint center = grect_center_point(&bounds);
  gpath_move_to(minute_arrow, center);
  gpath_move_to(hour_arrow, center);

  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  gpath_destroy(minute_arrow);
  gpath_destroy(hour_arrow);
  inverter_layer_destroy(main_layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
