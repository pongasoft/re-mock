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
#include "FileManager.h"
#include "PatchParser.h"
#include "lua/InfoLua.h"
#include <fstream>

namespace re::mock {

//------------------------------------------------------------------------
// Config::skeletonMotherboardDef
//------------------------------------------------------------------------
resource::String Config::skeletonMotherboardDef() {
  return {R"(
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
user_samples = {}
)"};
}

//------------------------------------------------------------------------
// Config::skeletonRealtimeController
//------------------------------------------------------------------------
resource::String Config::skeletonRealtimeController() {
  return {R"(
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
rt_input_setup = { notify = { } }
)"};
}

//------------------------------------------------------------------------
// Config::fromSkeleton
//------------------------------------------------------------------------
Config Config::fromSkeleton(Info const &iInfo)
{
  return Config(iInfo).mdef(skeletonMotherboardDef()).rtc(skeletonRealtimeController());
}


//------------------------------------------------------------------------
// Config::stereoOut
//------------------------------------------------------------------------
resource::String Config::stereo_audio_out(char const *iLeftSocketName, char const *iRightSocketName)
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
resource::String Config::stereo_audio_in(char const *iLeftSocketName, char const *iRightSocketName)
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
resource::String Config::audio_out(std::string const &iSocketName)
{
  return {fmt::printf(R"(audio_outputs["%s"] = jbox.audio_output{})", iSocketName)};
}

//------------------------------------------------------------------------
// Config::audio_in
//------------------------------------------------------------------------
resource::String Config::audio_in(std::string const &iSocketName)
{
  return {fmt::printf(R"(audio_inputs["%s"] = jbox.audio_input{})", iSocketName)};
}

//------------------------------------------------------------------------
// Config::cv_out
//------------------------------------------------------------------------
resource::String Config::cv_out(char const *iSocketName)
{
  return {fmt::printf(R"(cv_outputs["%s"] = jbox.cv_output{})", iSocketName)};
}

//------------------------------------------------------------------------
// Config::cv_in
//------------------------------------------------------------------------
resource::String Config::cv_in(char const *iSocketName)
{
  return {fmt::printf(R"(cv_inputs["%s"] = jbox.cv_input{})", iSocketName)};
}

//------------------------------------------------------------------------
// Config::rtc_binding
//------------------------------------------------------------------------
resource::String Config::rtc_binding(std::string const &iSource, std::string const &iDest)
{
  return {fmt::printf(R"(rtc_bindings[#rtc_bindings + 1] = { source ="%s", dest = "%s"})", iSource, iDest)};
}

//------------------------------------------------------------------------
// Config::rt_input_setup_notify
//------------------------------------------------------------------------
resource::String Config::rt_input_setup_notify(std::string const &iPropertyName)
{
  return {fmt::printf(R"(do local _x = rt_input_setup["notify"];_x[#_x + 1] = "%s" end)", iPropertyName)};
}

//------------------------------------------------------------------------
// Config::rt_input_setup_notify_all_notes
//------------------------------------------------------------------------
resource::String Config::rt_input_setup_notify_all_notes()
{
  return rt_input_setup_notify("/note_states/*");
}


namespace impl {

//! Adds an argument to the vector of args of the form `iArgName = format(iArg)`
template<typename T>
inline void arg(char const *iArgName, char const *iFormat, T iArg, std::vector<std::string> &oArgs)
{
  oArgs.emplace_back(fmt::printf(fmt::printf("%s = %s", iArgName, iFormat), iArg));
}

//! Optionally add the property_tag argument
inline void property_tag(int iPropertyTag, std::vector<std::string> &oArgs)
{
  if(iPropertyTag > 0)
    arg("property_tag", "%d", iPropertyTag, oArgs);
}

//! Optionally add the persistence argument
inline void persistence(std::optional<lua::EPersistence> iPersistence, std::vector<std::string> &oArgs)
{
  if(iPersistence && *iPersistence != lua::EPersistence::kNone)
    arg("persistence", "\"%s\"", *iPersistence == lua::EPersistence::kPatch ? "patch" : "song", oArgs);
}

//! Transform the vector into a string properly separated with commas (none if 1 element, etc...)
inline std::string to_args(std::vector<std::string> const &iArgs) { return stl::join_to_string(iArgs); }

//! Generates the custom property string with the proper arguments
inline resource::String custom_property(std::string const &iPropertyType,
                                    std::string const &iPropertyName,
                                    char const *iJboxType,
                                    std::vector<std::string> const &iArgs)
{
  return { fmt::printf(R"(%s["%s"] = jbox.%s{ %s })", iPropertyType, iPropertyName, iJboxType, to_args(iArgs)) };
}

//! Generates the custom property string with the proper arguments
inline resource::String custom_property(std::string const &iPropertyType,
                                    int iPropertyIndex,
                                    char const *iJboxType,
                                    std::vector<std::string> const &iArgs)
{
  return { fmt::printf(R"(%s[%d] = jbox.%s{ %s })", iPropertyType, iPropertyIndex, iJboxType, to_args(iArgs)) };
}

//! custom_property for lua::jbox_boolean_property
inline resource::String custom_property(std::string const &iPropertyType,
                                        std::string const &iPropertyName,
                                        lua::jbox_boolean_property const &iProperty)
{
  std::vector<std::string> args{};
  arg("default", "%s", iProperty.fDefaultValue ? "true" : "false", args);
  property_tag(iProperty.fPropertyTag, args);
  return custom_property(iPropertyType, iPropertyName, "boolean", args);
}

//! custom_property for lua::jbox_number_property
inline resource::String custom_property(std::string const &iPropertyType,
                                        std::string const &iPropertyName,
                                        lua::jbox_number_property const &iProperty)
{
  std::vector<std::string> args{};
  arg("default", "%f", iProperty.fDefaultValue, args);
  if(iProperty.fSteps)
    arg("steps", "%d", iProperty.fSteps.value(), args);
  property_tag(iProperty.fPropertyTag, args);
  impl::persistence(iProperty.fPersistence, args);
  return custom_property(iPropertyType, iPropertyName, "number", args);
}

//! custom_property for lua::jbox_string_property
inline resource::String custom_property(std::string const &iPropertyType,
                                        std::string const &iPropertyName,
                                        lua::jbox_string_property const &iProperty)
{
  std::vector<std::string> args{};
  if(iProperty.fMaxSize > 0)
    arg("max_size", "%d", iProperty.fMaxSize, args);
  else
  {
    arg("default", "\"%s\"", iProperty.fDefaultValue, args);
    property_tag(iProperty.fPropertyTag, args);
  }
  return custom_property(iPropertyType, iPropertyName, "string", args);
}

//! custom_property for lua::jbox_blob_property
inline resource::String custom_property(std::string const &iPropertyType,
                                        std::string const &iPropertyName,
                                        lua::jbox_blob_property const &iProperty)
{
  std::vector<std::string> args{};
  if(iProperty.fDefaultValue)
    arg("default", "\"%s\"", *iProperty.fDefaultValue, args);
  property_tag(iProperty.fPropertyTag, args);
  return custom_property(iPropertyType, iPropertyName, "blob", args);
}

//! custom_property for lua::jbox_sample_property
inline resource::String custom_property(std::string const &iPropertyType,
                                    std::string const &iPropertyName,
                                    lua::jbox_sample_property const &iProperty)
{
  std::vector<std::string> args{};
  if(iProperty.fDefaultValue)
    arg("default", "\"%s\"", *iProperty.fDefaultValue, args);
  property_tag(iProperty.fPropertyTag, args);
  return custom_property(iPropertyType, iPropertyName, "sample", args);
}

}

//------------------------------------------------------------------------
// Config::gui_owner_property
//------------------------------------------------------------------------
resource::String Config::gui_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty)
{
  return impl::custom_property("gui_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::gui_owner_property
//------------------------------------------------------------------------
resource::String Config::gui_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty)
{
  return impl::custom_property("gui_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::document_owner_property
//------------------------------------------------------------------------
resource::String Config::gui_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty)
{
  return impl::custom_property("gui_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::document_owner_property
//------------------------------------------------------------------------
resource::String Config::document_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty)
{
  return impl::custom_property("document_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::document_owner_property
//------------------------------------------------------------------------
resource::String Config::document_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty)
{
  return impl::custom_property("document_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::document_owner_property
//------------------------------------------------------------------------
resource::String Config::document_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty)
{
  return impl::custom_property("document_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::user_sample
//------------------------------------------------------------------------
resource::String Config::user_sample(std::string const &iSampleName, lua::jbox_user_sample_property const &iProperty)
{
  RE_MOCK_ASSERT(iProperty.fName == std::nullopt || *iProperty.fName == iSampleName,
                 "name mismatch");
  std::vector<std::string> args{};
  if(!iProperty.fSampleParameters.empty())
  {
    std::vector<std::string> params{};
    for(auto &p: iProperty.fSampleParameters)
      params.emplace_back(fmt::printf("\"%s\"", p));
    impl::arg("sample_parameters", "{%s}", stl::join_to_string(params), args);
  }
  impl::persistence(iProperty.fPersistence, args);
  return impl::custom_property("user_samples", iSampleName, "user_sample", args);
}

//------------------------------------------------------------------------
// Config::user_sample
//------------------------------------------------------------------------
resource::String Config::user_sample(int iSampleIndex, lua::jbox_user_sample_property const &iProperty)
{
  RE_MOCK_ASSERT(iProperty.fName == std::nullopt, "no name should be defined when using this api");
  std::vector<std::string> args{};
  if(!iProperty.fSampleParameters.empty())
  {
    std::vector<std::string> params{};
    for(auto &p: iProperty.fSampleParameters)
      params.emplace_back(fmt::printf("\"%s\"", p));
    impl::arg("sample_parameters", "{%s}", stl::join_to_string(params), args);
  }
  impl::persistence(iProperty.fPersistence, args);
  return impl::custom_property("user_samples", iSampleIndex + 1, "user_sample", args);
}

//------------------------------------------------------------------------
// Config::patterns
//------------------------------------------------------------------------
resource::String Config::patterns(int iNumPatterns)
{
  return resource::String{fmt::printf("patterns = { num_patterns = %d }", iNumPatterns)};
}


//------------------------------------------------------------------------
// Config::rt_owner_property
//------------------------------------------------------------------------
resource::String Config::rt_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty)
{
  return impl::custom_property("rt_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rt_owner_property
//------------------------------------------------------------------------
resource::String Config::rt_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty)
{
  return impl::custom_property("rt_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rt_owner_property
//------------------------------------------------------------------------
resource::String Config::rt_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty)
{
  return impl::custom_property("rt_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
resource::String Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty)
{
  return impl::custom_property("rtc_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
resource::String Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty)
{
  return impl::custom_property("rtc_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
resource::String Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty)
{
  return impl::custom_property("rtc_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
resource::String Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_blob_property const &iProperty)
{
  return impl::custom_property("rtc_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
resource::String Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_sample_property const &iProperty)
{
  return impl::custom_property("rtc_owner_properties", iPropertyName, iProperty);
}

//------------------------------------------------------------------------
// Config::rtc_owner_property
//------------------------------------------------------------------------
resource::String Config::rtc_owner_property(std::string const &iPropertyName, lua::jbox_native_object const &iProperty)
{
  struct visitor {
    std::string operator()(bool v) { return (v ? "true" : "false"); }
    std::string operator()(TJBox_Float64 v) { return std::to_string(v); }
    std::string operator()(std::string v) { return fmt::printf(R"("%s")", v); }
  };

  std::vector<std::string> params{};

  auto defaultValue = std::string{};
  if(!iProperty.fDefaultValue.operation.empty())
  {
    if(!iProperty.fDefaultValue.params.empty())
    {
      for(int i = 0; i < iProperty.fDefaultValue.params.size(); i++)
        params.emplace_back(std::visit(visitor{}, iProperty.fDefaultValue.params[i]));
    }

    defaultValue = fmt::printf(R"({ "%s", { %s } })", iProperty.fDefaultValue.operation, impl::to_args(params));
  }

  std::vector<std::string> args{};
  if(!defaultValue.empty())
    impl::arg("default", "%s", defaultValue.c_str(), args);
  impl::property_tag(iProperty.fPropertyTag, args);
  return impl::custom_property("rtc_owner_properties", iPropertyName, "native_object", args);
}

//------------------------------------------------------------------------
// Config::resource_file
//------------------------------------------------------------------------
std::optional<resource::File> Config::resource_file(fs::path const &iRelativeResourcePath) const
{
  if(fDeviceResourcesDir)
  {
    return resource::File{*fDeviceResourcesDir / iRelativeResourcePath.relative_path()};
  }
  else
    return std::nullopt;
}

//------------------------------------------------------------------------
// Config::findPatchResource
//------------------------------------------------------------------------
std::unique_ptr<resource::Patch> Config::findPatchResource(std::string const &iResourcePath) const
{
  auto resourceFile = resource::File{iResourcePath};

  auto patchResource = fResources.find(iResourcePath);
  if(patchResource != fResources.end())
  {
    auto r = std::get<ConfigResource::Patch>(patchResource->second);

    // it's already a patch...
    if(std::holds_alternative<resource::Patch>(r.fPatchVariant))
      return std::make_unique<resource::Patch>(std::get<resource::Patch>(r.fPatchVariant));

    // it's a string
    if(std::holds_alternative<resource::String>(r.fPatchVariant))
      return PatchParser::from(std::get<resource::String>(r.fPatchVariant));
    else
      // it's a file
      resourceFile = std::get<resource::File>(r.fPatchVariant);
  }

  // check the path as-is
  if(FileManager::fileExists(resourceFile))
    return PatchParser::from(resourceFile);

  // resolve the path against the resource dir
  auto resolvedResource = resource_file(resourceFile.fFilePath);
  if(resolvedResource && FileManager::fileExists(*resolvedResource))
    return PatchParser::from(*resolvedResource);

  return nullptr;
}

//------------------------------------------------------------------------
// Config::findBlobResource
//------------------------------------------------------------------------
std::unique_ptr<resource::Blob> Config::findBlobResource(std::string const &iResourcePath) const
{
  auto resourceFile = resource::File{iResourcePath};

  auto blobResource = fResources.find(iResourcePath);
  if(blobResource != fResources.end())
  {
    auto b = std::get<ConfigResource::Blob>(blobResource->second);
    if(std::holds_alternative<resource::Blob>(b.fBlobVariant))
      return std::make_unique<resource::Blob>(std::get<resource::Blob>(b.fBlobVariant));
    else
      resourceFile = std::get<resource::File>(b.fBlobVariant);
  }

  // check the path as-is
  auto blob = FileManager::loadBlob(resourceFile);
  if(blob)
    return blob;

  // resolve the path against the resource dir
  auto resolvedResource = resource_file(resourceFile.fFilePath);
  if(resolvedResource && FileManager::fileExists(*resolvedResource))
    return FileManager::loadBlob(*resolvedResource);

  return nullptr;
}

//------------------------------------------------------------------------
// Config::findSampleResource
//------------------------------------------------------------------------
std::unique_ptr<resource::Sample> Config::findSampleResource(std::string const &iResourcePath) const
{
  auto resourceFile = resource::File{iResourcePath};

  auto sampleResource = fResources.find(iResourcePath);
  if(sampleResource != fResources.end())
  {
    auto s = std::get<ConfigResource::Sample>(sampleResource->second);

    if(std::holds_alternative<resource::Sample>(s.fSampleVariant))
      return std::make_unique<resource::Sample>(std::get<resource::Sample>(s.fSampleVariant));
    else
      resourceFile = std::get<resource::File>(s.fSampleVariant);
  }

  // check the path as-is
  auto sample = FileManager::loadSample(resourceFile);
  if(sample)
    return sample;

  // resolve the path against the resource dir
  auto resolvedResource = resource_file(resourceFile.fFilePath);
  if(resolvedResource)
    return FileManager::loadSample(*resolvedResource);

  return nullptr;
}

//------------------------------------------------------------------------
// checkAllowedCategory
//------------------------------------------------------------------------
void checkAllowedCategory(std::string_view iCategory)
{
  static auto kAllowedCategories = std::set<std::string>(std::begin(kDeviceCategories), std::end(kDeviceCategories));
  if(std::find(kAllowedCategories.begin(), kAllowedCategories.end(), iCategory) == kAllowedCategories.end())
  {
    RE_MOCK_ASSERT(false, "[%s] is not a valid device category", iCategory.data());
  }
}

//------------------------------------------------------------------------
// checkValidCategories
//------------------------------------------------------------------------
void checkValidCategories(std::vector<std::string> const &iCategories)
{
  RE_MOCK_ASSERT(iCategories.size() > 0, "device_categories cannot be empty");
  for(auto const &category: iCategories)
    checkAllowedCategory(category);
}

namespace impl {

//------------------------------------------------------------------------
// deviceTypeFromString
//------------------------------------------------------------------------
DeviceType deviceTypeFromString(std::string s)
{
  if(s == "instrument")
    return DeviceType::kInstrument;
  if(s == "creative_fx")
    return DeviceType::kCreativeFX;
  if(s == "studio_fx")
    return DeviceType::kStudioFX;
  if(s == "helper")
    return DeviceType::kHelper;
  if(s == "note_player")
    return DeviceType::kNotePlayer;

  RE_MOCK_ASSERT(false, "Invalid device type: %s", s);

  // not reached
  return DeviceType::kHelper;
}

//------------------------------------------------------------------------
// fromInfoLua
//------------------------------------------------------------------------
Info fromInfoLua(lua::InfoLua &iInfo)
{
  Info res{};

  res.device_type(deviceTypeFromString(iInfo.device_type()));
  res.device_categories(iInfo.device_categories());
  res.default_patch(iInfo.default_patch());
  res.fSupportPatches = iInfo.supports_patches();
  res.fAcceptNotes = iInfo.accepts_notes();
  res.fDeviceHeightRU = iInfo.device_height_ru();
  res.fLongName = iInfo.long_name();
  res.fMediumName = iInfo.medium_name();
  res.fShortName = iInfo.short_name();
  res.fProductId = iInfo.product_id();
  res.fManufacturer = iInfo.manufacturer();
  res.fVersionNumber = iInfo.version_number();
  res.fAutoCreateTrack = iInfo.auto_create_track();
  res.fAutoCreateNoteLane = iInfo.auto_create_note_lane();
  res.fSupportsPerformanceAutomation = iInfo.supports_performance_automation();

  return res;
}

}

//------------------------------------------------------------------------
// Info::info
//------------------------------------------------------------------------
Info Info::from(resource::File iFile)
{
  auto luaInfo = lua::InfoLua::fromFile(iFile.fFilePath);
  return impl::fromInfoLua(*luaInfo);
}

//------------------------------------------------------------------------
// Info::info
//------------------------------------------------------------------------
Info Info::from(resource::String iString)
{
  auto luaInfo = lua::InfoLua::fromString(iString.fString);
  return impl::fromInfoLua(*luaInfo);
}

//------------------------------------------------------------------------
// Info::fromSkeleton
//------------------------------------------------------------------------
Info Info::fromSkeleton(DeviceType iDeviceType)
{
  Info info{iDeviceType};
  return info;
}

//------------------------------------------------------------------------
// resource::LoadingContext::getStatusAsString
//------------------------------------------------------------------------
std::string resource::LoadingContext::getStatusAsString() const
{
  std::string res;

  switch(fStatus)
  {
    case LoadStatus::kNil:
      res = "nil";
      break;
    case LoadStatus::kPartiallyResident:
      res = "partially_resident";
      break;
    case LoadStatus::kResident:
      res = "resident";
      break;
    case LoadStatus::kHasErrors:
      res = "has_errors";
      break;
    case LoadStatus::kMissing:
      res = "missing";
      break;
  }

  return res;
}

//------------------------------------------------------------------------
// Sample::Metadata::getStateAsInt
//------------------------------------------------------------------------
int resource::LoadingContext::getStatusAsInt() const
{
  int res;

  switch(fStatus)
  {
    case LoadStatus::kNil:
    case LoadStatus::kHasErrors:
    case LoadStatus::kMissing:
      res = 0;
      break;
    case LoadStatus::kPartiallyResident:
      res = 1;
      break;
    case LoadStatus::kResident:
      res = 2;
      break;
  }

  return res;
}


}