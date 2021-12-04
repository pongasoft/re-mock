format_version = "1.0"

rtc_bindings = {
  -- this will initialize the C++ object
  { source = "/environment/system_sample_rate", dest = "/global_rtc/init_instance" },
  { source = "/user_samples/0/item", dest = "/global_rtc/on_prop_user_sample_0" },
  { source = "/user_samples/1/item", dest = "/global_rtc/on_prop_user_sample_1" },
}

function on_prop_user_sample(index, new_value)
  local info = jbox.get_sample_info(new_value)
  local metadata = jbox.get_sample_meta_data(new_value)
  jbox.store_property("/custom_properties/on_prop_user_sample_return_" .. tostring(index),
      "i=" .. tostring(index) ..
          ";i.fc=" .. tostring(info.frame_count) ..
          ";i.rfc=" .. tostring(info.resident_count) ..
          ";i.ch=" .. tostring(info.channels) ..
          ";i.sr=" .. tostring(info.sample_rate) ..
          ";i.st=" .. tostring(info.state) ..
          ";m.fc=" .. tostring(metadata.frame_count) ..
          ";m.rfc=" .. tostring(metadata.resident_frame_count) ..
          ";m.ch=" .. tostring(metadata.channels) ..
          ";m.sr=" .. tostring(metadata.sample_rate) ..
          ";m.ls=" .. tostring(metadata.load_status) ..
          ";m.sn=[" .. tostring(metadata.sample_name) .. "]" ..
          ";m.rk=" .. tostring(metadata.root_key) ..
          ";m.tc=" .. tostring(metadata.tune_cents) ..
          ";m.prs=" .. tostring(metadata.play_range_start) ..
          ";m.pre=" .. tostring(metadata.play_range_end) ..
          ";m.lrs=" .. tostring(metadata.loop_range_start) ..
          ";m.lre=" .. tostring(metadata.loop_range_end) ..
          ";m.lm=" .. tostring(metadata.loop_mode) ..
          ";m.pvl=" .. tostring(metadata.preview_volume_level))
end

global_rtc = {
  init_instance = function(source_property_path, new_value)
    local sample_rate = jbox.load_property("/environment/system_sample_rate")
    local new_no = jbox.make_native_object_rw("Instance", { sample_rate })
    jbox.store_property("/custom_properties/instance", new_no);
  end,

  on_prop_user_sample_0 = function(source_property_path, new_value)
    on_prop_user_sample(0, new_value)
  end,

  on_prop_user_sample_1 = function(source_property_path, new_value)
    on_prop_user_sample(1, new_value)
  end,
}

rt_input_setup = {
  notify = {
    "/user_samples/0/*",
    "/user_samples/1/*",
  }
}
