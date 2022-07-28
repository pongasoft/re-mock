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

#pragma once
#ifndef __Pongasoft_re_mock_types_h__
#define __Pongasoft_re_mock_types_h__

#include <JukeboxTypes.h>
#include <Jukebox.h>
#include <variant>
#include <map>
#include <set>
#include <re/mock/lua/MotherboardDef.h>
#include "Errors.h"
#include "Resources.h"

namespace re::mock {

enum class PropertyOwner {
  kHostOwner,
  kRTOwner,
  kRTCOwner,
  kDocOwner,
  kGUIOwner
};

enum class DeviceType
{
  kUnknown,
  kInstrument,
  kCreativeFX,
  kStudioFX,
  kHelper,
  kNotePlayer
};

enum class JboxObjectType : int
{
  kUnknown          = 0,
  kAudioInput       = 1 << 0,
  kAudioOutput      = 1 << 1,
  kCustomProperties = 1 << 2,
  kCVInput          = 1 << 3,
  kCVOutput         = 1 << 4,
  kDeviceHost       = 1 << 5,
  kEnvironment      = 1 << 6,
  kGlobalRTC        = 1 << 7,
  kNoteStates       = 1 << 8,
  kPatterns         = 1 << 9,
  kTransport        = 1 << 10,
  kUserSamples      = 1 << 11,
};

struct Realtime
{
  using create_native_object_t = std::function<void *(const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount)>;
  using destroy_native_object_t = std::function<void (const char iOperation[], void *iPrivateState)>;
  using render_realtime_t = std::function<void (void *iPrivateState, const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount)>;

  create_native_object_t create_native_object{};
  destroy_native_object_t destroy_native_object{};
  render_realtime_t render_realtime{};

  template<typename T>
  static create_native_object_t bySampleRateCreator(std::string iOperation = "Instance");

  template<typename T>
  static destroy_native_object_t destroyer(std::string iOperation = "Instance");

  template<typename T>
  static render_realtime_t defaultRenderRealtime();

  template<typename T>
  static Realtime byDefault();
};

struct ConfigFile { std::string fFilename{}; };
struct ConfigString { std::string fString{}; };

using ConfigSource = std::variant<resource::File, resource::String>;

struct Info
{
  DeviceType fDeviceType{DeviceType::kUnknown};
  bool fSupportPatches{};
  std::string fDefaultPatch{};
  bool fAcceptNotes{};
  int fDeviceHeightRU{1};

  Info &device_type(DeviceType t) { fDeviceType = t; return *this; }
  Info &default_patch(std::string s) { fDefaultPatch = std::move(s); fSupportPatches = !fDefaultPatch.empty(); return *this; }
  Info &accept_notes(bool b) { fAcceptNotes = b; return *this; }
  Info &device_height_ru(int i) { fDeviceHeightRU = i; return *this; }

  static Info fromSkeleton(DeviceType iDeviceType);
  static Info from(resource::File iFile);
  static Info from(resource::String iString);
  static Info from_string(std::string iString) { return from(resource::String{iString}); }
  static Info from_file(std::string iFile) { return from(resource::File{iFile}); }
};

struct Config
{
  constexpr static auto LEFT_SOCKET = "L";
  constexpr static auto RIGHT_SOCKET = "R";
  constexpr static auto SOCKET = "C";

  using rt_callback_t = std::function<void (Realtime &rt)>;

  static resource::String gui_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty);
  static resource::String gui_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty);
  static resource::String gui_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty);

  static resource::String document_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty);
  static resource::String document_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty);
  static resource::String document_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty);

  static resource::String user_sample(std::string const &iSampleName, lua::jbox_user_sample_property const &iProperty);
  static resource::String user_sample(int iSampleIndex, lua::jbox_user_sample_property const &iProperty);

  static resource::String patterns(int iNumPatterns);

  // TODO add performance properties
  static resource::String rtc_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty);
  static resource::String rtc_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty);
  static resource::String rtc_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty);
  static resource::String rtc_owner_property(std::string const &iPropertyName, lua::jbox_native_object const &iProperty);
  static resource::String rtc_owner_property(std::string const &iPropertyName, lua::jbox_blob_property const &iProperty);
  static resource::String rtc_owner_property(std::string const &iPropertyName, lua::jbox_sample_property const &iProperty);
  static resource::String rt_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty);
  static resource::String rt_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty);
  static resource::String rt_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty);

  static resource::String audio_out(std::string const &iSocketName);
  static resource::String stereo_audio_out(char const *iLeftSocketName = LEFT_SOCKET, char const *iRightSocketName = RIGHT_SOCKET);
  static resource::String audio_in(std::string const &iSocketName);
  static resource::String stereo_audio_in(char const *iLeftSocketName = LEFT_SOCKET, char const *iRightSocketName = RIGHT_SOCKET);
  static resource::String cv_out(char const *iSocketName = SOCKET);
  static resource::String cv_in(char const *iSocketName = SOCKET);
  static resource::String rtc_binding(std::string const &iSource, std::string const &iDest);
  static resource::String rt_input_setup_notify(std::string const &iPropertyName);
  static resource::String rt_input_setup_notify_all_notes();

  explicit Config(DeviceType iDeviceType) { fInfo.device_type(iDeviceType); }
  explicit Config(Info const &iInfo) :fInfo{iInfo} {}

  bool debug_config() const { return fDebugConfig; }
  Config &debug_config(bool iDebug = true) { fDebugConfig = iDebug; return *this; }

  bool traceEnabled() const { return fTraceEnabled; }
  Config &enable_trace() { fTraceEnabled = true; return *this; }
  Config &disable_trace() { fTraceEnabled = false; return *this; }

  Info const &info() const { return fInfo; }

  Config &default_patch(std::string const &s) { fInfo.default_patch(s); return *this; }
  Config &accept_notes(bool b) { fInfo.accept_notes(b); return *this; }

  std::optional<std::string> device_root_dir() const { return fDeviceRootDir; };
  Config &device_root_dir(std::string s) { fDeviceRootDir = s; return *this;}

  std::optional<std::string> device_resources_dir() const { return fDeviceResourcesDir; };
  Config &device_resources_dir(std::string s) { fDeviceResourcesDir = s; return *this; }

  std::vector<ConfigSource> const &mdef() const { return fMotherboardDefs; }
  Config &mdef(resource::File iFile) { fMotherboardDefs.emplace_back(iFile); return *this; }
  Config &mdef(resource::String iString) { fMotherboardDefs.emplace_back(iString); return *this; }
  Config &mdef_string(std::string iString) { return mdef(resource::String{iString}); }
  Config &mdef_file(std::string iFile) { return mdef(resource::File{iFile}); }

  std::vector<ConfigSource> const &rtc() const { return fRealtimeControllers; }
  Config &rtc(resource::File iFile) { fRealtimeControllers.emplace_back(iFile); return *this; }
  Config &rtc(resource::String iString) { fRealtimeControllers.emplace_back(iString); return *this; }
  Config &rtc_string(std::string iString) { return rtc(resource::String{iString}); }
  Config &rtc_file(std::string iFile) { return rtc(resource::File{iFile}); }

  rt_callback_t rt() const { return fRealtime; }
  Config &rt(rt_callback_t iCallback)
  {
    if(iCallback)
    {
      if(fRealtime)
      {
        auto previousCallback = fRealtime;
        fRealtime = [cb = std::move(previousCallback), iCallback](Realtime &rt) {
          cb(rt);
          iCallback(rt);
        };
      }
      else
        fRealtime = iCallback;
    }
    return *this;
  }

  Config clone() const { return *this; }

  template<typename T>
  Config &rt_jbox_export(std::optional<Realtime::destroy_native_object_t> iDestroyNativeObject = Realtime::destroyer<T>());

  Config& patch_string(std::string iResourcePath, std::string const &iPatchString) { fResources[iResourcePath] = ConfigResource::Patch{resource::String{iPatchString}}; return *this; }
  Config& patch_file(std::string iResourcePath, std::string const &iPatchFile) { fResources[iResourcePath] = ConfigResource::Patch{resource::File{iPatchFile}}; return *this; }
  Config& patch_data(std::string iResourcePath, resource::Patch iPatch) { fResources[iResourcePath] = ConfigResource::Patch{std::move(iPatch)}; return *this; }

  Config& blob_file(std::string iResourcePath, std::string const &iBlobFile) { fResources[iResourcePath] = ConfigResource::Blob{resource::File{iBlobFile}}; return *this; }
  Config& blob_data(std::string iResourcePath, std::vector<char> iBlobData) { fResources[iResourcePath] = ConfigResource::Blob{resource::Blob{std::move(iBlobData)}}; return *this; }

  Config& sample_file(std::string iResourcePath, std::string const &iSampleFile) { fResources[iResourcePath] = ConfigResource::Sample{resource::File{iSampleFile}}; return *this; }
  Config& sample_data(std::string iResourcePath, resource::Sample iSample) { fResources[iResourcePath] = ConfigResource::Sample{std::move(iSample)}; return *this; }

  Config& resource_loading_context(std::string iResourcePath, resource::LoadingContext iCtx) { fResourceLoadingContexts[iResourcePath] = std::move(iCtx); return *this; }

  /**
   * Returns the resource relative to `device_resources_dir()` (if device_resources_dir exists).
   *
   * @param iRelativeResourcePath must use Unix like path
   *                              (which is what the SDK uses (for example: `/Public/Default.repatch`)) */
  std::optional<resource::File> resource_file(resource::File iRelativeResourcePath) const;

  std::unique_ptr<resource::Patch> findPatchResource(std::string const &iResourcePath) const;
  std::unique_ptr<resource::Blob> findBlobResource(std::string const &iResourcePath) const;
  std::unique_ptr<resource::Sample> findSampleResource(std::string const &iResourcePath) const;

  static Config fromSkeleton(Info const &iInfo);
  static Config fromSkeleton(DeviceType iDeviceType = DeviceType::kHelper) { return fromSkeleton(Info::fromSkeleton(iDeviceType)); }

  friend class Motherboard;

  static resource::String skeletonMotherboardDef();
  static resource::String skeletonRealtimeController();

protected:
  struct ConfigResource
  {
    struct Patch { std::variant<resource::String, resource::File, resource::Patch> fPatchVariant; };
    struct Blob { std::variant<resource::File, resource::Blob> fBlobVariant; };
    struct Sample { std::variant<resource::File, resource::Sample> fSampleVariant; };
  };

  using AnyConfigResource = std::variant<ConfigResource::Patch, ConfigResource::Blob, ConfigResource::Sample>;

  bool fDebugConfig{};
  bool fTraceEnabled{true};
  Info fInfo{};
  std::optional<std::string> fDeviceRootDir{};
  std::optional<std::string> fDeviceResourcesDir{};
  std::vector<ConfigSource> fMotherboardDefs{};
  std::vector<ConfigSource> fRealtimeControllers{};
  rt_callback_t fRealtime{};
  std::map<std::string, AnyConfigResource> fResources{};
  std::map<std::string, resource::LoadingContext> fResourceLoadingContexts{};
};

template<typename T>
struct DeviceConfig
{
  using rt_callback_t = std::function<void (Realtime &rt)>;

  explicit DeviceConfig(DeviceType iDeviceType) : fConfig{iDeviceType} {}
  explicit DeviceConfig(Info const &iInfo) : fConfig{iInfo} {}

  bool debug() const { return fConfig.debug_config(); }
  Info const &info() const { return fConfig.info(); }
  std::optional<std::string> device_root_dir() const { return fConfig.device_root_dir(); };
  std::optional<std::string> device_resources_dir() const { return fConfig.device_resources_dir(); };

  std::optional<resource::File> resource_file(resource::File iRelativeResourcePath) const { return fConfig.resource_file(iRelativeResourcePath); }

  std::unique_ptr<resource::Patch> findPatchResource(std::string const &iResourcePath) const { return fConfig.findPatchResource(iResourcePath); }
  std::unique_ptr<resource::Blob> findBlobResource(std::string const &iResourcePath) const { return fConfig.findBlobResource(iResourcePath); }
  std::unique_ptr<resource::Sample> findSampleResource(std::string const &iResourcePath) const { return fConfig.findSampleResource(iResourcePath); }

  DeviceConfig &debug_config(bool iDebug = true) { fConfig.debug_config(iDebug); return *this; }

  DeviceConfig &enable_trace() { fConfig.enable_trace(); return *this; }
  DeviceConfig &disable_trace() { fConfig.disable_trace(); return *this; }

  DeviceConfig &device_root_dir(std::string s) { fConfig.device_root_dir(s); return *this;}
  DeviceConfig &device_resources_dir(std::string s) { fConfig.device_resources_dir(s); return *this;}

  DeviceConfig &default_patch(std::string s) { fConfig.default_patch(s); return *this; }
  DeviceConfig &accept_notes(bool b) { fConfig.accept_notes(b); return *this; }

  DeviceConfig &mdef(resource::File iFile) { fConfig.mdef(iFile); return *this; }
  DeviceConfig &mdef(resource::String iString) { fConfig.mdef(iString); return *this; }
  DeviceConfig &mdef_string(std::string iString) { return mdef(resource::String{iString}); }
  DeviceConfig &mdef_file(std::string iFile) { return mdef(resource::File{iFile}); }

  DeviceConfig &rtc(resource::File iFile) { fConfig.rtc(iFile); return *this; }
  DeviceConfig &rtc(resource::String iString) { fConfig.rtc(iString); return *this; }
  DeviceConfig &rtc_string(std::string iString) { return rtc(resource::String{iString}); }
  DeviceConfig &rtc_file(std::string iFile) { return rtc(resource::File{iFile}); }

  DeviceConfig &rt(rt_callback_t iCallback) { fConfig.rt(std::move(iCallback)); return *this; }

  DeviceConfig &rt_jbox_export(std::optional<Realtime::destroy_native_object_t> iDestroyNativeObject = Realtime::destroyer<T>())
  {
    fConfig.template rt_jbox_export<T>(iDestroyNativeObject);
    return *this;
  }

  DeviceConfig clone() const { return *this; }

  DeviceConfig& patch_string(std::string iResourcePath, std::string const &iPatchString) { fConfig.patch_string(iResourcePath, iPatchString); return *this; }
  DeviceConfig& patch_file(std::string iResourcePath, std::string const &iPatchFile) { fConfig.patch_file(iResourcePath, iPatchFile); return *this; }
  DeviceConfig& patch_data(std::string iResourcePath, resource::Patch iPatch) { fConfig.patch_data(iResourcePath, std::move(iPatch)); return *this; }

  DeviceConfig& blob_file(std::string iResourcePath, std::string const &iBlobFile) { fConfig.blob_file(iResourcePath, iBlobFile); return *this; }
  DeviceConfig& blob_data(std::string iResourcePath, std::vector<char> iBlobData) { fConfig.blob_data(iResourcePath, iBlobData); return *this; }

  DeviceConfig& sample_file(std::string iResourcePath, std::string const &iSampleFile) { fConfig.sample_file(iResourcePath, iSampleFile); return *this; }
  DeviceConfig& sample_data(std::string iResourcePath, resource::Sample iSample) { fConfig.sample_data(iResourcePath, std::move(iSample)); return *this; }

  DeviceConfig& resource_loading_context(std::string iResourcePath, resource::LoadingContext iCtx) { fConfig.resource_loading_context(iResourcePath, iCtx); return *this; }

  static DeviceConfig fromJBoxExport(std::string const &iDeviceRootFolder,
                                     std::optional<Realtime::destroy_native_object_t> iDestroyNativeObject = Realtime::destroyer<T>());

  static DeviceConfig fromJBoxExport(std::string const &iInfoFile,
                                     std::string const &iMotherboardDefFile,
                                     std::string const &iRealtimeControllerFile,
                                     std::optional<Realtime::destroy_native_object_t> iDestroyNativeObject = Realtime::destroyer<T>());

  static DeviceConfig fromSkeleton(Info const &iInfo);
  static DeviceConfig fromSkeleton(DeviceType iDeviceType = DeviceType::kHelper) { return fromSkeleton(Info::fromSkeleton(iDeviceType)); }

  const Config &getConfig() const { return fConfig; }

private:
  Config fConfig;
};

//------------------------------------------------------------------------
// Realtime::byDefault
//------------------------------------------------------------------------
template<typename T>
Realtime Realtime::byDefault()
{
  return {
    /* .create_native_object = */  Realtime::bySampleRateCreator<T>(),
    /* .destroy_native_object = */ Realtime::destroyer<T>(),
    /* .render_realtime = */       Realtime::defaultRenderRealtime<T>()
  };
}

//------------------------------------------------------------------------
// Realtime::destroyer
//------------------------------------------------------------------------
template<typename T>
Realtime::destroy_native_object_t Realtime::destroyer(std::string iOperation)
{
  return [operation=iOperation](const char iOperation[], void *iNativeObject) {
    if(std::strcmp(iOperation, operation.c_str()) == 0)
    {
      auto device = reinterpret_cast<T *>(iNativeObject);
      delete device;
    }
  };
}

//------------------------------------------------------------------------
// Realtime::bySampleRateCreator
//------------------------------------------------------------------------
template<typename T>
Realtime::create_native_object_t Realtime::bySampleRateCreator(std::string iOperation)
{
  return [operation=iOperation](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount) -> void *{
    if(std::strcmp(iOperation, operation.c_str()) == 0)
    {
      if(iCount >= 1)
      {
        TJBox_Float64 sampleRate = JBox_GetNumber(iParams[0]);
        return new T(static_cast<int>(sampleRate));
      }
    }

    return nullptr;
  };
}

//------------------------------------------------------------------------
// Realtime::defaultRenderRealtime
//------------------------------------------------------------------------
template<typename T>
Realtime::render_realtime_t Realtime::defaultRenderRealtime()
{
  return [](void *iPrivateState, const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) {
    if(!iPrivateState)
      return;
    auto device = reinterpret_cast<T *>(iPrivateState);
    device->renderBatch(iPropertyDiffs, iDiffCount);
  };
}

//------------------------------------------------------------------------
// Config::rt_jbox_export
//------------------------------------------------------------------------
template<typename T>
Config &Config::rt_jbox_export(std::optional<Realtime::destroy_native_object_t> iDestroyNativeObject)
{
  rt([iDestroyNativeObject](Realtime &rt) {
    rt.create_native_object = JBox_Export_CreateNativeObject;
    rt.render_realtime = JBox_Export_RenderRealtime;
    if(iDestroyNativeObject)
      rt.destroy_native_object = iDestroyNativeObject.value();
  });
  return *this;
}

//------------------------------------------------------------------------
// DeviceConfig::fromJBoxExport
//------------------------------------------------------------------------
template<typename T>
DeviceConfig<T> DeviceConfig<T>::fromJBoxExport(std::string const &iInfoFile,
                                                std::string const &iMotherboardDefFile,
                                                std::string const &iRealtimeControllerFile,
                                                std::optional<Realtime::destroy_native_object_t> iDestroyNativeObject)
{
  return DeviceConfig<T>(Info::from_file(iInfoFile))
    .mdef_file(iMotherboardDefFile)
    .rtc_file(iRealtimeControllerFile)
    .rt_jbox_export(iDestroyNativeObject);
}

//------------------------------------------------------------------------
// DeviceConfig::fromSkeleton
//------------------------------------------------------------------------
template<typename T>
DeviceConfig<T> DeviceConfig<T>::fromJBoxExport(std::string const &iDeviceRootFolder,
                                                std::optional<Realtime::destroy_native_object_t> iDestroyNativeObject)
{
  return DeviceConfig<T>(Info::from_file(fmt::path(iDeviceRootFolder, "info.lua")))
    .device_root_dir(iDeviceRootFolder)
    .device_resources_dir(fmt::path(iDeviceRootFolder, "Resources"))
    .mdef_file(fmt::path(iDeviceRootFolder, "motherboard_def.lua"))
    .rtc_file(fmt::path(iDeviceRootFolder, "realtime_controller.lua"))
    .rt_jbox_export(iDestroyNativeObject);
}

//------------------------------------------------------------------------
// DeviceConfig::fromSkeleton
//------------------------------------------------------------------------
template<typename T>
DeviceConfig<T> DeviceConfig<T>::fromSkeleton(Info const &iInfo)
{
  return DeviceConfig<T>(iInfo)
    .mdef(Config::skeletonMotherboardDef())
    .rtc(Config::skeletonRealtimeController())
    .rt([](Realtime &rt) {
      rt = Realtime::byDefault<T>();
    });
}



}

#endif //__Pongasoft_re_mock_types_h__
