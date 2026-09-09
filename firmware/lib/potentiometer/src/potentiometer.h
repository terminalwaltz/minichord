#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include "Arduino.h"
#include <functional>
#include "parameter_lookup.h"

class potentiometer {
private:
    std::function<void(int, int)> apply_audio_parameter;
public:
    potentiometer(int pot_pin);
    void setup(int main_adress, float main_range_lower, int alternate_adress, float alternate_range_lower, int16_t *current_sysex_parameters_pointer, int alternate_initial_value, const std::function<void(int, int)> & apply_audio_parameter, int alternate_storage_adress);
    void set_main(int adress);
    void set_main_range_lower(float range);
    void set_main_range_upper(float range);
    void set_alternate(int adress);
    void set_alternate_range_lower(float range);
    void set_alternate_range_upper(float range);
    void set_alternate_default(int alternate_initial_value);
    void force_update();
    bool update_parameter(bool alternate_flag); // the flag tells us whether the modifier button was pushed
private:
    // Applies hysteresis to a quantised integer output so that a pot resting on a
    // step boundary does not flip back and forth on noise. Returns the value to emit.
    int16_t apply_integer_hysteresis(int16_t proposed, int16_t scaled_min, int16_t scaled_max, int target_adress);
    // Memory variables
    int potentiometer_smoothed_value = 0; // smoothing of the analog reading
    int potentiometer_old_value = 0; // old value to detect if we meet threshold
    int alternate_value = 0;
    
    uint16_t old_main_output = 0;
    uint16_t old_alternate_output = 0;

    // Potentiometer target values
    int main_adress;
    float main_range_lower;
    float main_range_upper; // Upper bound for main range
    int alternate_adress;
    float alternate_range_lower;
    float alternate_range_upper; // Upper bound for alternate range
    int alternate_storage_adress;
    int old_main_adress_value = 0;
    // Potentiometer hardware parameters
    int pot_pin;
    int16_t dead_zone = 20;
    int16_t threshold = 20;
    // Hysteresis state for discrete (integer) targets
    int16_t last_integer_output = 0;   // last value emitted for a discrete target
    int16_t last_integer_reading = 0;  // smoothed reading when that value was latched
    int last_integer_adress = -1;      // target the latch refers to
    float hysteresis_fraction = 0.6f;  // travel required, as a fraction of one step
    // To access initial value
    int16_t *current_sysex_parameters_pointer;
};

#endif
