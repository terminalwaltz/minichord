#include "potentiometer.h"
#include <Audio.h>

potentiometer::potentiometer(int pot_pin) {
    this->pot_pin = pot_pin;
}

void potentiometer::setup(int main_adress, float main_range_lower, int alternate_adress, float alternate_range_lower, int16_t *current_sysex_parameters_pointer, int alternate_initial_value, const std::function<void(int, int)> & apply_audio_parameter, int alternate_storage_adress) {
    this->main_adress = main_adress;
    this->main_range_lower = main_range_lower;
    this->main_range_upper = 100.0; // Default, set via sysex_handler.h
    this->alternate_adress = alternate_adress;
    this->alternate_range_lower = alternate_range_lower;
    this->alternate_range_upper = 100.0; // Default, set via sysex_handler.h
    this->current_sysex_parameters_pointer = current_sysex_parameters_pointer;
    this->alternate_value = alternate_initial_value;
    this->apply_audio_parameter = apply_audio_parameter;
    this->alternate_storage_adress = alternate_storage_adress;
    // Debug setup values
    if (current_sysex_parameters_pointer && alternate_adress == 34) {
    }
}

void potentiometer::set_main(int adress) {
    this->main_adress = adress;
}

void potentiometer::set_main_range_lower(float range) {
    this->main_range_lower = range;
}

void potentiometer::set_main_range_upper(float range) {
    this->main_range_upper = range;
}

void potentiometer::set_alternate(int adress) {
    this->alternate_adress = adress;
}

void potentiometer::set_alternate_range_lower(float range) {
    this->alternate_range_lower = range;
}

void potentiometer::set_alternate_range_upper(float range) {
    this->alternate_range_upper = range;
}

void potentiometer::set_alternate_default(int alternate_initial_value) {
    this->alternate_value = alternate_initial_value;
}

// Applies hysteresis to a quantised integer output. Without this, a pot resting
// exactly on a step boundary flips between two adjacent values on noise alone.
// A change is accepted only once the reading has travelled a fraction of one
// step away from where the current value was latched.
int16_t potentiometer::apply_integer_hysteresis(int16_t proposed, int16_t scaled_min, int16_t scaled_max, int target_adress) {
    if (target_adress != last_integer_adress) { // new target, adopt without resistance
        last_integer_adress = target_adress;
        last_integer_output = proposed;
        last_integer_reading = potentiometer_smoothed_value;
        return proposed;
    }
    if (proposed == last_integer_output) {
        return proposed;
    }
    int16_t steps = scaled_max - scaled_min;
    if (steps <= 0) { // single-valued range, nothing to debounce
        last_integer_output = proposed;
        last_integer_reading = potentiometer_smoothed_value;
        return proposed;
    }
    float step_width = (float)(1024 - 2 * dead_zone) / (float)(steps + 1);
    if (abs(potentiometer_smoothed_value - last_integer_reading) < step_width * hysteresis_fraction) {
        return last_integer_output; // too close to the boundary, hold
    }
    last_integer_output = proposed;
    last_integer_reading = potentiometer_smoothed_value;
    return proposed;
}

void potentiometer::force_update() {
    potentiometer_smoothed_value = 0;
    potentiometer_old_value = 1024; // Trigger update of main parameter
    update_parameter(false); // Apply main parameter

    // Find alternate parameter in lookup table
    bool found = false;
    bool is_integer = false;
    bool is_percent = false;
    int16_t min_value = 0;
    int16_t max_value = 100; // Default for float
    for (size_t i = 0; i < sizeof(parameter_lookup) / sizeof(parameter_lookup[0]); i++) {
        if (parameter_lookup[i].sysex_adress == alternate_adress) {
            found = true;
            is_integer = parameter_lookup[i].is_integer;
            is_percent = parameter_lookup[i].is_percent;
            min_value = parameter_lookup[i].min_value;
            max_value = parameter_lookup[i].max_value;
            break;
        }
    }

    uint16_t output_value;
    if (found && is_integer) {
        if (is_percent) {
            // Treat percent controls as float despite "int" data type
            uint16_t min_val = max(0, alternate_range_lower) * 100;
            uint16_t max_val = alternate_range_upper * 100;
            output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
        } else {
            // Integer parameter
            int16_t base_value = current_sysex_parameters_pointer ? current_sysex_parameters_pointer[alternate_adress] : min_value;
            base_value = constrain(base_value, min_value, max_value);
            int16_t range_size = max_value - min_value + 1;
            float range_fraction = alternate_range_lower / 100.0f;
            int16_t scaled_min, scaled_max;
            if (range_fraction >= 0.99f || range_fraction == 0.0f) { // Full range for 0 or near 100%
                scaled_min = min_value;
                scaled_max = max_value;
            } else {
                int16_t scaled_range = round(range_size * range_fraction);
                scaled_range = max(2, scaled_range); // Ensure at least two values
                int16_t half_range = (scaled_range + 1) / 2;
                scaled_min = max(min_value, base_value - half_range);
                scaled_max = min(max_value, base_value + half_range);
                if (scaled_min == scaled_max) {
                    if (scaled_max < max_value) scaled_max++;
                    else if (scaled_min > min_value) scaled_min--;
                }
            }
            output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, scaled_min, scaled_max), min_value, max_value);
            last_integer_adress = alternate_adress; // prime the hysteresis latch
            last_integer_output = output_value;
            last_integer_reading = potentiometer_smoothed_value;
        }
    } else {
        // Float parameter
        uint16_t min_val = max(0, alternate_range_lower) * 100;
        uint16_t max_val = alternate_range_upper * 100;
        output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
    }
    if (output_value != old_alternate_output) {
        apply_audio_parameter(alternate_adress, output_value);
        old_alternate_output = output_value;
    }
}

bool potentiometer::update_parameter(bool alternate_flag) {
    uint16_t current_reading = 1024 - analogRead(pot_pin);
    potentiometer_smoothed_value = (10 * potentiometer_smoothed_value + current_reading) / 11;
    bool trigger_change = false;
    if (!alternate_flag) {
        if (current_sysex_parameters_pointer && current_sysex_parameters_pointer[main_adress] != old_main_adress_value) {
            trigger_change = true;
            old_main_adress_value = current_sysex_parameters_pointer[main_adress];
        }
    }

    if (abs(potentiometer_old_value - potentiometer_smoothed_value) > threshold || trigger_change) {
        trigger_change = false;
        this->potentiometer_old_value = potentiometer_smoothed_value;
        uint16_t target_adress = alternate_flag ? alternate_adress : main_adress;
        float range = alternate_flag ? alternate_range_lower : main_range_lower;
        float range_upper = alternate_flag ? alternate_range_upper : main_range_upper;

        // Find parameter in lookup table
        bool found = false;
        bool is_integer = false;
        bool is_percent = false;
        int16_t min_value = 0;
        int16_t max_value = 100; // Default for float
        for (size_t i = 0; i < sizeof(parameter_lookup) / sizeof(parameter_lookup[0]); i++) {
            if (parameter_lookup[i].sysex_adress == target_adress) {
                found = true;
                is_integer = parameter_lookup[i].is_integer;
                is_percent = parameter_lookup[i].is_percent;
                min_value = parameter_lookup[i].min_value;
                max_value = parameter_lookup[i].max_value;
                break;
            }
        }

        uint16_t output_value;
        if (found && is_integer) {
            if (is_percent) {
                // Treat percent controls as float despite "int" data type
                uint16_t min_val = max(0, range) * 100;
                uint16_t max_val = range_upper * 100;
                output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
            } else {
                // Integer parameter
                int16_t base_value = current_sysex_parameters_pointer ? current_sysex_parameters_pointer[target_adress] : min_value;
                base_value = constrain(base_value, min_value, max_value);
                int16_t range_size = max_value - min_value + 1;
                float range_fraction = range / 100.0f;
                int16_t scaled_min, scaled_max;
                if (range_fraction >= 0.99f || range_fraction == 0.0f) { // Full range for 0 or near 100%
                    scaled_min = min_value;
                    scaled_max = max_value;
                } else {
                    int16_t scaled_range = round(range_size * range_fraction);
                    scaled_range = max(2, scaled_range); // Ensure at least two values
                    int16_t half_range = (scaled_range + 1) / 2;
                    scaled_min = max(min_value, base_value - half_range);
                    scaled_max = min(max_value, base_value + half_range);
                    if (scaled_min == scaled_max) {
                        if (scaled_max < max_value) scaled_max++;
                        else if (scaled_min > min_value) scaled_min--;
                    }
                }
                output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, scaled_min, scaled_max), min_value, max_value);
                output_value = apply_integer_hysteresis(output_value, scaled_min, scaled_max, target_adress);
            }
        } else {
            // Float parameter
            uint16_t min_val = max(0, range) * 100;
            uint16_t max_val = range_upper * 100;
            output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
        }

        if (alternate_flag) {
            if (output_value != old_alternate_output) {
                apply_audio_parameter(alternate_adress, output_value);
                this->alternate_value = potentiometer_smoothed_value;
                if (current_sysex_parameters_pointer) {
                    current_sysex_parameters_pointer[alternate_storage_adress] = potentiometer_smoothed_value;
                }
                old_alternate_output = output_value;
            }
            return true;
        } else {
            if (output_value != old_main_output) {
                apply_audio_parameter(main_adress, output_value);
                old_main_output = output_value;
            }
            return true;
        }
    }
    return false;
}