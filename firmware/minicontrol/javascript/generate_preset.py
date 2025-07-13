import random
import base64
import pyperclip

# Fixed values for indices 0-9 (from provided preset)
fixed_values = {
    0: 0,
    1: 0,
    2: 50,    # Harp global gain (float, 0-100)
    3: 50,    # Chord global gain (float, 0-100)
    4: 512,   # Chord alternate value (int, 0-1024)
    5: 512,   # Harp alternate value (int, 0-1024)
    6: 512,   # Mod alternate value (int, 0-1024)
    7: 0,     # Firmware revision (float, 0-10)
    8: 0,
    9: 0
}

# Parameter ranges from parameters.json (adjusted for scale)
parameter_ranges = {
    10: {"min": 40, "max": 219, "type": "int"},    # Chord alternate control
    11: {"min": 0, "max": 100, "type": "int"},     # Chord alternate range
    12: {"min": 40, "max": 219, "type": "int"},    # Harp alternate control
    13: {"min": 0, "max": 100, "type": "int"},     # Harp alternate percent range
    14: {"min": 40, "max": 219, "type": "int"},    # Mod main control
    15: {"min": 0, "max": 100, "type": "int"},     # Mod main percent range
    16: {"min": 40, "max": 219, "type": "int"},    # Mod alternate control
    17: {"min": 0, "max": 100, "type": "int"},     # Mod alternate percent range
    18: {"min": 0, "max": 6, "type": "int"},       # Chord frame shift
    20: {"min": 0, "max": 360, "type": "int"},     # Bank color
    21: {"min": 0, "max": 1, "type": "int"},       # Retrigger chords
    23: {"min": 0, "max": 2, "type": "int"},       # Slash level
    24: {"min": 0, "max": 1, "type": "float"},     # Reverb size
    25: {"min": 0, "max": 1, "type": "float"},     # Reverb high damping
    26: {"min": 0, "max": 1, "type": "float"},     # Reverb low damping
    27: {"min": 0, "max": 1, "type": "float"},     # Reverb low pass
    28: {"min": 0, "max": 1, "type": "float"},     # Reverb diffusion
    29: {"min": 0, "max": 1, "type": "float"},     # Pan
    30: {"min": 0, "max": 12, "type": "int"},      # Transpose
    31: {"min": 0, "max": 1, "type": "int"},       # Sharp function
    32: {"min": 0, "max": 1, "type": "float"},     # LED attenuation
    33: {"min": 0, "max": 1, "type": "int"},       # Barry Harris mode
    41: {"min": 0, "max": 100, "type": "float"},   # Harp oscillator amplitude
    42: {"min": 0, "max": 11, "type": "int"},      # Harp oscillator waveform
    43: {"min": 0, "max": 5000, "type": "int"},    # Harp envelope attack
    44: {"min": 0, "max": 5000, "type": "int"},    # Harp envelope hold
    45: {"min": 0, "max": 5000, "type": "int"},    # Harp envelope decay
    46: {"min": 0, "max": 1, "type": "float"},     # Harp envelope sustain
    47: {"min": 0, "max": 5000, "type": "int"},    # Harp envelope release
    48: {"min": 0, "max": 10, "type": "int"},      # Harp envelope retrigger release
    49: {"min": 0, "max": 2000, "type": "int"},    # Harp low pass filter base frequency
    50: {"min": 0, "max": 3, "type": "float"},     # Harp low pass filter keytrack value
    51: {"min": 0.7, "max": 5, "type": "float"},   # Harp low pass filter resonance
    52: {"min": 0, "max": 5000, "type": "int"},    # Harp low pass filter attack
    53: {"min": 0, "max": 5000, "type": "int"},    # Harp low pass filter hold
    54: {"min": 0, "max": 5000, "type": "int"},    # Harp low pass filter decay
    55: {"min": 0, "max": 1, "type": "float"},     # Harp low pass filter sustain
    56: {"min": 0, "max": 5000, "type": "int"},    # Harp low pass filter release
    57: {"min": 0, "max": 100, "type": "int"},     # Harp low pass filter retrigger release
    58: {"min": 0, "max": 5, "type": "float"},     # Harp low pass filter filter sensitivity
    59: {"min": 0, "max": 11, "type": "int"},      # Harp tremolo waveform
    60: {"min": 0, "max": 20, "type": "float"},    # Harp tremolo frequency
    61: {"min": 0, "max": 1, "type": "float"},     # Harp tremolo amplitude
    62: {"min": 0, "max": 11, "type": "int"},      # Harp vibrato waveform
    63: {"min": 0, "max": 20, "type": "float"},    # Harp vibrato frequency
    64: {"min": 0, "max": 1, "type": "float"},     # Harp vibrato amplitude
    65: {"min": 0, "max": 5000, "type": "int"},    # Harp vibrato attack
    66: {"min": 0, "max": 5000, "type": "int"},    # Harp vibrato hold
    67: {"min": 0, "max": 5000, "type": "int"},    # Harp vibrato decay
    68: {"min": 0, "max": 1, "type": "float"},     # Harp vibrato sustain
    69: {"min": 0, "max": 5000, "type": "int"},    # Harp vibrato release
    70: {"min": 0, "max": 100, "type": "int"},     # Harp vibrato retrigger release
    71: {"min": 0, "max": 2, "type": "float"},     # Harp vibrato pitch bend
    72: {"min": 0, "max": 5000, "type": "int"},    # Harp vibrato attack bend
    73: {"min": 0, "max": 5000, "type": "int"},    # Harp vibrato hold bend
    74: {"min": 0, "max": 5000, "type": "int"},    # Harp vibrato decay bend
    75: {"min": 0, "max": 5000, "type": "int"},    # Harp vibrato retrigger release bend
    76: {"min": 0, "max": 1, "type": "float"},     # Harp vibrato intensity
    77: {"min": 0, "max": 600, "type": "int"},     # Harp effects delay length
    78: {"min": 0, "max": 5000, "type": "int"},    # Harp effects delay filter frequency
    79: {"min": 0.7, "max": 5, "type": "float"},   # Harp effects delay filter resonance
    80: {"min": 0, "max": 1, "type": "float"},     # Harp effects delay lowpass
    81: {"min": 0, "max": 1, "type": "float"},     # Harp effects delay bandpass
    82: {"min": 0, "max": 1, "type": "float"},     # Harp effects delay highpass
    83: {"min": 0, "max": 1, "type": "float"},     # Harp effects dry mix
    84: {"min": 0, "max": 1, "type": "float"},     # Harp effects delay mix
    85: {"min": 0, "max": 1, "type": "float"},     # Harp effects reverb level
    86: {"min": 0, "max": 1, "type": "float"},     # Harp effects crunch level
    87: {"min": 0, "max": 2, "type": "int"},       # Harp effects crunch type
    88: {"min": 0, "max": 5000, "type": "int"},    # Harp output filter frequency
    89: {"min": 0.7, "max": 5, "type": "float"},   # Harp output filter resonance
    90: {"min": 0, "max": 1, "type": "float"},     # Harp output filter lowpass
    91: {"min": 0, "max": 1, "type": "float"},     # Harp output filter bandpass
    92: {"min": 0, "max": 1, "type": "float"},     # Harp output filter highpass
    93: {"min": 0, "max": 11, "type": "int"},      # Harp output filter LFO waveform
    94: {"min": 0, "max": 20, "type": "float"},    # Harp output filter LFO frequency
    95: {"min": 0, "max": 1, "type": "float"},     # Harp output filter LFO amplitude
    96: {"min": 0, "max": 5, "type": "float"},     # Harp output filter LFO sensitivity
    97: {"min": 0, "max": 2, "type": "float"},     # Harp output filter output amplifier
    99: {"min": 0, "max": 4, "type": "int"},       # Harp octave change
    100: {"min": 0, "max": 11, "type": "int"},     # Harp transient waveform
    101: {"min": 0, "max": 1, "type": "float"},    # Harp transient amplitude
    102: {"min": 0, "max": 5000, "type": "int"},   # Harp transient attack
    103: {"min": 0, "max": 5000, "type": "int"},   # Harp transient hold
    104: {"min": 0, "max": 5000, "type": "int"},   # Harp transient decay
    105: {"min": 0, "max": 24, "type": "int"},     # Harp transient note level
    120: {"min": 0, "max": 5, "type": "int"},      # Chord shuffling
    121: {"min": 0, "max": 1, "type": "float"},    # Chord oscillator amplitude 1
    122: {"min": 0, "max": 11, "type": "int"},     # Chord oscillator waveform 1
    123: {"min": 0.5, "max": 2, "type": "float"},  # Chord oscillator frequency multiplier 1
    124: {"min": 0, "max": 1, "type": "float"},    # Chord oscillator amplitude 2
    125: {"min": 0, "max": 11, "type": "int"},     # Chord oscillator waveform 2
    126: {"min": 0.5, "max": 2, "type": "float"},  # Chord oscillator frequency multiplier 2
    127: {"min": 0, "max": 1, "type": "float"},    # Chord oscillator amplitude 3
    128: {"min": 0, "max": 11, "type": "int"},     # Chord oscillator waveform 3
    129: {"min": 0.5, "max": 2, "type": "float"},  # Chord oscillator frequency multiplier 3
    130: {"min": 0, "max": 1, "type": "float"},    # Chord oscillator noise
    131: {"min": 0, "max": 1, "type": "float"},    # Chord oscillator first note
    132: {"min": 0, "max": 1, "type": "float"},    # Chord oscillator second note
    133: {"min": 0, "max": 1, "type": "float"},    # Chord oscillator third note
    134: {"min": 0, "max": 1, "type": "float"},    # Chord oscillator fourth note
    135: {"min": 0, "max": 100, "type": "int"},    # Chord oscillator inter-note delay
    136: {"min": 0, "max": 100, "type": "int"},    # Chord oscillator random note delay
    137: {"min": 0, "max": 5000, "type": "int"},   # Chord envelope attack
    138: {"min": 0, "max": 5000, "type": "int"},   # Chord envelope hold
    139: {"min": 0, "max": 5000, "type": "int"},   # Chord envelope decay
    140: {"min": 0, "max": 1, "type": "float"},    # Chord envelope sustain
    141: {"min": 0, "max": 5000, "type": "int"},   # Chord envelope release
    142: {"min": 0, "max": 100, "type": "int"},    # Chord envelope retrigger release
    143: {"min": 0, "max": 5000, "type": "int"},   # Chord low pass filter base frequency
    144: {"min": 0, "max": 1, "type": "float"},    # Chord low pass filter keytrack value
    145: {"min": 0.7, "max": 5, "type": "float"},  # Chord low pass filter resonance
    146: {"min": 0, "max": 5000, "type": "int"},   # Chord low pass filter attack
    147: {"min": 0, "max": 5000, "type": "int"},   # Chord low pass filter hold
    148: {"min": 0, "max": 5000, "type": "int"},   # Chord low pass filter decay
    149: {"min": 0, "max": 1, "type": "float"},    # Chord low pass filter sustain
    150: {"min": 0, "max": 5000, "type": "int"},   # Chord low pass filter release
    151: {"min": 0, "max": 100, "type": "int"},    # Chord low pass filter retrigger release
    152: {"min": 0, "max": 11, "type": "int"},     # Chord low pass filter LFO waveform
    153: {"min": 0, "max": 20, "type": "float"},   # Chord low pass filter LFO frequency
    154: {"min": 0, "max": 1, "type": "float"},    # Chord low pass filter LFO amplitude
    155: {"min": 0, "max": 5, "type": "float"},    # Chord low pass filter filter sensitivity
    156: {"min": 0, "max": 11, "type": "int"},     # Chord tremolo waveform
    157: {"min": 0, "max": 20, "type": "float"},   # Chord tremolo frequency
    158: {"min": 0, "max": 5, "type": "float"},    # Chord tremolo keytrack value
    159: {"min": 0, "max": 1, "type": "float"},    # Chord tremolo amplitude
    160: {"min": 0, "max": 11, "type": "int"},     # Chord vibrato waveform
    161: {"min": 0, "max": 20, "type": "float"},   # Chord vibrato frequency
    162: {"min": 0, "max": 1, "type": "float"},    # Chord vibrato keytrack value
    163: {"min": 0, "max": 1, "type": "float"},    # Chord vibrato amplitude
    164: {"min": 0, "max": 5000, "type": "int"},   # Chord vibrato attack
    165: {"min": 0, "max": 5000, "type": "int"},   # Chord vibrato hold
    166: {"min": 0, "max": 5000, "type": "int"},   # Chord vibrato decay
    167: {"min": 0, "max": 1, "type": "float"},    # Chord vibrato sustain
    168: {"min": 0, "max": 5000, "type": "int"},   # Chord vibrato release
    169: {"min": 0, "max": 100, "type": "int"},    # Chord vibrato retrigger release
    170: {"min": 0, "max": 2, "type": "float"},    # Chord vibrato pitch bend
    171: {"min": 0, "max": 5000, "type": "int"},   # Chord vibrato attack bend
    172: {"min": 0, "max": 5000, "type": "int"},   # Chord vibrato hold bend
    173: {"min": 0, "max": 5000, "type": "int"},   # Chord vibrato decay bend
    174: {"min": 0, "max": 100, "type": "int"},    # Chord vibrato retrigger release bend
    175: {"min": 0, "max": 1, "type": "float"},    # Chord vibrato intensity
    176: {"min": 0, "max": 600, "type": "int"},    # Chord effects delay length
    177: {"min": 0, "max": 5000, "type": "int"},   # Chord effects delay filter frequency
    178: {"min": 0.7, "max": 5, "type": "float"},  # Chord effects delay filter resonance
    179: {"min": 0, "max": 1, "type": "float"},    # Chord effects delay lowpass
    180: {"min": 0, "max": 1, "type": "float"},    # Chord effects delay bandpass
    181: {"min": 0, "max": 1, "type": "float"},    # Chord effects delay highpass
    182: {"min": 0, "max": 1, "type": "float"},    # Chord effects dry mix
    183: {"min": 0, "max": 1, "type": "float"},    # Chord effects delay mix
    184: {"min": 0, "max": 1, "type": "float"},    # Chord effects reverb level
    185: {"min": 0, "max": 1, "type": "float"},    # Chord effects crunch level
    186: {"min": 0, "max": 2, "type": "int"},      # Chord effects crunch type
    187: {"min": 30, "max": 300, "type": "int"},   # Chord rhythm default BPM
    188: {"min": 1, "max": 16, "type": "int"},     # Chord rhythm cycle length
    189: {"min": 1, "max": 8, "type": "int"},      # Chord rhythm measure update
    190: {"min": 0.5, "max": 1.5, "type": "float"},# Chord rhythm shuffle value
    191: {"min": 20, "max": 1000, "type": "int"},  # Chord rhythm note pushed duration
    192: {"min": 0, "max": 5000, "type": "int"},   # Chord output filter frequency
    193: {"min": 0.7, "max": 5, "type": "float"},  # Chord output filter resonance
    194: {"min": 0, "max": 1, "type": "float"},    # Chord output filter lowpass
    195: {"min": 0, "max": 1, "type": "float"},    # Chord output filter bandpass
    196: {"min": 0, "max": 1, "type": "float"},    # Chord output filter highpass
    197: {"min": 0, "max": 2, "type": "float"},    # Chord output filter output amplifier
    198: {"min": 0, "max": 4, "type": "int"},      # Chord octave change
    220: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 0
    221: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 1
    222: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 2
    223: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 3
    224: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 4
    225: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 5
    226: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 6
    227: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 7
    228: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 8
    229: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 9
    230: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 10
    231: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 11
    232: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 12
    233: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 13
    234: {"min": 0, "max": 128, "type": "int"},    # Rhythm pattern 14
    235: {"min": 0, "max": 128, "type": "int"}     # Rhythm pattern 15
}


def generate_random_preset():
    # Initialize preset array with 256 elements
    preset = [0] * 256
    
    # Set fixed values for indices 0-9
    for index, value in fixed_values.items():
        preset[index] = value
    
    # Define parameters to constrain for musicality and audibility
    oscillator_amplitudes = [41, 121, 124, 127]  # Harp and chord oscillator amplitudes
    output_amplifiers = [97, 197]  # Harp and chord output filter output amplifiers
    resonance_params = [51, 79, 89, 145, 178, 193]  # Resonance parameters
    envelope_times = [43, 44, 47, 52, 53, 54, 56, 65, 66, 67, 69, 72, 73, 74, 77, 78,
                      137, 138, 141, 146, 147, 150, 164, 165, 166, 168, 171, 172, 173]  # Envelope attack, hold, decay, release
    envelope_sustains = [46, 140, 149]  # Sustain levels for harp and chord envelopes
    volume_params = [2, 3]  # Harp and chord volume
    noise_amplitude = [130]  # Chord noise amplitude
    transient_amplitude = [42]  # Harp transient waveform
    rhythm_patterns = list(range(220, 236))  # Rhythm pattern indices
    harp_filter_mix = [90, 91, 92]  # Harp output filter lowpass, bandpass, highpass
    chord_filter_mix = [194, 195, 196]  # Chord output filter lowpass, bandpass, highpass
    chord_note_amplitudes = [131, 132, 133, 134]  # Chord oscillator note amplitudes
    chord_freq_multipliers = [123, 126, 129]  # Chord oscillator frequency multipliers
    reverb_params = [25, 26, 27, 28]  # Reverb high damping, low damping, low pass, diffusion
    effects_mix = [83, 182, 183]  # Harp effects dry mix, chord effects dry mix, chord effects delay mix
    
    # Ensure volumes are audible (0-100 scale, tightened to defaults)
    for index in volume_params:
        preset[index] = random.randint(50, 75)  # Integer, closer to default 50
    
    # Ensure harp oscillator and transient are audible
    preset[41] = random.randint(30, 70)  # Harp oscillator (0-100 scale, matches defaults)
    preset[42] = random.randint(0, 11)  # Transient waveform (0-11 scale, per parameter_ranges)
    
    # Ensure at least one chord oscillator or noise is robustly audible
    chord_sound_sources = [121, 124, 127, 130]  # Chord oscillators + noise
    chosen_chord_source = random.choice(chord_sound_sources)
    preset[chosen_chord_source] = random.randint(50, 100)  # Strong amplitude (0-100 scale, matches defaults)
    
    # Set remaining chord oscillators and noise
    for index in chord_sound_sources:
        if index != chosen_chord_source:
            preset[index] = random.randint(0, 50)  # Low or off, matches defaults
    
    # Ensure output amplifiers are non-zero
    preset[97] = random.randint(80, 100)  # Harp amplifier (0-100 scale, matches defaults)
    preset[197] = random.randint(50, 80)  # Chord amplifier (0-100 scale, avoids harshness)
    
    # Ensure harp and chord effects mix are non-zero
    for index in effects_mix:
        preset[index] = random.randint(50, 100)  # Non-zero for audibility (0-100 scale, matches logs)
    
    # Ensure at least one harp filter mix is non-zero
    chosen_harp_filter = random.choice(harp_filter_mix)
    preset[chosen_harp_filter] = random.randint(50, 100)  # Non-zero (0-100 scale, matches defaults)
    for index in harp_filter_mix:
        if index != chosen_harp_filter:
            preset[index] = random.randint(0, 50)  # Low or off, matches defaults
    
    # Ensure at least one chord filter mix is non-zero
    chosen_chord_filter = random.choice(chord_filter_mix)
    preset[chosen_chord_filter] = random.randint(50, 100)  # Non-zero (0-100 scale, matches defaults)
    for index in chord_filter_mix:
        if index != chosen_chord_filter:
            preset[index] = random.randint(0, 50)  # Low or off, matches defaults
    
    # Ensure chord note amplitudes are non-zero
    for index in chord_note_amplitudes:
        preset[index] = random.randint(50, 100)  # Non-zero (0-100 scale, matches defaults)
    
    # Ensure chord frequency multipliers are musical and match logs
    for index in chord_freq_multipliers:
        preset[index] = random.randint(100, 150)  # Non-zero, matches log values (0-100 scale)
    
    # Ensure sustain levels are non-zero
    for index in envelope_sustains:
        preset[index] = round(random.uniform(0.8, 1.0), 2)  # Non-zero sustain (0-1 scale, matches defaults)
    
    # Ensure reverb parameters are non-zero for effect contribution
    for index in reverb_params:
        preset[index] = round(random.uniform(0.2, 0.6), 2)  # Non-zero for audible reverb (matches defaults)
    
    # Ensure at least one rhythm pattern is non-zero
    preset[random.choice(rhythm_patterns)] = random.randint(1, 65)  # Non-zero, matches defaults
    
    # Generate random values for defined parameters, excluding fixed indices 0-9
    for index, params in parameter_ranges.items():
        if index in fixed_values or index in volume_params or index in envelope_sustains or index in transient_amplitude or index in oscillator_amplitudes or index in noise_amplitude or index in output_amplifiers or index in harp_filter_mix or index in chord_filter_mix or index in chord_note_amplitudes or index in chord_freq_multipliers or index in effects_mix or index in reverb_params:
            continue  # Already set above
        min_val = params["min"]
        max_val = params["max"]
        data_type = params["type"]
        
        if index in resonance_params:
            # Limit resonance to avoid harshness
            preset[index] = round(random.uniform(min_val, min(3.0, max_val)), 2)
        elif index in envelope_times:
            # Bias envelope times to shorter, musical values
            preset[index] = random.randint(min_val, min(1500, max_val))
        elif index in rhythm_patterns:
            # Random rhythm values, already ensured one non-zero
            preset[index] = random.randint(0, 65)
        else:
            # Use original ranges for other parameters
            if data_type == "int":
                preset[index] = random.randint(min_val, max_val)
            else:  # float
                preset[index] = round(random.uniform(min_val, max_val), 2)
    
    # Encode the preset and copy to clipboard
    encoded_preset = encode_preset(preset)
    try:
        pyperclip.copy(encoded_preset)
        print("Random Preset Array:", preset)
        print("Encoded Preset (copied to clipboard):", encoded_preset)
    except pyperclip.PyperclipException:
        with open("preset.txt", "w") as f:
            f.write(encoded_preset)
        print("Random Preset Array:", preset)
        print("Encoded Preset (saved to preset.txt due to clipboard error):", encoded_preset)
        print("Manually copy the encoded preset above if needed.")
    
    return preset

def encode_preset(preset):
    # Convert preset array to a semicolon-separated string
    preset_str = ";".join(str(int(val)) if isinstance(val, int) else str(val) for val in preset)
    # Encode to base64
    preset_bytes = preset_str.encode('ascii')
    base64_bytes = base64.b64encode(preset_bytes)
    base64_str = base64_bytes.decode('ascii')
    return base64_str

def decode_preset(encoded):
    # Decode from base64
    base64_bytes = encoded.encode('ascii')
    preset_bytes = base64.b64decode(base64_bytes)
    preset_str = preset_bytes.decode('ascii')
    # Split into array and convert to appropriate types
    preset = []
    for val in preset_str.split(";"):
        try:
            if "." in val:
                preset.append(float(val))
            else:
                preset.append(int(val))
        except ValueError:
            preset.append(0)  # Fallback for invalid values
    return preset

# Generate and encode a preset
random_preset = generate_random_preset()
