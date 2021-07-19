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
#include <vector>
#include <set>
#include <array>
#include "fmt.h"
#include "LuaJBox.h"
#include "Config.h"
#include "ObjectManager.hpp"
#include "MotherboardImpl.h"

namespace re::mock {

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
  inline T *getNativeObjectRW(std::string const &iPropertyPath) const {
    return reinterpret_cast<T *>(getNativeObjectRW(getValue(iPropertyPath)));
  }

  template<typename T>
  inline const T *getNativeObjectRO(std::string const &iPropertyPath) const {
    return reinterpret_cast<T const *>(getNativeObjectRO(getValue(iPropertyPath)));
  }

  template<typename T>
  inline T* getInstance() const {
    return getNativeObjectRW<T>("/custom_properties/instance");
  }

  std::string toString(TJBox_Value const &iValue) const;
  std::string toString(std::string const &iPropertyPath) const { return toString(getValue(iPropertyPath)); }
  std::string toString(TJBox_PropertyRef const &iPropertyRef) const;
  std::string getObjectPath(TJBox_ObjectRef iObjectRef) const;

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
  TJBox_Value makeNativeObjectRO(std::string const &iOperation, std::vector<TJBox_Value> const &iParams);
  const void *getNativeObjectRO(TJBox_Value const &iValue) const;
  void *getNativeObjectRW(TJBox_Value const &iValue) const;

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

  TJBox_Value makeNativeObject(std::string const &iOperation,
                               std::vector<TJBox_Value> const &iParams,
                               impl::NativeObject::AccessMode iAccessMode);

  void nextFrame();

protected:

  static bool compare(TJBox_PropertyRef const &l, TJBox_PropertyRef const &r)
  {
    if(l.fObject == r.fObject)
      return strncmp(l.fKey, r.fKey, kJBox_MaxPropertyNameLen + 1) < 0;
    else
      return l.fObject < r.fObject;
  }

  using ComparePropertyRef = decltype(&compare);

protected:
  ObjectManager<std::unique_ptr<impl::JboxObject>> fJboxObjects{};
  std::map<std::string, TJBox_ObjectRef> fJboxObjectRefs{};
  TJBox_ObjectRef fCustomPropertiesRef{};
  std::vector<TJBox_PropertyDiff> fInitBindings{};
  std::vector<TJBox_PropertyDiff> fCurrentFramePropertyDiffs{};
  ObjectManager<DSPBuffer> fDSPBuffers{};
  Realtime fRealtime{};
  ObjectManager<std::unique_ptr<impl::NativeObject>> fNativeObjects{};
  std::set<TJBox_PropertyRef, ComparePropertyRef> fRTCNotify{compare};
  std::map<TJBox_PropertyRef, RTCCallback, ComparePropertyRef> fRTCBindings{compare};
};

}

#endif //__PongasoftCommon_re_mock_motherboard_h__