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
        // Integer parameter: map stored alternate_value to min_value–max_value
        output_value = constrain(map(alternate_value, dead_zone, 1024 - dead_zone, min_value, max_value), min_value, max_value);
        Serial.print("force_update integer: adress=");
        Serial.print(alternate_adress);
        Serial.print(", alternate_value=");
        Serial.print(alternate_value);
        Serial.print(", min=");
        Serial.print(min_value);
        Serial.print(", max=");
        Serial.print(max_value);
        Serial.print(", output=");
        Serial.println(output_value);
    } else {
        // Float parameter: map to lookup table range, scaled by 100 for sysex_handler.h
        int16_t base_value = current_sysex_parameters_pointer[alternate_adress];
        base_value = constrain(base_value, min_value, max_value);
        uint16_t min_val, max_val;
        if (base_value == 0) {
            // Fallback to lookup table range, scaled by 100
            min_val = min_value * 100;
            max_val = max_value * 100;
        } else {
            // Scale around base_value * 100, cap to avoid overflow
            min_val = max(0, (int32_t)(base_value * 100 * (1.0f - alternate_range / 100.0f)));
            max_val = min(65535, (int32_t)(base_value * 100 * (1.0f + alternate_range / 100.0f)));
        }
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
                // Integer parameter: map to min_value–max_value
                output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_value, max_value), min_value, max_value);
                Serial.print("update_parameter main integer: adress=");
                Serial.print(main_adress);
                Serial.print(", smoothed=");
                Serial.print(potentiometer_smoothed_value);
                Serial.print(", min=");
                Serial.print(min_value);
                Serial.print(", max=");
                Serial.print(max_value);
                Serial.print(", output=");
                Serial.println(output_value);
            } else {
                // Float parameter: map to lookup table range, scaled by 100 for sysex_handler.h
                int16_t base_value = current_sysex_parameters_pointer[main_adress];
                base_value = constrain(base_value, min_value, max_value);
                uint16_t min_val, max_val;
                if (base_value == 0) {
                    // Fallback to lookup table range, scaled by 100
                    min_val = min_value * 100;
                    max_val = max_value * 100;
                } else {
                    // Scale around base_value * 100, cap to avoid overflow
                    min_val = max(0, (int32_t)(base_value * 100 * (1.0f - range / 100.0f)));
                    max_val = min(65535, (int32_t)(base_value * 100 * (1.0f + range / 100.0f)));
                }
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
                // Integer parameter: map to min_value–max_value
                output_value = constrain(map(potentiometer_smoothed_value, dead_zone, 1024 - dead_zone, min_value, max_value), min_value, max_value);
                Serial.print("update_parameter alternate integer: adress=");
                Serial.print(alternate_adress);
                Serial.print(", smoothed=");
                Serial.print(potentiometer_smoothed_value);
                Serial.print(", min=");
                Serial.print(min_value);
                Serial.print(", max=");
                Serial.print(max_value);
                Serial.print(", output=");
                Serial.println(output_value);
            } else {
                // Float parameter: map to lookup table range, scaled by 100 for sysex_handler.h
                int16_t base_value = current_sysex_parameters_pointer[alternate_adress];
                base_value = constrain(base_value, min_value, max_value);
                uint16_t min_val, max_val;
                if (base_value == 0) {
                    // Fallback to lookup table range, scaled by 100
                    min_val = min_value * 100;
                    max_val = max_value * 100;
                } else {
                    // Scale around base_value * 100, cap to avoid overflow
                    min_val = max(0, (int32_t)(base_value * 100 * (1.0f - range / 100.0f)));
                    max_val = min(65535, (int32_t)(base_value * 100 * (1.0f + range / 100.0f)));
                }
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