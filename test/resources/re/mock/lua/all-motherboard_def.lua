format_version = "2.0"

local documentOwnerProperties = {
  doc_boolean = jbox.boolean { property_tag = 100, default = true },
  doc_number = jbox.number { property_tag = 101, default = 3 },
  doc_number_with_steps = jbox.number { property_tag = 110, default = 2, steps = 5 },
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
        default = { "Operation", { 0.5, true, 48000, "abc" } }
      },
    }
  },

  rt_owner = {
    properties = rtOwnerProperties
  }
}

user_samples = {
  -- Each sample are available from the lua code as /user_samples/0, /user_samples/1 etc
  jbox.user_sample{
    ui_name = jbox.ui_text("user_samples_sample_0"),
    sample_parameters = { "root_key", "tune_cents", "play_range_start", "play_range_end", "loop_range_start", "loop_range_end", "loop_mode", "preview_volume_level" }
  },

  jbox.user_sample{
    ui_name = jbox.ui_text("user_samples_sample_1"),
    sample_parameters = { "preview_volume_level" }
  }
}

patterns = { num_patterns = 3 }
