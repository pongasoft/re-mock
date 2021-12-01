format_version = "1.0"

rtc_bindings = {
  -- this will initialize the C++ object
  { source = "/environment/system_sample_rate", dest = "/global_rtc/init_instance" },
  { source = "/custom_properties/prop_function", dest = "/global_rtc/on_prop_function" },
  { source = "/custom_properties/prop_sample", dest = "/global_rtc/on_prop_sample" },
}

global_rtc = {
  init_instance = function(source_property_path, new_value)
    local sample_rate = jbox.load_property("/environment/system_sample_rate")
    local new_no = jbox.make_native_object_rw("Instance", { sample_rate })
    jbox.store_property("/custom_properties/instance", new_no);
  end,

  on_prop_sample = function(source_property_path, new_value)
    --print("from on_prop_sample " .. tostring(new_value))
    local sample_info = jbox.get_sample_info(new_value)
    jbox.store_property("/custom_properties/on_prop_sample_return",
        "is_sample=" .. tostring(jbox.is_sample(new_value)) ..
            ";frame_count=" .. tostring(sample_info.frame_count) ..
            ";resident_count=" .. tostring(sample_info.resident_count) ..
            ";channels=" .. tostring(sample_info.channels) ..
            ";sample_rate=" .. tostring(sample_info.sample_rate) ..
            ";state=" .. tostring(sample_info.state)
    )
  end,

  on_prop_function = function(source_property_path, new_value)
    --print("from on_prop_function " .. tostring(new_value))
    if new_value == 'noop' then
      jbox.store_property("/custom_properties/prop_function_return", "noop -> void")
    elseif new_value == 'load_mono_sample_data' then
      local new_no = jbox.load_sample_async("/Private/mono_sample.data")
      jbox.store_property("/custom_properties/prop_sample", new_no)
      jbox.store_property("/custom_properties/prop_function_return", "load_mono_sample_data -> " .. tostring(jbox.is_sample(new_no)))
    elseif new_value == 'load_stereo_sample_data' then
      local new_no = jbox.load_sample_async("/Private/stereo_sample.data")
      jbox.store_property("/custom_properties/prop_sample", new_no)
      jbox.store_property("/custom_properties/prop_function_return", "load_stereo_sample_data -> " .. tostring(jbox.is_sample(new_no)))
    elseif new_value == 'nil_sample' then
      local new_no = jbox.make_empty_sample()
      jbox.store_property("/custom_properties/prop_sample", new_no)
      jbox.store_property("/custom_properties/prop_function_return", "nil_sample -> " .. tostring(jbox.is_sample(new_no)))
    end
  end,
}

rt_input_setup = {
  notify = {
    "/custom_properties/prop_sample",
    "/custom_properties/prop_function"
  }
}
