#include "debouncer.h"

debouncer::debouncer() {
}

void debouncer::set(bool set_value) {
    if (value != set_value) {
        flag = true;
        last_update = 0;
        value = set_value;
    }
}

uint8_t debouncer::read_transition() {
    if (flag == true && last_update > debounce_value) {
        flag = false;
        if (value == true) {
            return 2; // Press transition
        } else {
            return 1; // Release transition
        }
    }
    return 0;
}

bool debouncer::read_value() {
    return value;
}

void debouncer::clear_transition() {
    flag = false; // Clear transition flag to prevent read_transition() from firing
}