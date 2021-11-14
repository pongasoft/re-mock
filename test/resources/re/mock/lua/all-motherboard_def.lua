format_version = "2.0"

local documentOwnerProperties = {
  doc_boolean = jbox.boolean { property_tag = 100, default = true },
  doc_number = jbox.number { property_tag = 101, default = 3 },
  doc_string = jbox.string { property_tag = 103, default = "abcd" },
  doc_mod_wheel = jbox.performance_modwheel { property_tag = 104 },
  doc_pitch_bend = jbox.performance_pitchbend { property_tag = 105 },
  doc_sustain_pedal = jbox.performance_sustainpedal { property_tag = 106 },
  doc_expression = jbox.performance_expression { property_tag = 107 },
  doc_breath_control = jbox.performance_breathcontrol { property_tag = 108 },
  doc_aftertouch = jbox.performance_aftertouch { property_tag = 109 },
}

local rtOwnerProperties = {
  rt_number = jbox.number { property_tag = 102 },
  rt_string = jbox.string { max_size = 100 }
}

-- ignored
local guiOwnerProperties = {
  gui_boolean = jbox.boolean { default = true },
  gui_number = jbox.number { default = 5 },
  gui_string = jbox.string { default = "efg" },
}

-- Audio Inputs/Outputs
audio_outputs = {}
audio_inputs = {}

-- CV Inputs/Outputs
cv_inputs = {}
cv_outputs = {}

audio_inputs["au_in"] = jbox.audio_input { ui_name = jbox.ui_text("au_in") }
audio_outputs["au_out"] = jbox.audio_output { ui_name = jbox.ui_text("au_out") }
cv_inputs["cv_in"] = jbox.cv_input { ui_name = jbox.ui_text("cv_in") }
cv_outputs["cv_out"] = jbox.cv_output { ui_name = jbox.ui_text("cv_out") }

--------------------------------------------------------------------------
-- Setup
--------------------------------------------------------------------------
custom_properties = jbox.property_set {
  gui_owner = {
    properties = guiOwnerProperties
  },

  document_owner = {
    properties = documentOwnerProperties
  },

  rtc_owner = {
    properties = {
      instance = jbox.native_object{ },
      instance_with_default = jbox.native_object{
        property_tag = 103,
        default = { "Operation", { 0.5, true, 48000 } }
      },
    }
  },

  rt_owner = {
    properties = rtOwnerProperties
  }
}
