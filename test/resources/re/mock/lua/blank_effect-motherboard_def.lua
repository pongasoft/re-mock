format_version = "2.0"

--------------------------------------------------------------------------
-- Initialization
--------------------------------------------------------------------------
-- Custom properties
local documentOwnerProperties = {}
local rtOwnerProperties = {}
local guiOwnerProperties = {}

-- Audio Inputs/Outputs
audio_outputs = {}
audio_inputs = {}

-- CV Inputs/Outputs
cv_inputs = {}
cv_outputs = {}

--------------------------------------------------------------------------
-- Properties
--------------------------------------------------------------------------
--------------------------------------------------------------------------
-- stereo pair MainInLeft / MainInRight
--------------------------------------------------------------------------
audio_inputs["MainInLeft"] = jbox.audio_input {
  ui_name = jbox.ui_text("MainInLeft ui_name")
}
audio_inputs["MainInRight"] = jbox.audio_input {
  ui_name = jbox.ui_text("MainInRight ui_name")
}

--------------------------------------------------------------------------
-- stereo pair MainOutLeft / MainOutRight
--------------------------------------------------------------------------
audio_outputs["MainOutLeft"] = jbox.audio_output {
  ui_name = jbox.ui_text("MainOutLeft ui_name")
}
audio_outputs["MainOutRight"] = jbox.audio_output {
  ui_name = jbox.ui_text("MainOutRight ui_name")
}

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
    }
  },

  rt_owner = {
    properties = rtOwnerProperties
  }
}

--------------------------------------------------------------------------
-- Routing
--------------------------------------------------------------------------
jbox.add_stereo_audio_routing_pair {
  left = "/audio_inputs/MainInLeft",
  right = "/audio_inputs/MainInRight",
}

jbox.add_stereo_audio_routing_pair {
  left = "/audio_outputs/MainOutLeft",
  right = "/audio_outputs/MainOutRight",
}

jbox.add_stereo_effect_routing_hint {
  type = "true_stereo",
  left_input = "/audio_inputs/MainInLeft",
  right_input = "/audio_inputs/MainInRight",
  left_output = "/audio_outputs/MainOutLeft",
  right_output = "/audio_outputs/MainOutRight"
}

jbox.add_stereo_audio_routing_target {
  signal_type = "normal",
  left = "/audio_outputs/MainOutLeft",
  right = "/audio_outputs/MainOutRight",
  auto_route_enable = true
}

jbox.add_stereo_audio_routing_target {
  signal_type = "normal",
  left = "/audio_inputs/MainInLeft",
  right = "/audio_inputs/MainInRight",
  auto_route_enable = true
}

jbox.set_effect_auto_bypass_routing {
  {
    "/audio_inputs/MainInLeft",
    "/audio_outputs/MainOutLeft"
  },
  {
    "/audio_inputs/MainInRight",
    "/audio_outputs/MainOutRight"
  }
}
