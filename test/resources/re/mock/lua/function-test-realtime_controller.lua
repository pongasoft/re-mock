format_version = "1.0"

rtc_bindings = {
  -- this will initialize the C++ object
  { source = "/environment/system_sample_rate", dest = "/global_rtc/init_instance" },
  { source = "/custom_properties/prop_function", dest = "/global_rtc/test_function" },
}

global_rtc = {
  init_instance = function(source_property_path, new_value)
    local sample_rate = jbox.load_property("/environment/system_sample_rate")
    local new_no = jbox.make_native_object_rw("Instance", { sample_rate })
    jbox.store_property("/custom_properties/instance", new_no);
  end,

  test_function = function(source_property_path, new_value)
    print("from test_function " .. tostring(new_value))
    if new_value == 'noop' then
      -- do nothing
      print("noop")
    elseif new_value == 'trace' then
      jbox.trace("from trace")
    elseif new_value == 'new_gain' then
      local new_no = jbox.make_native_object_rw("Gain", { 0.7 })
      jbox.store_property("/custom_properties/prop_gain", new_no)
      print("jbox.is_native_object(2) = " .. tostring(jbox.is_native_object(new_no)))
    elseif new_value == 'nil_gain' then
      local new_no = jbox.make_empty_native_object()
      jbox.store_property("/custom_properties/prop_gain", new_no)
      print("jbox.is_native_object(3) = " .. tostring(jbox.is_native_object(new_no)))
    end
  end,
}

rt_input_setup = {
  notify = {
    "/custom_properties/prop_gain"
  }
}
