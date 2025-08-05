#include "debouncer.h"

debouncer::debouncer() : debounce_value(10000), flag(false), last_update(0), value(false) {}

void debouncer::set(bool set_value) {
    if (value != set_value) {
        flag = true;
        last_update = 0; // Reset timer for debounce
        value = set_value; // Update debounced state
    }
}

uint8_t debouncer::read_transition() {
    if (flag && last_update > debounce_value) {
        flag = false; // Clear transition flag
        return value ? 2 : 1; // 2 for press, 1 for release
    }
    return 0; // No transition
}

bool debouncer::read_value() {
    return value; // Return current debounced state
}