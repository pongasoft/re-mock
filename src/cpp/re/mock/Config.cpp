/*
 * Copyright (c) 2021 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include "Config.h"

namespace re::mock {

ConfigString Config::SKELETON_MOTHERBOARD_DEF{R"(
format_version = "2.0"
gui_owner_properties = {}
document_owner_properties = {}
rtc_owner_properties = { instance = jbox.native_object{ } }
rt_owner_properties = {}
audio_outputs = {}
audio_inputs = {}
cv_inputs = {}
cv_outputs = {}
custom_properties = jbox.property_set {
  gui_owner = { properties = gui_owner_properties },
  document_owner = { properties = document_owner_properties },
  rtc_owner = { properties = rtc_owner_properties },
  rt_owner = { properties = rt_owner_properties }
}
)"};

ConfigString Config::SKELETON_REALTIME_CONTROLLER{R"(
format_version = "1.0"
rtc_bindings = {
  { source = "/environment/system_sample_rate", dest = "/global_rtc/init_instance" },
}
global_rtc = {
  init_instance = function(source_property_path, new_value)
    local new_no = jbox.make_native_object_rw("Instance", { new_value })
    jbox.store_property("/custom_properties/instance", new_no);
  end,
}
)"};

//------------------------------------------------------------------------
// Config::skeleton
//------------------------------------------------------------------------
Config Config::fromSkeleton()
{
  return Config().mdef(SKELETON_MOTHERBOARD_DEF).rtc(SKELETON_REALTIME_CONTROLLER);
}

//------------------------------------------------------------------------
// Config::stereoOut
//------------------------------------------------------------------------
ConfigString Config::stereo_audio_out(char const *iLeftSocketName, char const *iRightSocketName)
{
  return {
    fmt::printf(R"(audio_outputs["%s"] = jbox.audio_output{};audio_outputs["%s"] = jbox.audio_output{})",
                iLeftSocketName,
                iRightSocketName)
  };
}

//------------------------------------------------------------------------
// Config::stereoIn
//------------------------------------------------------------------------
ConfigString Config::stereo_audio_in(char const *iLeftSocketName, char const *iRightSocketName)
{
  return {
    fmt::printf(R"(audio_inputs["%s"] = jbox.audio_input{};audio_inputs["%s"] = jbox.audio_input{})",
                iLeftSocketName,
                iRightSocketName)
  };
}

//------------------------------------------------------------------------
// Config::audio_out
//------------------------------------------------------------------------
ConfigString Config::audio_out(std::string const &iSocketName)
{
  return {fmt::printf(R"(audio_outputs["%s"] = jbox.audio_output{})", iSocketName)};
}

//------------------------------------------------------------------------
// Config::audio_in
//------------------------------------------------------------------------
ConfigString Config::audio_in(std::string const &iSocketName)
{
  return {fmt::printf(R"(audio_inputs["%s"] = jbox.audio_input{})", iSocketName)};
}

//------------------------------------------------------------------------
// Config::cv_out
//------------------------------------------------------------------------
ConfigString Config::cv_out(char const *iSocketName)
{
  return {fmt::printf(R"(cv_outputs["%s"] = jbox.cv_output{})", iSocketName)};
}

//------------------------------------------------------------------------
// Config::cv_in
//------------------------------------------------------------------------
ConfigString Config::cv_in(char const *iSocketName)
{
  return {fmt::printf(R"(cv_inputs["%s"] = jbox.cv_input{})", iSocketName)};
}

//------------------------------------------------------------------------
// Config::rtc_binding
//------------------------------------------------------------------------
ConfigString Config::rtc_binding(std::string const &iSource, std::string const &iDest)
{
  return {fmt::printf(R"(rtc_bindings[#rtc_bindings + 1] = { source ="%s", dest = "%s"})", iSource, iDest)};
}

namespace impl {

inline std::string property_tag(int iPropertyTag)
{
  if(iPropertyTag > 0)
    return fmt::printf(", property_tag = %d", iPropertyTag);
  else
    return "";
}

inline ConfigString custom_property(std::string const &iPropertyType,
                                    std::string const &iPropertyName,
                                    lua::jbox_boolean_property const &iProperty)
{
  return { fmt::printf(R"(%s["%s"] = jbox.boolean{ default = %s%s })",
                       iPropertyType, iPropertyName, iProperty.default_value ? "true" : "false", property_tag(iProperty.property_tag)) };
}

inline ConfigString custom_property(std::string const &iPropertyType,
                                    std::string const &iPropertyName,
                                    lua::jbox_number_property const &iProperty)
{
  return { fmt::printf(R"(%s["%s"] = jbox.number{ default = %f%s })",
                       iPropertyType, iPropertyName, iProperty.default_value, property_tag(iProperty.property_tag)) };
}

}

//------------------------------------------------------------------------
// Config::document_owner_property
//------------------------------------------------------------------------
ConfigString Config::document_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty)
{
  return impl::custom_property("document_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::document_owner_property
//------------------------------------------------------------------------
ConfigString Config::document_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty)
{
  return impl::custom_property("document_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rt_owner_property
//------------------------------------------------------------------------
ConfigString Config::rt_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty)
{
  return impl::custom_property("rt_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rt_owner_property
//------------------------------------------------------------------------
ConfigString Config::rt_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty)
{
  return impl::custom_property("rt_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
ConfigString Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty)
{
  return impl::custom_property("rtc_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
ConfigString Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty)
{
  return impl::custom_property("rtc_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
ConfigString Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_native_object const &iProperty)
{
  auto defaultValue = std::string{};
  if(!iProperty.default_value.operation.empty())
  {
    auto params = std::string{};
    if(!iProperty.default_value.params.empty())
    {
      for(int i = 0; i < iProperty.default_value.params.size(); i++)
      {
        if(i > 0)
          params += ", ";
        auto const &param = iProperty.default_value.params[i];
        switch(JBox_GetType(param))
        {
          case kJBox_Boolean:
            params += JBox_GetBoolean(param) ? "true" : "false";
            break;

          case kJBox_Number:
            params += std::to_string(JBox_GetNumber(param));
            break;

          default:
            RE_MOCK_ASSERT(false, "Invalid param [%d] type for jbox.native_object{} [%s]", i, iPropertyName);
            break;
        }
      }
    }

    defaultValue = fmt::printf(R"(default = { "%s", { %s } })", iProperty.default_value.operation, params);
  }
  return { fmt::printf(R"(rtc_owner_properties["%s"] = jbox.native_object { %s%s })",
                       iPropertyName, defaultValue, impl::property_tag(iProperty.property_tag)) };
}


}