format_version = "1.0"

rtc_bindings = {
  -- this will initialize the C++ object
  { source = "/environment/system_sample_rate", dest = "/global_rtc/init_instance" },
  { source = "/custom_properties/prop_function", dest = "/global_rtc/on_prop_function" },
  { source = "/custom_properties/prop_blob", dest = "/global_rtc/on_prop_blob" },
}

global_rtc = {
  init_instance = function(source_property_path, new_value)
    local sample_rate = jbox.load_property("/environment/system_sample_rate")
    local new_no = jbox.make_native_object_rw("Instance", { sample_rate })
    jbox.store_property("/custom_properties/instance", new_no);
  end,

  on_prop_blob = function(source_property_path, new_value)
    local blob_info = jbox.get_blob_info(new_value)
    jbox.store_property("/custom_properties/on_prop_blob_return",
        "is_blob=" .. tostring(jbox.is_blob(new_value)) ..
            ";size=" .. tostring(blob_info.size) ..
            ";resident_size=" .. tostring(blob_info.resident_size) ..
            ";state=" .. tostring(blob_info.state)
    )
  end,

  on_prop_function = function(source_property_path, new_value)
    if new_value == 'noop' then
      jbox.store_property("/custom_properties/prop_function_return", "noop -> void")
    elseif new_value == 'load_blob_data' then
      local new_no = jbox.load_blob_async("/Private/blob.data")
      jbox.store_property("/custom_properties/prop_blob", new_no)
      jbox.store_property("/custom_properties/prop_function_return", "load_blob_data -> " .. tostring(jbox.is_blob(new_no)))
    elseif new_value == 'load_blob_file' then
      local new_no = jbox.load_blob_async("/Private/blob.file")
      jbox.store_property("/custom_properties/prop_blob", new_no)
      jbox.store_property("/custom_properties/prop_function_return", "load_blob_file -> " .. tostring(jbox.is_blob(new_no)))
    elseif new_value == 'nil_blob' then
      local new_no = jbox.make_empty_blob()
      jbox.store_property("/custom_properties/prop_blob", new_no)
      jbox.store_property("/custom_properties/prop_function_return", "nil_blob -> " .. tostring(jbox.is_blob(new_no)))
    elseif new_value == 'is_blob' then
      local blob = jbox.load_property("/custom_properties/prop_blob")
      jbox.store_property("/custom_properties/prop_function_return", "is_blob -> " .. tostring(jbox.is_blob(blob)))
    end
  end,
}

rt_input_setup = {
  notify = {
    "/custom_properties/prop_blob",
    "/custom_properties/prop_function"
  }
}
