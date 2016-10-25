#include <pebble.h>
#include "utilities.h"
//STATIC for a global variable or a function: only accessible from this file; for a variable inside a function: the variable keeps its value between function calls

/*** Constants ***/
#define PERSISTKEY_WAKEUP_ID 1 //Persistent storage key for the wakeup id.
#define PERSISTKEY_WAKEUP_TIME 2 //Persistent storage key for the wakeup time.
#define WAKE_UP_MIN_BEFORE 15 //Wake up X minutes before the train leaves
#define NUMBER_OF_LAYERS 20 //Number of layers in the layer list.
#define TIME_BUFFER_SIZE 8 //Size of the countdown char buffer.
#define TRIP_IN_LAYER_WIDTH 0.8 //Percentage width and height of the station information (name and departure/arrival time). There will be a margin left of TRIP_IN_LAYER_WIDTH/2 per side.
#define TRIP_IN_LAYER_HEIGHT 0.8
#define STATION_NAME_WIDTH 0.7 //Percentage of TRIP_LAYER_WIDTH and TRIP_LAYER_HEIGHT. The departure and arrival times layer width will be 1-STATION_NAME_WIDTH.
#define STATION_NAME_HEIGHT 0.3 //The countdown height will be 1-2*STATION_NAME_HEIGHT.


/*** Global variables ***/
static Layer* layerList[NUMBER_OF_LAYERS]; //List of layers, they will be added one after another and deleted from last to first. It won't account for window root layers
static int layerListIndex = 0;

static Window* trip_window; //Window for the train trip, which shows departure and arrival stations and times and countdown.
static Layer* in_trip_layer; //Layer for departure and arrival stations and times and countdown. It will have some margin from the trip_window root layer.
static TextLayer* countdown_layer;
static TextLayer* dep_station_layer;
static TextLayer* dep_time_layer;
static TextLayer* arr_station_layer;
static TextLayer* arr_time_layer;

static time_t current_time;
static char time_buffer[TIME_BUFFER_SIZE];
static char dep_time_buffer[TIME_BUFFER_SIZE];
static char arr_time_buffer[TIME_BUFFER_SIZE];
static struct train_trip trainPlcatSqv1836;

/*** Helper functions ***/
static void add_child_layer(Layer* parent, Layer* child) {
  layer_add_child(parent, child);
  layerList[layerListIndex] = child;
  layerListIndex++;
}
  
static void destroy_all_layers(){
  for (int i=layerListIndex; i >= 0; i--) { //parent layers will always be at a lower index, so we start from the last one.
    layer_destroy(layerList[i]);
  }
  layerListIndex = 0;
}

/*
static void scheduleWakeup(){
  //Cancel all wakeups
  wakeup_cancel_all();
  
  //Set a new one: later today or tomorrow?
  struct tm wakeup_time;
  struct tm* now = localtime(current_time);
  if (now->tm_hour > ) {
    wakeup_time = current_time;
  } else {
    
  }
  wakeup_schedule(mktime(wakeup_time), 0, FALSE);
}
*/

static void update_time() {
  //Get a tm structure
  current_time = time(NULL);
}

static bool countdown_update(TextLayer *text_layer) {
  bool res = TRUE;
  // Display this time on the TextLayer
  struct tm* time_now = localtime(&current_time);
  struct time_ms timeToTrain = time_diff(trainPlcatSqv1836.depTime, time_now);
  
  if (timeToTrain.min < 60) {
    if (! write_time_ms(time_buffer, TIME_BUFFER_SIZE, timeToTrain, TRUE)){
      strcpy(time_buffer, ""); //if something went wrong, set buffer to "".
      res = FALSE;
    }
    text_layer_set_text(countdown_layer, time_buffer);
  } else {
    res = FALSE;
  }
  return res;
}

static void second_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  countdown_update(countdown_layer);
}

static void init_text_layer(TextLayer *text_layer) {
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorBlack);
}

/*** Init / deinit ***/
static void trip_window_load(Window *window) {
  // Get information about the Window
  Layer* window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create layer inside window to have margin
  GRect in_trip_layer_bounds;
  in_trip_layer_bounds.origin.x = window_bounds.size.w*(1-TRIP_IN_LAYER_WIDTH)/2;
  in_trip_layer_bounds.origin.y = window_bounds.size.h*(1-TRIP_IN_LAYER_HEIGHT)/2;
  in_trip_layer_bounds.size.w = window_bounds.size.w*TRIP_IN_LAYER_WIDTH;
  in_trip_layer_bounds.size.h = window_bounds.size.h*TRIP_IN_LAYER_HEIGHT;
  in_trip_layer = layer_create(in_trip_layer_bounds);
  add_child_layer(window_layer, in_trip_layer);
  
  // Create departure station name field
  GRect dep_station_bounds = {{0,0},{in_trip_layer_bounds.size.w*STATION_NAME_WIDTH, in_trip_layer_bounds.size.h*STATION_NAME_HEIGHT}};
  dep_station_layer = text_layer_create(dep_station_bounds);
  add_child_layer(in_trip_layer,  text_layer_get_layer(dep_station_layer));
  init_text_layer(dep_station_layer);
  text_layer_set_font(dep_station_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(dep_station_layer, GTextAlignmentLeft);
  
  //Create departure station time field. Once we have the measurements of the departure station name layer it's easy:
  dep_time_layer = text_layer_create(GRect(dep_station_bounds.size.w+1, 0, in_trip_layer_bounds.size.w-dep_station_bounds.size.w, dep_station_bounds.size.h));
  add_child_layer(in_trip_layer, text_layer_get_layer(dep_time_layer));
  init_text_layer(dep_time_layer);
  text_layer_set_font(dep_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(dep_time_layer, GTextAlignmentRight);
  
  //Create arrival station name field
  arr_station_layer = text_layer_create(GRect(0, dep_station_bounds.size.h+1, dep_station_bounds.size.w, dep_station_bounds.size.h));
  add_child_layer(in_trip_layer, text_layer_get_layer(arr_station_layer));
  init_text_layer(arr_station_layer);
  text_layer_set_font(arr_station_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(arr_station_layer, GTextAlignmentLeft);
  
  //Create arrival station time field.
  arr_time_layer = text_layer_create(GRect(dep_station_bounds.size.w+1, dep_station_bounds.size.h+1, in_trip_layer_bounds.size.w-dep_station_bounds.size.w, dep_station_bounds.size.h));
  add_child_layer(in_trip_layer, text_layer_get_layer(arr_time_layer));
  init_text_layer(arr_time_layer);
  text_layer_set_font(arr_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(arr_time_layer, GTextAlignmentRight);
  
  //Create the countdown TextLayer and initialize it
  uint16_t aux = dep_station_bounds.size.h*2;
  countdown_layer = text_layer_create(GRect(0, aux+1, in_trip_layer_bounds.size.w, in_trip_layer_bounds.size.h - aux));
  add_child_layer(in_trip_layer, text_layer_get_layer(countdown_layer));
  init_text_layer(countdown_layer);
  text_layer_set_font(countdown_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(countdown_layer, GTextAlignmentCenter);
  
                  
  //Set train info
  text_layer_set_text(dep_station_layer, trainPlcatSqv1836.depStation);
  text_layer_set_text(arr_station_layer, trainPlcatSqv1836.arrStation);
  write_time_hm(dep_time_buffer, 8, trainPlcatSqv1836.depTime, FALSE);
  text_layer_set_text(dep_time_layer, dep_time_buffer);
  write_time_hm(arr_time_buffer, 8, trainPlcatSqv1836.arrTime, FALSE);
  text_layer_set_text(arr_time_layer, arr_time_buffer);
}

static void trip_window_unload(Window *window) {
  // Destroy layers
  destroy_all_layers();
}

static void handle_init(void) {
  update_time();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, second_tick_handler);
  
  //Set train trip data
  strcpy(trainPlcatSqv1836.depStation, "Pl. Catalunya"); //13 + NULL = 14
  trainPlcatSqv1836.depTime.hour = 18;
  trainPlcatSqv1836.depTime.min = 36;
  strcpy(trainPlcatSqv1836.arrStation, "Sant Quirze"); //11 + NULL = 12
  trainPlcatSqv1836.arrTime.hour = 19;
  trainPlcatSqv1836.arrTime.min = 14;
  
  trip_window = window_create();
  window_set_window_handlers(trip_window, (WindowHandlers) {
    .load = trip_window_load,
    .unload = trip_window_unload
  });

  window_stack_push(trip_window, true);
}

static void handle_deinit(void) {
  //scheduleWakeup();
  window_destroy(trip_window);
}

/*** Main ***/
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
