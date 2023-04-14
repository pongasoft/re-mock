format_version = "1.0"

rtc_bindings = {
  -- this will initialize the C++ object
  { source = "/environment/system_sample_rate", dest = "/global_rtc/init_instance" },
  { source = "/custom_properties/source_1", dest = "/global_rtc/on_source_1" },
  { source = "/custom_properties/source_1", dest = "/global_rtc/on_any_source" },
  { source = "/custom_properties/source_2", dest = "/global_rtc/on_any_source" },
}

global_rtc = {
  init_instance = function(source_property_path, new_value)
    local sample_rate = jbox.load_property("/environment/system_sample_rate")
    local new_no = jbox.make_native_object_rw("Instance", { sample_rate })
    jbox.store_property("/custom_properties/instance", new_no)
  end,

  on_source_1 = function(source_property_path, new_value)
    --print("from on_source_1 " .. tostring(new_value))
    jbox.store_property("/custom_properties/source_1_return", new_value)
  end,

  on_any_source = function(source_property_path, new_value)
    --print("from on_any_source(" .. tostring(source_property_path) .. "," .. tostring(new_value) .. ")")
    -- allowed because both are declared as sources of the callback
    local s1_value = jbox.load_property("/custom_properties/source_1")
    local s2_value = jbox.load_property("/custom_properties/source_2")
    if source_property_path == "/custom_properties/source_1" then
      assert(s1_value == new_value)
      jbox.store_property("/custom_properties/any_source_1_return", new_value)
    end
    if source_property_path == "/custom_properties/source_2" then
      assert(s2_value == new_value)
      jbox.store_property("/custom_properties/any_source_2_return", new_value)
    end
  end,

}
