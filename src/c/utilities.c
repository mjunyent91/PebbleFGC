#include "utilities.h"

uint16_t time_hm_diff(struct time_hm t1, struct time_hm t2) {
  uint16_t minutes;
  
  //Get minute difference
  if (t1.min >= t2.min) {
    minutes = t1.min - t2.min;
  } else {
    minutes = 60 + t1.min - t2.min;
    t1.hour--; //one hour was used for the minute subtraction
  }
  
  //Get hour difference
  if (t1.hour >= t2.hour) {
    minutes += (t1.hour - t2.hour)*60;
  } else {
    minutes += (24 + t1.hour - t2.hour)*60; //we don't allow negative numbers, so we think t2 is actually tomorrow
  }
  
  return minutes;
}

struct time_ms time_diff(struct time_hm t1, struct tm* t2) {
  struct time_ms res = {255, 255};
  struct time_hm t2hm = {t2->tm_hour, t2->tm_min};
  uint16_t minutes = time_hm_diff(t1, t2hm);
  //Check for overflow, otherwise return 255,255
  if (minutes < 256) { 
    res.min = minutes - 1; //subtract 1 since we have to account for the seconds (next line)
    res.sec = 59 - t2->tm_sec;
  }
  return res;
}

bool write_tm(char* buffer, int max_size, struct tm* time) {
  // Write the current hours and minutes into a buffer
  return strftime(buffer, max_size, clock_is_24h_style() ? "%H:%M" : "%I:%M", time) > 0; //check if it returns a positive value
}

bool write_time_ms(char* buffer, int max_size, struct time_ms time, bool negative) {
  int aux;
  if (time.sec < 10) {
    if (negative) {
      aux = snprintf(buffer, max_size, "-%u:0%u", time.min, time.sec); //add 0 for displaying minutes like ":mm"
    } else {
      aux = snprintf(buffer, max_size, "%u:0%u", time.min, time.sec);
    }
  } else {
    if (negative) {
      aux = snprintf(buffer, max_size, "-%u:%u", time.min, time.sec);
    } else {
      aux = snprintf(buffer, max_size, "%u:%u", time.min, time.sec);
    }
  }
  if (aux > 0 && aux < max_size) //the string has been written correctly
    return TRUE;
  else
    return FALSE;
}

bool write_time_hm(char* buffer, int max_size, struct time_hm time, bool negative) {
  struct time_ms t = {time.hour, time.min};
  return write_time_ms(buffer, max_size, t, negative);
}