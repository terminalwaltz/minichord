
// Global controller instance
const miniChordController = new MiniChordController();

// Legacy global variables for backward compatibility
let minichord_device = false;
let active_bank_number = -1;

//-->>UTILITIES
function map_value(value, in_min, in_max, out_min, out_max) {
  return (value - in_min) * (out_max - out_min) / (in_max - in_min) + Number(out_min);
}

//-->>INTERFACE HANDLER
//Slider Handler
// Track current and initial octave values and transposition state
let chord_octave; // Current for address 99, undefined until set
let harp_octave;  // Current for address 198, undefined until set
let initial_chord_octave = 2; // Default firmware octave for address 99
let initial_harp_octave = 2;  // Default firmware octave for address 198
let transposition_state = "positive"; // Track if in positive or negative transposition

function mapNegativeTransposition(range_value) {
  let transpose_value;
  let chord_octave_value;
  let harp_octave_value;

  if (range_value < 0) {
    transpose_value = Math.round(range_value + 12); // e.g., -1 → 11
    chord_octave_value = 1; // +12 semitones
    harp_octave_value = 1;  // +12 semitones
    console.log("Negative transposition:", { range_value, transpose_value, chord_octave_value, harp_octave_value, initial_chord_octave, initial_harp_octave });
    transposition_state = "negative";
  } else {
    transpose_value = Math.round(range_value); // e.g., 12 → 12
    if (transposition_state === "negative") {
      chord_octave_value = initial_chord_octave;
      harp_octave_value = initial_harp_octave;
      console.log("Transition to positive transposition:", { range_value, transpose_value, chord_octave_value, harp_octave_value, initial_chord_octave, initial_harp_octave });
      transposition_state = "positive";
    } else {
      chord_octave_value = null;
      harp_octave_value = null;
      console.log("Continued positive transposition:", { range_value, transpose_value });
    }
  }

  return {
    transpose_value: Math.max(0, Math.min(127, transpose_value)),
    chord_octave_value,
    harp_octave_value
  };
}

function handlechange(element) {
  console.log("handlechange triggered, element:", {
    tagName: element?.tagName || "undefined",
    value: element?.value || "undefined",
    attributes: element ? Array.from(element.attributes || []).reduce((obj, attr) => {
      obj[attr.name] = attr.value;
      return obj;
    }, {}) : "no attributes"
  });

  if (!element) {
    console.error("handlechange: Invalid element");
    return;
  }

  try {
    if (miniChordController.isConnected()) {
      console.log("MiniChord connected");
      const curve_type = element.getAttribute("curve");
      let range_value = parseFloat(element.value) || 0;
      console.log("range_value:", range_value, "curve_type:", curve_type);

      if (curve_type === "exponential") {
        range_value = Math.exp((Math.log(parseFloat(element.max)) / parseFloat(element.max)) * range_value);
      }

      let displayed_value;
      const address = String(element.getAttribute("adress_field") || "");
      const transposition_address = "30";
      const chord_octave_address = "99";
      const harp_octave_address = "198";

      if (element.getAttribute("data_type") === "int") {
        displayed_value = map_value(range_value, parseFloat(element.min), parseFloat(element.max),
          parseFloat(element.getAttribute("target_min")), parseFloat(element.getAttribute("target_max"))).toFixed(0);

        if (address === transposition_address) {
          const { transpose_value, chord_octave_value, harp_octave_value } = mapNegativeTransposition(range_value);
          miniChordController.sendParameter(transposition_address, transpose_value);
          console.log("sendParameter called:", { address: transposition_address, transpose_value });
          if (chord_octave_value !== null) {
            miniChordController.sendParameter(chord_octave_address, chord_octave_value);
            console.log("sendParameter called:", { address: chord_octave_address, chord_octave_value });
            chord_octave = chord_octave_value;
          }
          if (harp_octave_value !== null) {
            miniChordController.sendParameter(harp_octave_address, harp_octave_value);
            console.log("sendParameter called:", { address: harp_octave_address, harp_octave_value });
            harp_octave = harp_octave_value;
          }
        } else {
          const send_value = Math.max(0, Math.round(range_value));
          console.log("Non-transposition int:", { send_value });
          miniChordController.sendParameter(address, send_value);
          console.log("sendParameter called:", { address, send_value });
          if (address === chord_octave_address) {
            chord_octave = send_value;
            initial_chord_octave = send_value;
            console.log("Updated chord_octave:", chord_octave, "Initial:", initial_chord_octave);
          } else if (address === harp_octave_address) {
            harp_octave = send_value;
            initial_harp_octave = send_value;
            console.log("Updated harp_octave:", harp_octave, "Initial:", initial_harp_octave);
          }
        }
      } else if (element.getAttribute("data_type") === "float") {
        displayed_value = map_value(range_value, parseFloat(element.min), parseFloat(element.max),
          parseFloat(element.getAttribute("target_min")), parseFloat(element.getAttribute("target_max"))).toFixed(2);
        const send_value = Math.round(range_value * miniChordController.float_multiplier);
        miniChordController.sendParameter(address, send_value);
        console.log("sendParameter called:", { address, send_value });
      }

      console.log("Final:", { address, displayed_value });
      const value_zone = document.getElementById("value_zone" + address);
      if (value_zone) {
        value_zone.innerHTML = displayed_value;
      }

      if (address === miniChordController.color_hue_sysex_adress) {
        const hue = Math.round(Math.max(0, Math.min(360, range_value)));
        const elements = document.getElementsByClassName('slider');
        for (let i = 0; i < elements.length; i++) {
          elements[i].style.setProperty('--slider_color', 'hsl(' + hue + ',100%,50%)');
        }
      }
    } else {
      console.log("MiniChord not connected");
      element.value = element.getAttribute("value");
      document.getElementById("information_zone")?.focus();
    }
  } catch (error) {
    console.error("handlechange error:", error);
  }
}


//function that builds the checkboxes for rythm pattern
function checkbox_array() {
  var container
  for (var i = 0; i <  + 16; i++) {
    target_adress=miniChordController.base_adress_rythm+i
    container = document.getElementById(target_adress);
    if(i<7){
      container.innerHTML = "";
      var padding_name = document.createElement("div");
      padding_name.className="bloc B4 M0 S0";
      padding_name.innerHTML=""
      container.appendChild(padding_name);
      var padding_category = document.createElement("div");
      padding_category.className="bloc B3 M2 S3";
      padding_category.innerHTML=""
      container.appendChild(padding_category);
      var padding_ID = document.createElement("div");
      padding_ID.className="bloc B1 M1 S1";
      padding_ID.innerHTML="***"
      container.appendChild(padding_ID);
      var name_zone = document.createElement("div");
      name_zone.className=" bloc B4 M3 S4";
      if(i<4){
      name_zone.innerHTML="voice "+(i+1)
      }else{
      name_zone.innerHTML="voice "+(i-2)+'"'
      }
      container.appendChild(name_zone);
      var line = document.createElement("div");
      line.className="checkbox_div bloc B5 M4 S7";
      container.appendChild(line);
  
      for (var j = 0; j < 16; j++) {
        var checkBox = document.createElement("input");
        checkBox.onclick=send_array_data;
        checkBox.type = "checkbox";
        checkBox.id = "checkbox" + i + j;
  
        line.appendChild(checkBox);
      }
    }else{
    container.innerHTML = "";
    container.style.display = 'block';
    }
  }
}
checkbox_array();
function send_array_data() {
  for (var i = 0; i < 16; i++) {
    var output_value = 0;
    for (var j = 0; j < 7; j++) {
      var checkbox = document.getElementById("checkbox" + j + i);
      if (null != checkbox) {
        output_value = output_value | (checkbox.checked << j);
      }
    }
    console.log(output_value);
    miniChordController.sendParameter(miniChordController.base_adress_rythm + i, output_value);
  }
}
//-->>UI INTEGRATION
// UI callbacks for controller events
miniChordController.onConnectionChange = function(connected, message) {
  if (connected) {
    document.getElementById("step3").classList.remove("unsatisfied");
    document.getElementById("information_text").innerHTML = "";
    document.getElementById("status_zone").className = "";
    document.getElementById("status_zone").classList.add("connected");
    var body = document.getElementById('body');
    body.classList.remove("control_full");
  } else {
    document.getElementById("information_text").innerHTML = message;
    document.getElementById("information_zone").focus();
    if (message.includes("disconnected")) {
      document.getElementById("status_zone").className = "";
      document.getElementById("status_zone").classList.add("disconnected");
      var body = document.getElementById('body');
      window.scrollTo(0, 0);
      body.classList.add("control_full");
      var elements = document.getElementsByClassName('active');
      while (elements.length > 0) {
        elements.item(0).classList.add("inactive");
        elements[0].classList.remove("active");
      }
    }
  }
  // Update legacy variable for backward compatibility
  minichord_device = miniChordController.isConnected();
};

miniChordController.onDataReceived = function(data) {
  // Update sliders
  for (let i = 2; i < miniChordController.parameter_size; i++) {
    if (data.parameters[i] !== undefined) {
      set_slider_to_value(i, data.parameters[i]);
    }
  }
  
  // Update rhythm checkboxes
  for (let j = 0; j < data.rhythmData.length; j++) {
    const rhythmBits = data.rhythmData[j];
    if (rhythmBits) {
      for (let k = 0; k < 7; k++) {
        const checkbox = document.getElementById("checkbox" + k + j);
        if (checkbox) {
          checkbox.checked = rhythmBits[k];
        }
      }
    }
  }
  
  // Update bank selection
  const element = document.getElementById("bank_number_selection");
  if (element) {
    element.value = data.bankNumber;
  }
  active_bank_number = data.bankNumber;
  
  // Apply color theme
  const result = document.querySelectorAll('[adress_field="' + miniChordController.color_hue_sysex_adress + '"]');
  if (result.length > 0) {
    const hue = result[0].valueAsNumber;
    const elements = document.getElementsByClassName('slider');
    for (let i = 0; i < elements.length; i++) {
      elements[i].style.setProperty('--slider_color', 'hsl(' + hue + ',100%,50%)');
    }
  }
  
  // Update UI state
  document.getElementById("step3").classList.remove("unsatisfied");
  document.getElementById("information_text").innerHTML = "";
  document.getElementById("status_zone").className = "";
  document.getElementById("status_zone").classList.add("connected");
  
  var body = document.getElementById('body');
  body.classList.remove("control_full");
  var elements = document.querySelectorAll('.inactive');
  
  for (let i = 0; i < elements.length; i++) {
    if (elements[i].getAttribute("version") <= data.firmwareVersion) {
      elements[i].classList.add("active");
      elements[i].classList.remove("inactive");
    }
  }
  
  // Update connection status
  miniChordController.onConnectionChange(true, "");
};

// Initialize the controller
async function initializeMidiController() {
  try {
    const success = await miniChordController.initialize();
    if (success) {
      document.getElementById("step1").classList.remove("unsatisfied");
      document.getElementById("step2").classList.remove("unsatisfied");
    } else {
      document.getElementById("information_text").innerHTML = "> please use a compatible browser";
      document.getElementById("information_zone").focus();
    }
  } catch (error) {
    console.log(">> ERROR: MIDI initialization failed");
    console.error(error);
    document.getElementById("information_text").innerHTML = "> please reload and provide the authorisation to access the minichord";
    document.getElementById("information_zone").focus();
  }
}

// Start initialization
initializeMidiController();

function set_slider_to_value(slider_num, sysex_value) {
  var result = document.querySelectorAll('[adress_field="' + slider_num + '"]');
  if (result.length > 0) {
    var slider_value;
    if (result[0].getAttribute("curve") == "exponential") {
      slider_value = Math.round((result[0].max * Math.log(sysex_value) / Math.log(result[0].max)));
    } else {
      slider_value = sysex_value;
    }
    if (result[0].getAttribute("data_type") == "float") {
      result[0].value = slider_value / miniChordController.float_multiplier;
      var value_zone = document.getElementById("value_zone" + slider_num);
      if (value_zone) {
        value_zone.innerHTML = sysex_value / miniChordController.float_multiplier;
      }
    } else {
      result[0].value = slider_value;
      var value_zone = document.getElementById("value_zone" + slider_num);
      if (value_zone) {
        value_zone.innerHTML = sysex_value;
      }
    }
  }
}


//-->>COMMUNICATION HANDLERS 
function reset_memory() {
  if (miniChordController.isConnected()) {
    return miniChordController.resetMemory();
  } else {
    document.getElementById("information_zone").focus();
    return false;
  }
}

function save_current_settings() {
  if (miniChordController.isConnected()) {
    var e = document.getElementById("bank_number_selection");
    var bank_number = e.value;
    console.log(bank_number);
    return miniChordController.saveCurrentSettings(bank_number);
  } else {
    document.getElementById("information_zone").focus();
    return false;
  }
}

function reset_current_bank() {
  if (miniChordController.isConnected()) {
    console.log(miniChordController.active_bank_number);
    return miniChordController.resetCurrentBank();
  } else {
    document.getElementById("information_zone").focus();
    return false;
  }
}

//-->>SETTINGS OFFLINE SHARE
function generate_settings() {
  if (!miniChordController.isConnected()) {
    document.getElementById("information_zone").focus();
  }else{
    var sysex_array = Array(miniChordController.parameter_size).fill(0);
    var sliders = document.querySelectorAll(".slider")
    for (let i = 0; i < sliders.length; i++) {
      curve_type = sliders[i].getAttribute("curve")
      range_value = 0;
      adress = sliders[i].getAttribute("adress_field")
      if (curve_type == "exponential") {
        range_value = Math.round(Math.exp((Math.log(sliders[i].max) / sliders[i].max) * sliders[i].value));
      } else {
        range_value = sliders[i].value
      }

      if (sliders[i].getAttribute("data_type") == "float") {
        range_value = Math.round(range_value * 100);
      }
      sysex_array[adress] = range_value
    }
    //now the rythm pattern
    for (var i = 0; i < 16; i++) {
      var output_value = 0
      for (var j = 0; j < 7; j++) {
        var checkbox = document.getElementById("checkbox" + j + i);
        if (null != checkbox) {
          output_value = output_value | checkbox.checked << j;
        }

      }
      sysex_array[miniChordController.base_adress_rythm + i] = output_value
    }
    output_base64="";
    output_string="{";
    for (let i = 0; i < (miniChordController.parameter_size - 1); i++) {
      output_string += String(sysex_array[i])
      output_string += ","
      output_base64+=sysex_array[i];
      output_base64+=";"
    }
    output_string += String(sysex_array[miniChordController.parameter_size - 1]);
    output_string += "},"
    encoded=btoa(output_base64)
    console.log(encoded)
    navigator.clipboard.writeText(encoded);
    alert("Preset code copied to clipboard");
    console.log(atob(encoded))
    document.getElementById("output_zone").innerHTML = output_string;
  }
}

function load_settings() {
  if (!miniChordController.isConnected()) {
    document.getElementById("information_zone").focus();
  }else{
    let preset_code = prompt('Paste preset code');
    if(preset_code!=null){
      parameters=atob(preset_code).split(";");
      if(parameters.length!=miniChordController.parameter_size){
        alert("malformed preset code");
      }else{
        adress_index=2; //skip the command and bank number
        for (adress_index; adress_index<miniChordController.parameter_size;adress_index++) {
          miniChordController.sendParameter(adress_index, parameters[adress_index]);
        } 
        miniChordController.sendParameter(0, 0);//we ask the minichord to update the interface
      }
    }
  }
}

// Add hover highlighting for parameter names
function initializeSliderHoverEffects() {
  const sliders = document.querySelectorAll('.slider');
  
  sliders.forEach(slider => {
    // Find the corresponding dfn element in the same line
    const parentLine = slider.closest('.content_line');
    if (parentLine) {
      const dfnElement = parentLine.querySelector('dfn');
      
      if (dfnElement) {
        slider.addEventListener('mouseenter', () => {
          dfnElement.classList.add('highlighted');
        });
        
        slider.addEventListener('mouseleave', () => {
          dfnElement.classList.remove('highlighted');
        });
      }
    }
  });
}

// Initialize hover effects when the page loads
document.addEventListener('DOMContentLoaded', initializeSliderHoverEffects);