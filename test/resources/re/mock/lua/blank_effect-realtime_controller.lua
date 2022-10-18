format_version = "1.0"

rtc_bindings = {
  -- this will initialize the C++ object
  { source = "/environment/system_sample_rate", dest = "/global_rtc/init_instance" },
}

global_rtc = {
  init_instance = function(source_property_path, new_value)
    local sample_rate = jbox.load_property("/environment/system_sample_rate")
    local new_no = jbox.make_native_object_rw("Instance", { sample_rate })
    jbox.store_property("/custom_properties/instance", new_no)
  end,
}

rt_input_setup = {
  notify = {
    "/audio_inputs/MainInLeft/connected",
    "/audio_inputs/MainInRight/connected",
    "/audio_outputs/MainOutLeft/connected",
    "/audio_outputs/MainOutRight/connected"
  }
}

sample_rate_setup = {
  native = {
    22050,
    44100,
    48000,
    88200,
    96000,
    192000
  },

}