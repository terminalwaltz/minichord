#ifndef DEBOUNCER_H
#define DEBOUNCER_H

#include "Arduino.h"

class debouncer {
  public:
    debouncer();
    void set(bool set_value);
    uint8_t read_transition();
    bool read_value();
    void clear_transition(); // Declare new method
  private:
    u_int16_t debounce_value = 10000; // 10ms
    bool flag = false; // Flag for state change
    elapsedMicros last_update = 0;
    bool value = false;
};

#endif