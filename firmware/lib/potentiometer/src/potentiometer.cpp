#include "potentiometer.h"
#include <Audio.h>

potentiometer::potentiometer(int pot_pin){
    this->pot_pin = pot_pin;
}  

void potentiometer::setup(int main_adress,float main_range,int alternate_adress,float alternate_range, int16_t *current_sysex_parameters_pointer,int alternate_initial_value,const std::function<void(int, int)> & apply_audio_parameter, int alternate_storage_adress){
    this->main_adress = main_adress;
    this->main_range = main_range;
    this->alternate_adress = alternate_adress;
    this->alternate_range=alternate_range;
    this->current_sysex_parameters_pointer=current_sysex_parameters_pointer;
    this->alternate_value=alternate_initial_value;
    this->apply_audio_parameter=apply_audio_parameter;
    this->alternate_storage_adress=alternate_storage_adress;
}  
void potentiometer::set_main(int adress){
    this->main_adress = adress;
}
void potentiometer::set_main_range(float range){
    this->main_range = range;
}
void potentiometer::set_alternate(int adress){
    this->alternate_adress = adress;
}
void potentiometer::set_alternate_range(float range){
    this->alternate_range = range;
}
void potentiometer::set_alternate_default(int alternate_initial_value){
    this->alternate_value=alternate_initial_value;
}

void potentiometer::force_update(){
    potentiometer_smoothed_value=0;
    potentiometer_old_value=1024;// bit of a hack to trigger an update of the main parameter
    update_parameter(false); //apply to the main the current value of the potentiometer.
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
        // Integer parameter: apply percent range to limit the number of accessible values
        int16_t base_value = current_sysex_parameters_pointer[alternate_adress];
        base_value = constrain(base_value, min_value, max_value);
        int16_t range_size = max_value - min_value + 1; // Total number of integer values (e.g., 8 for 0–7)
        float range_fraction = alternate_range / 100.0f; // Percent range as a fraction (e.g., 100% = 1.0)
        
        int16_t scaled_min, scaled_max;
        if (range_fraction >= 1.0f) {
            // Full range: use min_value and max_value directly
            scaled_min = min_value;
            scaled_max = max_value;
        } else {
            // Partial range: calculate scaled range centered around base_value
            int16_t scaled_range = round(range_size * range_fraction); // Number of values in scaled range
            scaled_range = max(1, scaled_range); // Ensure at least one value
            int16_t half_range = scaled_range / 2;
            scaled_min = max(min_value, base_value - half_range);
            scaled_max = min(max_value, base_value + half_range);
            // Ensure at least one value difference if scaled_min equals scaled_max
            if (scaled_min == scaled_max) {
                if (scaled_max < max_value) {
                    scaled_max++;
                } else if (scaled_min > min_value) {
                    scaled_min--;
                }
            }
        }
        
        // Map potentiometer value to the scaled integer range
        output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, scaled_min, scaled_max), min_value, max_value);
        Serial.print("update_parameter integer: adress=");
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
    } else {
        // Float parameter: map to lookup table range, scaled by 100 for sysex_handler.h
        int16_t base_value = current_sysex_parameters_pointer[alternate_adress];
        base_value = constrain(base_value, min_value, max_value);
        uint16_t min_val, max_val;
        float scaled_min, scaled_max;
            if (base_value == 0) {
                // Fallback: use full parameter range
                scaled_min = min_value;
                scaled_max = max_value;
            } else {
                // Scale around current value but clamp to valid range
                scaled_min = max((float)min_value, base_value * (1.0f - alternate_range / 100.0f));
                scaled_max = min((float)max_value, base_value * (1.0f + alternate_range / 100.0f));
            }
        min_val = scaled_min * 100;
        max_val = scaled_max * 100;
        output_value = constrain(map(alternate_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
        Serial.print("force_update float: adress=");
        Serial.print(alternate_adress);
        Serial.print(", base=");
        Serial.print(base_value);
        Serial.print(", alternate_value=");
        Serial.print(alternate_value);
        Serial.print(", min=");
        Serial.print(min_val);
        Serial.print(", max=");
        Serial.print(max_val);
        Serial.print(", output=");
        Serial.println(output_value);
    }
    apply_audio_parameter(alternate_adress, output_value); //apply the saved alternate value
}

bool potentiometer::update_parameter(bool alternate_flag){
    uint16_t current_reading = 1024 - analogRead(pot_pin);
    potentiometer_smoothed_value = (10 * potentiometer_smoothed_value + current_reading) / 11;
    // Check if parameter was modified externally
    bool trigger_change = false;
    if (!alternate_flag) {
        if (current_sysex_parameters_pointer[main_adress] != old_main_adress_value) {
            trigger_change = true;
            old_main_adress_value = current_sysex_parameters_pointer[main_adress];
        }
    }
    if (abs(potentiometer_old_value - potentiometer_smoothed_value) > threshold || trigger_change) {
        trigger_change = false;
        this->potentiometer_old_value = potentiometer_smoothed_value;
        uint16_t target_adress = alternate_flag ? alternate_adress : main_adress;
        float range = alternate_flag ? alternate_range : main_range;
        // Find parameter in lookup table
        bool found = false;
        bool is_integer = false;
        int16_t min_value = 0;
        int16_t max_value = 100; // Default for float parameters
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
        if (!alternate_flag) {
            if (found && is_integer) {
                // Integer parameter: apply percent range to limit the number of accessible values
                int16_t base_value = current_sysex_parameters_pointer[target_adress];
                base_value = constrain(base_value, min_value, max_value);
                int16_t range_size = max_value - min_value + 1; // Total number of integer values (e.g., 8 for 0–7)
                float range_fraction = range / 100.0f; // Percent range as a fraction (e.g., 100% = 1.0)
                
                int16_t scaled_min, scaled_max;
                if (range_fraction >= 1.0f) {
                    // Full range: use min_value and max_value directly
                    scaled_min = min_value;
                    scaled_max = max_value;
                } else {
                    // Partial range: calculate scaled range centered around base_value
                    int16_t scaled_range = round(range_size * range_fraction); // Number of values in scaled range
                    scaled_range = max(1, scaled_range); // Ensure at least one value
                    int16_t half_range = scaled_range / 2;
                    scaled_min = max(min_value, base_value - half_range);
                    scaled_max = min(max_value, base_value + half_range);
                    // Ensure at least one value difference if scaled_min equals scaled_max
                    if (scaled_min == scaled_max) {
                        if (scaled_max < max_value) {
                            scaled_max++;
                        } else if (scaled_min > min_value) {
                            scaled_min--;
                        }
                    }
                }
                
                // Map potentiometer value to the scaled integer range
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
            } else {
                // Float parameter: map to lookup table range, scaled by 100 for sysex_handler.h
                int16_t base_value = current_sysex_parameters_pointer[main_adress];
                base_value = constrain(base_value, min_value, max_value);
                uint16_t min_val, max_val;
                float scaled_min, scaled_max;
                    if (base_value == 0) {
                        // Fallback: use full parameter range
                        scaled_min = min_value;
                        scaled_max = max_value;
                    } else {
                        // Scale around current value but clamp to valid range
                        scaled_min = max((float)min_value, base_value * (1.0f - range / 100.0f));
                        scaled_max = min((float)max_value, base_value * (1.0f + range / 100.0f));
                    }
                min_val = scaled_min * 100;
                max_val = scaled_max * 100;
                output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
                Serial.print("update_parameter main float: adress=");
                Serial.print(main_adress);
                Serial.print(", base=");
                Serial.print(base_value);
                Serial.print(", smoothed=");
                Serial.print(potentiometer_smoothed_value);
                Serial.print(", min=");
                Serial.print(min_val);
                Serial.print(", max=");
                Serial.print(max_val);
                Serial.print(", output=");
                Serial.println(output_value);
            }
            apply_audio_parameter(main_adress, output_value); //note: applied but not saved
            return true;
        } else {
            if (found && is_integer) {
                // Integer parameter: apply percent range to limit the number of accessible values
                int16_t base_value = current_sysex_parameters_pointer[target_adress];
                base_value = constrain(base_value, min_value, max_value);
                int16_t range_size = max_value - min_value + 1; // Total number of integer values (e.g., 8 for 0–7)
                float range_fraction = range / 100.0f; // Percent range as a fraction (e.g., 100% = 1.0)
                
                int16_t scaled_min, scaled_max;
                if (range_fraction >= 1.0f) {
                    // Full range: use min_value and max_value directly
                    scaled_min = min_value;
                    scaled_max = max_value;
                } else {
                    // Partial range: calculate scaled range centered around base_value
                    int16_t scaled_range = round(range_size * range_fraction); // Number of values in scaled range
                    scaled_range = max(1, scaled_range); // Ensure at least one value
                    int16_t half_range = scaled_range / 2;
                    scaled_min = max(min_value, base_value - half_range);
                    scaled_max = min(max_value, base_value + half_range);
                    // Ensure at least one value difference if scaled_min equals scaled_max
                    if (scaled_min == scaled_max) {
                        if (scaled_max < max_value) {
                            scaled_max++;
                        } else if (scaled_min > min_value) {
                            scaled_min--;
                        }
                    }
                }
                
                // Map potentiometer value to the scaled integer range
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
            } else {
                // Float parameter: map to lookup table range, scaled by 100 for sysex_handler.h
                int16_t base_value = current_sysex_parameters_pointer[alternate_adress];
                base_value = constrain(base_value, min_value, max_value);
                uint16_t min_val, max_val;
                float scaled_min, scaled_max;
                    if (base_value == 0) {
                        // Fallback: use full parameter range
                        scaled_min = min_value;
                        scaled_max = max_value;
                    } else {
                        // Scale around current value but clamp to valid range
                        scaled_min = max((float)min_value, base_value * (1.0f - range / 100.0f));
                        scaled_max = min((float)max_value, base_value * (1.0f + range / 100.0f));
                    }
                min_val = scaled_min * 100;
                max_val = scaled_max * 100;
                output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_val, max_val), min_val, max_val);
                Serial.print("update_parameter alternate float: adress=");
                Serial.print(alternate_adress);
                Serial.print(", base=");
                Serial.print(base_value);
                Serial.print(", smoothed=");
                Serial.print(potentiometer_smoothed_value);
                Serial.print(", min=");
                Serial.print(min_val);
                Serial.print(", max=");
                Serial.print(max_val);
                Serial.print(", output=");
                Serial.println(output_value);
            }
            apply_audio_parameter(alternate_adress, output_value);
            this->alternate_value = potentiometer_smoothed_value;
            current_sysex_parameters_pointer[alternate_storage_adress] = potentiometer_smoothed_value; // saving the position
            return true;
        }
    }
    return false;
}
