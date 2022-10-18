format_version = "2.0"

local documentOwnerProperties = {
  invalid_boolean = jbox.boolean()
}

local rtOwnerProperties = {
  rt_number = jbox.number { property_tag = 102 },
  rt_string = jbox.string { max_size = 100 }
}

local guiOwnerProperties = {
  gui_boolean = jbox.boolean { default = true },
--   gui_boolean = jbox.boolean { property_tag = 599, default = true },
  gui_number = jbox.number { default = 5 },
  gui_string = jbox.string { default = "efg" },
}

audio_outputs = {}
audio_inputs = {}

cv_inputs = {}
cv_outputs = {}

audio_inputs["au_in"] = jbox.audio_input { ui_name = jbox.ui_text("au_in") }
audio_outputs["au_out"] = jbox.audio_output { ui_name = jbox.ui_text("au_out") }
-- audio_outputs["au_out2"] = jbox.audio_input { ui_name = jbox.ui_text("au_out") }
cv_inputs["cv_in"] = jbox.cv_input { ui_name = jbox.ui_text("cv_in") }
cv_outputs["cv_out"] = jbox.cv_output { ui_name = jbox.ui_text("cv_out") }

custom_properties = jbox.property_set {
  gui_owner = {
    properties = guiOwnerProperties,
    test = myBool
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
--        default = { "Operation", { 0.5, true, 48000, "abc",  documentOwnerProperties } }
      },
    }
  },

  rt_owner = {
    properties = rtOwnerProperties
  }
}

user_samples = {
  jbox.user_sample{
    ui_name = jbox.ui_text("user_samples_sample_0"),
    sample_parameters = { "root_key", "tune_cents", "play_range_start", "play_range_end", "loop_range_start", "loop_range_end", "loop_mode", "preview_volume_level" }
  },

  jbox.user_sample{
    ui_name = jbox.ui_text("user_samples_sample_1"),
    sample_parameters = { "preview_volume_level" }
--    sample_parameters = { "preview_volume_level", 4 }
  }
}

patterns = { num_patterns = 3 }
