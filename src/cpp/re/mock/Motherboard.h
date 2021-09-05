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
#ifndef __Pongasoft_re_mock_motherboard_h__
#define __Pongasoft_re_mock_motherboard_h__

#include <JukeboxTypes.h>
#include <Jukebox.h>
#include <memory>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <array>
#include <ostream>
#include "fmt.h"
#include "Config.h"
#include "ObjectManager.hpp"
#include "MotherboardImpl.h"
#include "lua/MotherboardDef.h"
#include "lua/RealtimeController.h"

bool operator==(TJBox_NoteEvent const &lhs, TJBox_NoteEvent const &rhs);
bool operator!=(TJBox_NoteEvent const &lhs, TJBox_NoteEvent const &rhs);
std::ostream &operator<<(std::ostream &os, TJBox_NoteEvent const &event);
static bool compare(TJBox_NoteEvent const &l, TJBox_NoteEvent const &r);

namespace re::mock {

class Rack;

class Motherboard
{
public:
  constexpr static size_t DSP_BUFFER_SIZE = 64;
  constexpr static int FIRST_MIDI_NOTE = 0;
  constexpr static int LAST_MIDI_NOTE = 127;
  using DSPBuffer = std::array<TJBox_AudioSample, DSP_BUFFER_SIZE>;
  using NoteEvents = std::vector<TJBox_NoteEvent>;

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

  std::string getRTString(std::string const &iPropertyPath) const;
  void setRTString(std::string const &iPropertyPath, std::string const &iValue);

  std::string getString(std::string const &iPropertyPath) const;
  void setString(std::string const &iPropertyPath, std::string iValue);

  inline TJBox_Float64 getCVSocketValue(std::string const &iSocketPath) const {
    return getNum(fmt::printf("%s/value", iSocketPath.c_str()));
  }
  inline void setCVSocketValue(std::string const &iSocketPath, TJBox_Float64 iValue) {
    setNum(fmt::printf("%s/value", iSocketPath.c_str()), iValue);
  }

  TJBox_OnOffBypassStates getEffectBypassState() const { return static_cast<TJBox_OnOffBypassStates>(getNum<int>("/custom_properties/builtin_onoffbypass")); }
  void setEffectBypassState(TJBox_OnOffBypassStates iState) { setNum("/custom_properties/builtin_onoffbypass", static_cast<int>(iState)); }

  bool isNotePlayerBypassed() const { return getBool("/environment/player_bypassed"); }
  void setNotePlayerBypassed(bool iBypassed) { setBool("/environment/player_bypassed", iBypassed); }

  void setNoteInEvent(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iVelocity, TJBox_UInt16 iAtFrameIndex = 0);
  inline void setNoteInEvent(TJBox_NoteEvent const &iNoteEvent) { setNoteInEvent(iNoteEvent.fNoteNumber, iNoteEvent.fVelocity, iNoteEvent.fAtFrameIndex); };
  void setNoteInEvents(NoteEvents const &iNoteEvents);

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
  void storeProperty(TJBox_PropertyRef const &iProperty, TJBox_Value const &iValue, TJBox_UInt16 iAtFrameIndex = 0);
  void storeProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag, TJBox_Value const &iValue, TJBox_UInt16 iAtFrameIndex = 0);
  void getDSPBufferData(TJBox_Value const &iValue, TJBox_AudioFramePos iStartFrame, TJBox_AudioFramePos iEndFrame, TJBox_AudioSample oAudio[]) const;
  void setDSPBufferData(TJBox_Value const &iValue, TJBox_AudioFramePos iStartFrame, TJBox_AudioFramePos iEndFrame, const TJBox_AudioSample iAudio[]);
  TJBox_DSPBufferInfo getDSPBufferInfo(TJBox_Value const &iValue) const;
  TJBox_Value makeNativeObjectRW(std::string const &iOperation, std::vector<TJBox_Value> const &iParams);
  TJBox_Value makeNativeObjectRO(std::string const &iOperation, std::vector<TJBox_Value> const &iParams);
  const void *getNativeObjectRO(TJBox_Value const &iValue) const;
  void *getNativeObjectRW(TJBox_Value const &iValue) const;
  void setRTStringData(TJBox_PropertyRef const &iProperty, TJBox_SizeT iSize, const TJBox_UInt8 iData[]);
  TJBox_UInt32 getStringLength(TJBox_Value const &iValue) const;
  void getSubstring(TJBox_Value iValue, TJBox_SizeT iStart, TJBox_SizeT iEnd, char oString[]) const;
  TJBox_NoteEvent asNoteEvent(const TJBox_PropertyDiff &iPropertyDiff);
  void outputNoteEvent(TJBox_NoteEvent const &iNoteEvent);
  NoteEvents getNoteOutEvents() const { return fNoteOutEvents; }

  Motherboard(Motherboard const &iOther) = delete;
  Motherboard &operator=(Motherboard const &iOther) = delete;

  static void copy(TJBox_Value const &iFromValue, TJBox_Value &oToValue);
  static TJBox_Value clone(TJBox_Value const &iValue);

  friend class Rack;

protected:

  static std::unique_ptr<Motherboard> create(int iInstanceId, int iSampleRate);

  Motherboard();

  void init(Config const &iConfig);

  impl::JboxObject *addObject(std::string const &iObjectPath);
  inline impl::JboxObject *getObject(std::string const &iObjectPath) const { return getObject(getObjectRef(iObjectPath)); }
  impl::JboxObject *getObject(TJBox_ObjectRef iObjectRef) const;
  impl::JboxProperty *getProperty(std::string const &iPropertyPath) const;

  void addAudioInput(std::string const &iSocketName);
  void addAudioOutput(std::string const &iSocketName);
  void addCVInput(std::string const &iSocketName);
  void addCVOutput(std::string const &iSocketName);
  void addProperty(TJBox_ObjectRef iParentObject, std::string const &iPropertyName, PropertyOwner iOwner, lua::jbox_property const &iProperty);
  void registerRTCNotify(std::string const &iPropertyPath);
  TJBox_PropertyDiff registerRTCBinding(std::string const &iPropertyPath, std::string const &iBindingName);
  void handlePropertyDiff(std::optional<TJBox_PropertyDiff> const &iPropertyDiff);
  TJBox_Value makeRTString(int iMaxSize);
  TJBox_Value makeString(std::string iValue);

  TJBox_PropertyRef getPropertyRef(std::string const &iPropertyPath) const;
  std::string getPropertyPath(TJBox_PropertyRef const &iPropertyRef) const;

  TJBox_Value createDSPBuffer();
  DSPBuffer &getDSPBuffer(TJBox_Value const &iValue);
  DSPBuffer const &getDSPBuffer(TJBox_Value const &iValue) const;
  DSPBuffer getDSPBuffer(TJBox_ObjectRef iAudioSocket) const;
  void setDSPBuffer(TJBox_ObjectRef iAudioSocket, DSPBuffer iBuffer);

  void connectSocket(TJBox_ObjectRef iSocket);
  void disconnectSocket(TJBox_ObjectRef iSocket);

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
  TJBox_ObjectRef fEnvironmentRef{};
  TJBox_ObjectRef fNoteStatesRef{};
  std::vector<TJBox_PropertyDiff> fCurrentFramePropertyDiffs{};
  ObjectManager<DSPBuffer> fDSPBuffers{};
  std::unique_ptr<lua::RealtimeController> fRealtimeController{};
  Realtime fRealtime{};
  ObjectManager<std::unique_ptr<impl::NativeObject>> fNativeObjects{};
  ObjectManager<std::string> fStrings{};
  ObjectManager<std::unique_ptr<impl::RTString>> fRTStrings{};
  std::set<TJBox_PropertyRef, ComparePropertyRef> fRTCNotify{compare};
  std::map<TJBox_PropertyRef, std::string, ComparePropertyRef> fRTCBindings{compare};
  NoteEvents fNoteOutEvents{};
};

}

#endif //__Pongasoft_re_mock_motherboard_h__