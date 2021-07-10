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

namespace re::mock {

enum class PropertyOwner {
  kHostOwner,
  kRTOwner,
  kRTCOwner,
  kDocOwner,
  kGUIOwner
};

class Motherboard;

template<typename T>
int emplace_back(std::vector<T> &iVector, T &&iElement)
{
  auto size = iVector.size();
  iVector.emplace_back(std::forward<T>(iElement));
  CHECK_F(size + 1 == iVector.size());
  return size;
}

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
  explicit JboxObject(std::string const &iObjectPath);

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

struct RealtimeController
{
  std::map<std::string, std::string> rtc_bindings;
  std::map<std::string, std::function<void (std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue)>> global_rtc{};
  struct { std::set<std::string> notify{}; } rt_input_setup;
};

struct Realtime
{
  struct NativeObject
  {
    NativeObject(void *iObject, std::function<void(void*)> iDestructor = {}) : fObject{iObject}, fDestructor{std::move(iDestructor)} {}
    ~NativeObject() { if(fDestructor) fDestructor(fObject); }
    NativeObject(NativeObject const &) = delete;
    NativeObject(NativeObject &&) = delete;

    friend class Motherboard;

  private:
    void *fObject{};
    std::function<void(void*)> fDestructor{};
  };

  template<typename T>
  static inline std::unique_ptr<NativeObject> make_native_object(T *t) {
    return std::make_unique<NativeObject>(t, [](void *o) {
      std::default_delete<T>()(reinterpret_cast<T *>(o));
    });
  }

  std::function<std::unique_ptr<NativeObject> (std::string const &iOperation, std::vector<TJBox_Value> const &iParams)> create_native_object{};
  std::function<void (void *iPrivateState, std::vector<TJBox_PropertyDiff> const &iPropertyDiffs)> render_realtime;
};

class Motherboard
{
public:
  constexpr static size_t DSP_BUFFER_SIZE = 64;
  using DSPBuffer = std::array<TJBox_AudioSample, DSP_BUFFER_SIZE>;

public: // used by regular code
  static std::unique_ptr<Motherboard> init(std::function<void (MotherboardDef &, RealtimeController &, Realtime&)> iConfigFunction);

  ~Motherboard();

  void nextFrame(std::function<void(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount)> iNextFrameCallback);

  void connectSocket(std::string const &iSocketPath);

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

  void setDSPBuffer(std::string const &iAudioSocketPath, DSPBuffer const &iBuffer);
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

protected:

  Motherboard();

  impl::JboxObject *addObject(std::string const &iObjectPath);
  inline impl::JboxObject *getObject(std::string const &iObjectPath) const { return getObject(getObjectRef(iObjectPath)); }
  impl::JboxObject *getObject(TJBox_ObjectRef iObjectRef) const;

  void addAudioInput(std::string const &iSocketName);
  void addAudioOutput(std::string const &iSocketName);
  void addCVInput(std::string const &iSocketName);
  void addCVOutput(std::string const &iSocketName);
  void addProperty(TJBox_ObjectRef iParentObject, std::string const &iPropertyName, PropertyOwner iOwner, jbox_property const &iProperty);
  void registerNotifiableProperty(std::string const &iPropertyPath);

  TJBox_PropertyRef getPropertyRef(std::string const &iPropertyPath) const;

  TJBox_Value createDSPBuffer();
  DSPBuffer &getDSPBuffer(TJBox_Value const &iValue);
  DSPBuffer const &getDSPBuffer(TJBox_Value const &iValue) const;

protected:
  std::vector<std::unique_ptr<impl::JboxObject>> fJboxObjects{};
  std::map<std::string, TJBox_ObjectRef> fJboxObjectRefs{};
  TJBox_ObjectRef fCustomPropertiesRef{};
  std::vector<TJBox_PropertyDiff> fCurrentFramePropertyDiffs{};
  std::vector<DSPBuffer> fDSPBuffers{};
  Realtime fRealtime{};
  std::vector<std::unique_ptr<Realtime::NativeObject>> fNativeObjects{};
};

// Error handling
struct Error : public std::logic_error {
  Error(std::string s) : std::logic_error(s.c_str()) {}
  Error(char const *s) : std::logic_error(s) {}
};

}

#endif //__PongasoftCommon_re_mock_motherboard_h__