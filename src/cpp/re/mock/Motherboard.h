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
#ifndef __PongasoftCommon_re_mock_motherboard_h__
#define __PongasoftCommon_re_mock_motherboard_h__

#include <logging/logging.h>
#include <JukeboxTypes.h>
#include <Jukebox.h>
#include <memory>
#include <functional>
#include <map>
#include <string>
#include <stdexcept>
#include <vector>
#include <set>
#include <array>
#include "fmt.h"
#include "LuaJBox.h"
#include "ObjectManager.hpp"

namespace re::mock {

enum class PropertyOwner {
  kHostOwner,
  kRTOwner,
  kRTCOwner,
  kDocOwner,
  kGUIOwner
};

class Motherboard;

namespace impl {

template<typename T>
union JboxSecretInternal {
  T fValue;
  TJBox_UInt8 fSecret[15];
};

template<typename T>
TJBox_Value jbox_make_value(TJBox_ValueType iValueType, T iValue)
{
  TJBox_Value res;

  res.fSecret[0] = iValueType;
  JboxSecretInternal<T> secret{};
  secret.fValue = iValue;
  std::copy(std::begin(secret.fSecret), std::end(secret.fSecret), std::begin(res.fSecret) + 1);

  return res;
};

template<typename T>
T jbox_get_value(TJBox_ValueType iValueType, TJBox_Value const &iJboxValue)
{
  CHECK_F(iJboxValue.fSecret[0] == iValueType);
  JboxSecretInternal<T> secret{};
  std::copy(std::begin(iJboxValue.fSecret) + 1, std::end(iJboxValue.fSecret), std::begin(secret.fSecret));
  return secret.fValue;
}

struct JboxProperty
{
  JboxProperty(TJBox_PropertyRef const &iPropertyRef, std::string const &iPropertyPath, PropertyOwner iOwner, TJBox_Value const &iInitialValue, TJBox_Tag iTag);

  inline TJBox_Value loadValue() const { return fValue; };
  std::optional<TJBox_PropertyDiff> storeValue(TJBox_Value const &iValue);

  TJBox_PropertyDiff watchForChange();

  const TJBox_PropertyRef fPropertyRef;
  const std::string fPropertyPath;
  const PropertyOwner fOwner;
  const TJBox_Tag fTag;

protected:
  TJBox_Value fValue;
  bool fWatched{};
};

struct JboxObject
{
  explicit JboxObject(std::string const &iObjectPath, TJBox_ObjectRef iObjectRef);

  ~JboxObject() = default;

  TJBox_Value loadValue(std::string const &iPropertyName) const;
  TJBox_Value loadValue(TJBox_Tag iPropertyTag) const;
  std::optional<TJBox_PropertyDiff> storeValue(std::string const &iPropertyName, TJBox_Value const &iValue);
  std::optional<TJBox_PropertyDiff> storeValue(TJBox_Tag iPropertyTag, TJBox_Value const &iValue);
  TJBox_PropertyDiff watchPropertyForChange(std::string const &iPropertyName);

  const std::string fObjectPath;
  const TJBox_ObjectRef fObjectRef;

  friend class re::mock::Motherboard;

protected:
  void addProperty(std::string iPropertyName, PropertyOwner iOwner, TJBox_Value const &iInitialValue, TJBox_Tag iPropertyTag);
  JboxProperty *getProperty(std::string const &iPropertyName) const;
  JboxProperty *getProperty(TJBox_Tag iPropertyTag) const;

protected:
  std::map<std::string, std::unique_ptr<JboxProperty>> fProperties{};
};

}

struct MotherboardDef
{
  std::map<std::string, std::unique_ptr<jbox_audio_input>> audio_inputs{};
  std::map<std::string, std::unique_ptr<jbox_audio_output>> audio_outputs{};
  std::map<std::string, std::unique_ptr<jbox_cv_input>> cv_inputs{};
  std::map<std::string, std::unique_ptr<jbox_cv_output>> cv_outputs{};

  struct { std::map<std::string, std::unique_ptr<jbox_property>> properties{}; } document_owner;
  struct { std::map<std::string, std::unique_ptr<jbox_property>> properties{}; } rt_owner;
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
  using callback_t = std::function<void (MotherboardDef &, RealtimeController &, Realtime &)>;

  MotherboardDef def{};
  RealtimeController rtc{};
  Realtime rt{};


  static Config with(callback_t iCallback);

  template<typename T>
  static Config byDefault();

  template<typename T>
  static Config withDefault(callback_t iCallback);
};

class Rack;

class Motherboard
{
public:
  constexpr static size_t DSP_BUFFER_SIZE = 64;
  using DSPBuffer = std::array<TJBox_AudioSample, DSP_BUFFER_SIZE>;

public: // used by regular code
  ~Motherboard();

  inline int getSampleRate() const { return getNum<int>("/environment/system_sample_rate"); }

  inline TJBox_Value getValue(std::string const &iPropertyPath) const {
    return loadProperty(getPropertyRef(iPropertyPath));
  }
  inline void setValue(std::string const &iPropertyPath, TJBox_Value const &iValue) {
    storeProperty(getPropertyRef(iPropertyPath), iValue);
  }

  inline bool getBool(std::string const &iPropertyPath) const {
    return JBox_GetBoolean(getValue(iPropertyPath));
  }
  inline void setBool(std::string const &iPropertyPath, bool iValue) {
    setValue(iPropertyPath, JBox_MakeBoolean(iValue));
  }

  template<typename T = TJBox_Float64>
  T getNum(std::string const &iPropertyPath) const {
    return static_cast<T>(JBox_GetNumber(getValue(iPropertyPath)));
  }
  template<typename T = TJBox_Float64>
  void setNum(std::string const &iPropertyPath, T iValue) {
    setValue(iPropertyPath, JBox_MakeNumber(iValue));
  }

  inline TJBox_Float64 getCVSocketValue(std::string const &iSocketPath) const {
    return getNum(fmt::printf("%s/value", iSocketPath.c_str()));
  }
  inline void setCVSocketValue(std::string const &iSocketPath, TJBox_Float64 iValue) {
    setNum(fmt::printf("%s/value", iSocketPath.c_str()), iValue);
  }

  void setDSPBuffer(std::string const &iAudioSocketPath, DSPBuffer iBuffer);
  DSPBuffer getDSPBuffer(std::string const &iAudioSocketPath) const;

  template<typename T>
  inline T *getNativeOject(std::string const &iPropertyPath) const {
    return reinterpret_cast<T *>(getNativeObjectRW(getValue(iPropertyPath)));
  }

  template<typename T>
  inline T* getInstance() const {
    return getNativeOject<T>("/custom_properties/instance");
  }

public: // used by Jukebox.cpp (need to be public)
  TJBox_ObjectRef getObjectRef(std::string const &iObjectPath) const;
  TJBox_Tag getPropertyTag(TJBox_PropertyRef const &iPropertyRef) const;
  TJBox_PropertyRef getPropertyRef(TJBox_ObjectRef iObject, TJBox_Tag iTag) const;
  TJBox_Value loadProperty(TJBox_PropertyRef const &iProperty) const;
  TJBox_Value loadProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag) const;
  void storeProperty(TJBox_PropertyRef const &iProperty, TJBox_Value const &iValue);
  void storeProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag, TJBox_Value const &iValue);
  void getDSPBufferData(TJBox_Value const &iValue, TJBox_AudioFramePos iStartFrame, TJBox_AudioFramePos iEndFrame, TJBox_AudioSample oAudio[]) const;
  void setDSPBufferData(TJBox_Value const &iValue, TJBox_AudioFramePos iStartFrame, TJBox_AudioFramePos iEndFrame, const TJBox_AudioSample iAudio[]);
  TJBox_DSPBufferInfo getDSPBufferInfo(TJBox_Value const &iValue) const;
  TJBox_Value makeNativeObjectRW(std::string const &iOperation, std::vector<TJBox_Value> const &iParams);
  void *getNativeObjectRW(TJBox_Value iValue) const;

  Motherboard(Motherboard const &iOther) = delete;
  Motherboard &operator=(Motherboard const &iOther) = delete;

  friend class Rack;

protected:

  static std::unique_ptr<Motherboard> create(int iSampleRate, Config const &iConfig);

  Motherboard();

  void init();

  impl::JboxObject *addObject(std::string const &iObjectPath);
  inline impl::JboxObject *getObject(std::string const &iObjectPath) const { return getObject(getObjectRef(iObjectPath)); }
  impl::JboxObject *getObject(TJBox_ObjectRef iObjectRef) const;

  void addAudioInput(std::string const &iSocketName);
  void addAudioOutput(std::string const &iSocketName);
  void addCVInput(std::string const &iSocketName);
  void addCVOutput(std::string const &iSocketName);
  void addProperty(TJBox_ObjectRef iParentObject, std::string const &iPropertyName, PropertyOwner iOwner, jbox_property const &iProperty);
  void registerRTCNotify(std::string const &iPropertyPath);
  void registerRTCBinding(std::string const &iPropertyPath, RTCCallback iCallback);
  void handlePropertyDiff(std::optional<TJBox_PropertyDiff> const &iPropertyDiff);

  TJBox_PropertyRef getPropertyRef(std::string const &iPropertyPath) const;
  std::string getPropertyPath(TJBox_PropertyRef const &iPropertyRef) const;

  TJBox_Value createDSPBuffer();
  DSPBuffer &getDSPBuffer(TJBox_Value const &iValue);
  DSPBuffer const &getDSPBuffer(TJBox_Value const &iValue) const;
  DSPBuffer getDSPBuffer(TJBox_ObjectRef iAudioSocket) const;
  void setDSPBuffer(TJBox_ObjectRef iAudioSocket, DSPBuffer iBuffer);

  void connectSocket(TJBox_ObjectRef iSocket);

  TJBox_Float64 getCVSocketValue(TJBox_ObjectRef iCVSocket) const;
  void setCVSocketValue(TJBox_ObjectRef iCVSocket, TJBox_Float64 iValue);

  void nextFrame();

protected:

  static bool compare(TJBox_PropertyRef const &l, TJBox_PropertyRef const &r)
  {
    if(l.fObject == r.fObject)
      return strncmp(l.fKey, r.fKey, kJBox_MaxPropertyNameLen + 1);
    else
      return l.fObject < r.fObject;
  }

  using ComparePropertyRef = decltype(&compare);

  struct NativeObject
  {
    void *fNativeObject{};
    std::string fOperation{};
    std::vector<TJBox_Value> fParams{};
    Realtime::destroy_native_object_t fDeleter{};

    ~NativeObject();
  };

protected:
  ObjectManager<std::unique_ptr<impl::JboxObject>> fJboxObjects{};
  std::map<std::string, TJBox_ObjectRef> fJboxObjectRefs{};
  TJBox_ObjectRef fCustomPropertiesRef{};
  std::vector<TJBox_PropertyDiff> fInitBindings{};
  std::vector<TJBox_PropertyDiff> fCurrentFramePropertyDiffs{};
  ObjectManager<DSPBuffer> fDSPBuffers{};
  Realtime fRealtime{};
  ObjectManager<std::unique_ptr<NativeObject>> fNativeObjects{};
  std::set<TJBox_PropertyRef, ComparePropertyRef> fRTCNotify{compare};
  std::map<TJBox_PropertyRef, RTCCallback, ComparePropertyRef> fRTCBindings{compare};
};

// Error handling
struct Error : public std::logic_error {
  Error(std::string s) : std::logic_error(s.c_str()) {}
  Error(char const *s) : std::logic_error(s) {}
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

//------------------------------------------------------------------------
// Config::withDefault
//------------------------------------------------------------------------
template<typename T>
Config Config::withDefault(callback_t iCallback)
{
  Config c = Config::byDefault<T>();
  if(iCallback)
    iCallback(c.def, c.rtc, c.rt);
  return c;
}

}

#endif //__PongasoftCommon_re_mock_motherboard_h__