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
    Serial.print("setup: main_adress=");
    Serial.print(main_adress);
    Serial.print(", main_range_lower=");
    Serial.print(main_range_lower);
    Serial.print(", main_range_upper=");
    Serial.print(main_range_upper);
    Serial.print(", alternate_adress=");
    Serial.print(alternate_adress);
    Serial.print(", alternate_range_lower=");
    Serial.print(alternate_range_lower);
    Serial.print(", alternate_range_upper=");
    Serial.println(alternate_range_upper);
    if (current_sysex_parameters_pointer && alternate_adress == 34) {
        Serial.print("setup: base_value for 34=");
        Serial.println(current_sysex_parameters_pointer[34]);
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

void potentiometer::force_update() {
    potentiometer_smoothed_value = 0;
    potentiometer_old_value = 1024; // Trigger update of main parameter
    update_parameter(false); // Apply main parameter

    // Find alternate parameter in lookup table
    bool found = false;
    bool is_integer = false;
    int16_t min_value = 0;
    int16_t max_value = 100; // Default for float
    for (size_t i = 0; i < sizeof(parameter_lookup) / sizeof(parameter_lookup[0]); i++) {
        if (parameter_lookup[i].sysex_adress == alternate_adress) {
            found = true;
            is_integer = parameter_lookup[i].is_integer;
            min_value = parameter_lookup[i].min_value;
            max_value = parameter_lookup[i].max_value;
            break;
        }
    }
    Serial.print("force_update: adress=");
    Serial.print(alternate_adress);
    Serial.print(", is_integer=");
    Serial.print(is_integer);
    Serial.print(", min_value=");
    Serial.print(min_value);
    Serial.print(", max_value=");
    Serial.println(max_value);

    uint16_t output_value;
    if (found && is_integer) {
        // Check if parameter is a percent control (lower/upper bounds)
        bool is_percent_control = (alternate_adress == 11 || alternate_adress == 13 || alternate_adress == 15 || alternate_adress == 17 ||
                                  alternate_adress == 200 || alternate_adress == 201 || alternate_adress == 202 || alternate_adress == 203);
        if (is_percent_control) {
            // Treat percent controls as float despite "int" data type
            uint16_t min_val = max(0, alternate_range_lower) * 100;
            uint16_t max_val = alternate_range_upper * 100;
            output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
            Serial.print("force_update percent control: adress=");
            Serial.print(alternate_adress);
            Serial.print(", smoothed=");
            Serial.print(potentiometer_smoothed_value);
            Serial.print(", min=");
            Serial.print(min_val);
            Serial.print(", max=");
            Serial.print(max_val);
            Serial.print(", output=");
            Serial.println(output_value);
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
            Serial.print("force_update integer: adress=");
            Serial.print(alternate_adress);
            Serial.print(", base=");
            Serial.print(base_value);
            Serial.print(", smoothed=");
            Serial.print(potentiometer_smoothed_value);
            Serial.print(", scaled_min=");
            Serial.print(scaled_min);
            Serial.print(", scaled_max=");
            Serial.print(scaled_max);
            Serial.print(", output=");
            Serial.println(output_value);
        }
    } else {
        // Float parameter
        uint16_t min_val = max(0, alternate_range_lower) * 100;
        uint16_t max_val = alternate_range_upper * 100;
        output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
        Serial.print("force_update float: adress=");
        Serial.print(alternate_adress);
        Serial.print(", smoothed=");
        Serial.print(potentiometer_smoothed_value);
        Serial.print(", min=");
        Serial.print(min_val);
        Serial.print(", max=");
        Serial.print(max_val);
        Serial.print(", output=");
        Serial.println(output_value);
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
        int16_t min_value = 0;
        int16_t max_value = 100; // Default for float
        for (size_t i = 0; i < sizeof(parameter_lookup) / sizeof(parameter_lookup[0]); i++) {
            if (parameter_lookup[i].sysex_adress == target_adress) {
                found = true;
                is_integer = parameter_lookup[i].is_integer;
                min_value = parameter_lookup[i].min_value;
                max_value = parameter_lookup[i].max_value;
                break;
            }
        }
        Serial.print("update_parameter: adress=");
        Serial.print(target_adress);
        Serial.print(", is_integer=");
        Serial.print(is_integer);
        Serial.print(", min_value=");
        Serial.print(min_value);
        Serial.print(", max_value=");
        Serial.println(max_value);

        uint16_t output_value;
        if (found && is_integer) {
            // Check if parameter is a percent control (lower/upper bounds)
            bool is_percent_control = (target_adress == 11 || target_adress == 13 || target_adress == 15 || target_adress == 17 ||
                                      target_adress == 200 || target_adress == 201 || target_adress == 202 || target_adress == 203);
            if (is_percent_control) {
                // Treat percent controls as float despite "int" data type
                uint16_t min_val = max(0, range) * 100;
                uint16_t max_val = range_upper * 100;
                output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
                Serial.print("update_parameter percent control: adress=");
                Serial.print(target_adress);
                Serial.print(", smoothed=");
                Serial.print(potentiometer_smoothed_value);
                Serial.print(", min=");
                Serial.print(min_val);
                Serial.print(", max=");
                Serial.print(max_val);
                Serial.print(", output=");
                Serial.println(output_value);
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
                Serial.print("update_parameter integer: adress=");
                Serial.print(target_adress);
                Serial.print(", base=");
                Serial.print(base_value);
                Serial.print(", smoothed=");
                Serial.print(potentiometer_smoothed_value);
                Serial.print(", scaled_min=");
                Serial.print(scaled_min);
                Serial.print(", scaled_max=");
                Serial.print(scaled_max);
                Serial.print(", output=");
                Serial.println(output_value);
            }
        } else {
            // Float parameter
            uint16_t min_val = max(0, range) * 100;
            uint16_t max_val = range_upper * 100;
            output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
            Serial.print("update_parameter float: adress=");
            Serial.print(target_adress);
            Serial.print(", smoothed=");
            Serial.print(potentiometer_smoothed_value);
            Serial.print(", min=");
            Serial.print(min_val);
            Serial.print(", max=");
            Serial.print(max_val);
            Serial.print(", output=");
            Serial.println(output_value);
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