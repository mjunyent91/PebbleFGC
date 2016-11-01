#pragma once
#include <pebble.h>

#define TRUE 1
#define FALSE 0

/*** Type definitions ***/
struct time_hm{ //2 bytes
  uint8_t hour;
  uint8_t min;
};

struct time_ms{ //2 bytes
  uint8_t min;
  uint8_t sec;
};

struct train_trip{ //32 bytes
  char depStation[14];
  struct time_hm depTime;
  char arrStation[14];
  struct time_hm arrTime;
};

bool minutes_diff(struct time_hm t1, struct time_hm t2, uint16_t* result);
bool tm_minutes_diff(struct time_hm t1, struct tm* t2, uint16_t* result);
bool time_diff(struct time_hm t1, struct tm* t2, struct time_ms* result);
bool write_tm(char* buffer, int max_size, struct tm* time);
bool write_time_ms(char* buffer, int max_size, struct time_ms time, bool negative);
bool write_time_hm(char* buffer, int max_size, struct time_hm time, bool negative);
bool scheduleWakeup(struct time_hm scheduleTime, uint8_t minBefore);