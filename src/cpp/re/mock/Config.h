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
#ifndef __PongasoftCommon_re_mock_types_h__
#define __PongasoftCommon_re_mock_types_h__

#include <stdexcept>
#include <JukeboxTypes.h>
#include "LuaJBox.h"

namespace re::mock {

enum class PropertyOwner {
  kHostOwner,
  kRTOwner,
  kRTCOwner,
  kDocOwner,
  kGUIOwner
};

struct MotherboardDef
{
  std::map<std::string, std::shared_ptr<jbox_audio_input>> audio_inputs{};
  std::map<std::string, std::shared_ptr<jbox_audio_output>> audio_outputs{};
  std::map<std::string, std::shared_ptr<jbox_cv_input>> cv_inputs{};
  std::map<std::string, std::shared_ptr<jbox_cv_output>> cv_outputs{};

  struct { std::map<std::string, std::shared_ptr<jbox_property>> properties{}; } document_owner;
  struct { std::map<std::string, std::shared_ptr<jbox_property>> properties{}; } rt_owner;
};

using RTCCallback = std::function<void (std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue)>;

struct RealtimeController
{
  std::map<std::string, std::string> rtc_bindings;
  std::map<std::string, RTCCallback> global_rtc{};
  struct { std::set<std::string> notify{}; } rt_input_setup;

  static RealtimeController byDefault();
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
  static Realtime byDefault();
};

struct Config
{
  using callback_t = std::function<void (MotherboardDef &def, RealtimeController &rtc, Realtime &rt)>;

  MotherboardDef def{};
  RealtimeController rtc{};
  Realtime rt{};

  Config() = default;

  explicit Config(callback_t iCallback)
  {
    extend(iCallback);
  }

  Config &extend(callback_t iCallback)
  {
    if(iCallback)
      iCallback(def, rtc, rt);
    return *this;
  }

  Config extendNew(callback_t iCallback) const
  {
    return Config{*this}.extend(iCallback);
  }

  template<typename T>
  static Config byDefault();

  template<typename T>
  static Config byDefault(callback_t iCallback) { return byDefault<T>().extend(std::move(iCallback)); }
};

//------------------------------------------------------------------------
// Realtime::byDefault
//------------------------------------------------------------------------
template<typename T>
Realtime Realtime::byDefault()
{
  return {
    .create_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount) -> void *{
      if(std::strcmp(iOperation, "Instance") == 0)
      {
        if(iCount >= 1)
        {
          TJBox_Float64 sampleRate = JBox_GetNumber(iParams[0]);
          return new T(static_cast<int>(sampleRate));
        }
      }

      return nullptr;
    },

    .destroy_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount, void *iNativeObject) {
      if(std::strcmp(iOperation, "Instance") == 0)
      {
        auto device = reinterpret_cast<T *>(iNativeObject);
        delete device;
      }
    },

    .render_realtime = [](void *iPrivateState, const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) {
      if(!iPrivateState)
        return;

      auto device = reinterpret_cast<T *>(iPrivateState);
      device->renderBatch(iPropertyDiffs, iDiffCount);
    }
  };
}

//------------------------------------------------------------------------
// Config::byDefault
//------------------------------------------------------------------------
template<typename T>
Config Config::byDefault()
{
  Config c{};
  c.rtc = RealtimeController::byDefault();
  c.rt = Realtime::byDefault<T>();
  return c;
}

// Error handling
struct Exception : public std::logic_error {
  Exception(std::string s) : std::logic_error(s.c_str()) {}
  Exception(char const *s) : std::logic_error(s) {}
};


}

#endif //__PongasoftCommon_re_mock_types_h__
