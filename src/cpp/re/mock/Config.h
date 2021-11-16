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

struct Realtime
{
  using create_native_object_t = std::function<void *(const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount)>;
  using destroy_native_object_t = std::function<void (const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount, void *iPrivateState)>;
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

using ConfigSource = std::variant<ConfigFile, ConfigString>;

struct Info
{
  DeviceType fDeviceType{DeviceType::kUnknown};
  bool fSupportPatches{};
  std::optional<ConfigSource> fDefaultPatch{};

  Info &device_type(DeviceType t) { fDeviceType = t; return *this; }
  Info &default_patch(ConfigSource s);

  static Info fromSkeleton(DeviceType iDeviceType);
  static Info from(ConfigFile iFile);
  static Info from(ConfigString iString);
  static Info from_string(std::string iString) { return from(ConfigString{iString}); }
  static Info from_file(std::string iFile) { return from(ConfigFile{iFile}); }
};

struct Config
{
  constexpr static auto LEFT_SOCKET = "L";
  constexpr static auto RIGHT_SOCKET = "R";
  constexpr static auto SOCKET = "C";

  using rt_callback_t = std::function<void (Realtime &rt)>;

  static ConfigString gui_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty);
  static ConfigString gui_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty);
  static ConfigString gui_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty);

  static ConfigString document_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty);
  static ConfigString document_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty);
  static ConfigString document_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty);
  // TODO add performance properties
  static ConfigString rtc_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty);
  static ConfigString rtc_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty);
  static ConfigString rtc_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty);
  static ConfigString rtc_owner_property(std::string const &iPropertyName, lua::jbox_native_object const &iProperty);
  static ConfigString rt_owner_property(std::string const &iPropertyName, lua::jbox_boolean_property const &iProperty);
  static ConfigString rt_owner_property(std::string const &iPropertyName, lua::jbox_number_property const &iProperty);
  static ConfigString rt_owner_property(std::string const &iPropertyName, lua::jbox_string_property const &iProperty);

  static ConfigString audio_out(std::string const &iSocketName);
  static ConfigString stereo_audio_out(char const *iLeftSocketName = LEFT_SOCKET, char const *iRightSocketName = RIGHT_SOCKET);
  static ConfigString audio_in(std::string const &iSocketName);
  static ConfigString stereo_audio_in(char const *iLeftSocketName = LEFT_SOCKET, char const *iRightSocketName = RIGHT_SOCKET);
  static ConfigString cv_out(char const *iSocketName = SOCKET);
  static ConfigString cv_in(char const *iSocketName = SOCKET);
  static ConfigString rtc_binding(std::string const &iSource, std::string const &iDest);
  static ConfigString rt_input_setup_notify(std::string const &iPropertyName);
  static ConfigString rt_input_setup_notify_all_notes();

  explicit Config(DeviceType iDeviceType) { fInfo.device_type(iDeviceType); }
  explicit Config(Info const &iInfo) :fInfo{iInfo} {}

  bool debug() const { return fDebug; }
  Config &debug(bool iDebug = true) { fDebug = iDebug; return *this; }

  Info const &info() const { return fInfo; }

  Config &default_patch(ConfigSource s) { fInfo.default_patch(s); return *this; }

  std::optional<std::string> device_root_dir() const { return fDeviceRootDir; };
  Config &device_root_dir(std::string s) { fDeviceRootDir = s; return *this;}

  std::optional<std::string> device_resources_dir() const { return fDeviceResourcesDir; };
  Config &device_resources_dir(std::string s) { fDeviceResourcesDir = s; return *this; }

  /**
   * Returns the resource relative to `device_resources_dir()` (if device_resources_dir exists).
   *
   * @param iRelativeResourcePath must use Unix like path
   *                              (which is what the SDK uses (for example: `/Public/Default.repatch`)) */
  std::optional<ConfigFile> resource_file(ConfigFile iRelativeResourcePath) const;
  inline std::optional<ConfigFile> resource_file(std::string iRelativeResourcePath) const { return resource_file(ConfigFile{iRelativeResourcePath}); }

  std::vector<ConfigSource> const &mdef() const { return fMotherboardDefs; }
  Config &mdef(ConfigFile iFile) { fMotherboardDefs.emplace_back(iFile); return *this; }
  Config &mdef(ConfigString iString) { fMotherboardDefs.emplace_back(iString); return *this; }
  Config &mdef_string(std::string iString) { return mdef(ConfigString{iString}); }
  Config &mdef_file(std::string iFile) { return mdef(ConfigFile{iFile}); }

  std::vector<ConfigSource> const &rtc() const { return fRealtimeControllers; }
  Config &rtc(ConfigFile iFile) { fRealtimeControllers.emplace_back(iFile); return *this; }
  Config &rtc(ConfigString iString) { fRealtimeControllers.emplace_back(iString); return *this; }
  Config &rtc_string(std::string iString) { return rtc(ConfigString{iString}); }
  Config &rtc_file(std::string iFile) { return rtc(ConfigFile{iFile}); }

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

  static Config fromSkeleton(Info const &iInfo);
  static Config fromSkeleton(DeviceType iDeviceType = DeviceType::kHelper) { return fromSkeleton(Info::fromSkeleton(iDeviceType)); }

  friend class Motherboard;

  static ConfigString skeletonMotherboardDef();
  static ConfigString skeletonRealtimeController();


protected:
  bool fDebug{};
  Info fInfo{};
  std::optional<std::string> fDeviceRootDir{};
  std::optional<std::string> fDeviceResourcesDir{};
  std::vector<ConfigSource> fMotherboardDefs{};
  std::vector<ConfigSource> fRealtimeControllers{};
  rt_callback_t fRealtime{};
};

template<typename T>
struct DeviceConfig
{
  using rt_callback_t = std::function<void (Realtime &rt)>;

  explicit DeviceConfig(DeviceType iDeviceType) : fConfig{iDeviceType} {}
  explicit DeviceConfig(Info const &iInfo) : fConfig{iInfo} {}

  bool debug() const { return fConfig.debug(); }
  Info const &info() const { return fConfig.info(); }
  std::optional<std::string> device_root_dir() const { return fConfig.device_root_dir(); };
  std::optional<std::string> device_resources_dir() const { return fConfig.device_resources_dir(); };
  std::optional<ConfigFile> resource_file(ConfigFile iRelativeResourcePath) const { return fConfig.resource_file(iRelativeResourcePath); }

  DeviceConfig &debug(bool iDebug = true) { fConfig.debug(iDebug); return *this; }

  DeviceConfig &default_patch(ConfigSource s) { fConfig.default_patch(s); return *this; }
  DeviceConfig &device_root_dir(std::string s) { fConfig.device_root_dir(s); return *this;}
  DeviceConfig &device_resources_dir(std::string s) { fConfig.device_resources_dir(s); return *this;}

  DeviceConfig &mdef(ConfigFile iFile) { fConfig.mdef(iFile); return *this; }
  DeviceConfig &mdef(ConfigString iString) { fConfig.mdef(iString); return *this; }
  DeviceConfig &mdef_string(std::string iString) { return mdef(ConfigString{iString}); }
  DeviceConfig &mdef_file(std::string iFile) { return mdef(ConfigFile{iFile}); }

  DeviceConfig &rtc(ConfigFile iFile) { fConfig.rtc(iFile); return *this; }
  DeviceConfig &rtc(ConfigString iString) { fConfig.rtc(iString); return *this; }
  DeviceConfig &rtc_string(std::string iString) { return rtc(ConfigString{iString}); }
  DeviceConfig &rtc_file(std::string iFile) { return rtc(ConfigFile{iFile}); }

  DeviceConfig &rt(rt_callback_t iCallback) { fConfig.rt(std::move(iCallback)); return *this; }

  DeviceConfig &rt_jbox_export(std::optional<Realtime::destroy_native_object_t> iDestroyNativeObject = Realtime::destroyer<T>())
  {
    fConfig.template rt_jbox_export<T>(iDestroyNativeObject);
    return *this;
  }

  DeviceConfig clone() const { return *this; }

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
  return [operation=iOperation](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount, void *iNativeObject) {
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
