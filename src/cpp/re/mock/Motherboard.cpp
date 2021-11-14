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

#include "Motherboard.h"
#include "lua/MotherboardDef.h"
#include "stl.h"
#include <Jukebox.h>
#include <algorithm>

//------------------------------------------------------------------------
// TJBox_NoteEvent::operator==
//------------------------------------------------------------------------
bool operator==(TJBox_NoteEvent const &lhs, TJBox_NoteEvent const &rhs)
{
  return lhs.fNoteNumber == rhs.fNoteNumber &&
         lhs.fVelocity == rhs.fVelocity &&
         lhs.fAtFrameIndex == rhs.fAtFrameIndex;
}

//------------------------------------------------------------------------
// TJBox_NoteEvent::operator!=
//------------------------------------------------------------------------
bool operator!=(TJBox_NoteEvent const &lhs, TJBox_NoteEvent const &rhs)
{
  return !(rhs == lhs);
}

//------------------------------------------------------------------------
// TJBox_NoteEvent::operator<<
//------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, TJBox_NoteEvent const &event)
{
  os << re::mock::fmt::printf("{.fNoteNumber=%d,.fVelocity=%d,.fAtFrameIndex=%d}",
                              event.fNoteNumber, event.fVelocity, event.fAtFrameIndex);
  return os;
}

//------------------------------------------------------------------------
// TJBox_NoteEvent::compare
//------------------------------------------------------------------------
bool compare(TJBox_NoteEvent const &l, TJBox_NoteEvent const &r)
{
  return std::tuple(l.fNoteNumber, l.fVelocity, l.fAtFrameIndex) < std::tuple(r.fNoteNumber, r.fVelocity, r.fAtFrameIndex);
}

namespace re::mock {

//------------------------------------------------------------------------
// Motherboard::Motherboard
//------------------------------------------------------------------------
Motherboard::Motherboard()
{
//  DLOG_F(INFO, "Motherboard(%p)", this);

  // /custom_properties
  fCustomPropertiesRef = addObject("/custom_properties")->fObjectRef;

  // /environment
  auto env = addObject("/environment");
  fEnvironmentRef = env->fObjectRef;

  env->addProperty("master_tune", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_EnvironmentMasterTune);
  env->addProperty("devicevisible", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_EnvironmentDeviceVisible);

//  // /custom_properties/instance (for the "privateState")
//  addProperty(fCustomPropertiesRef, "instance", PropertyOwner::kRTCOwner, lua::jbox_native_object{});

  // global_rtc
  addObject("/global_rtc");

  // note_states
  auto noteStates = addObject("/note_states");
  fNoteStatesRef = noteStates->fObjectRef;
  for(int i = FIRST_MIDI_NOTE; i <= LAST_MIDI_NOTE; i++)
  {
    noteStates->addProperty(std::to_string(i),
                            PropertyOwner::kHostOwner,
                            JBox_MakeNumber(0),
                            static_cast<TJBox_Tag>(i));
  }

  // transport
  auto transport = addObject("/transport");
  transport->addProperty("playing", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_TransportPlaying);
  transport->addProperty("play_pos", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_TransportPlayPos);
  transport->addProperty("tempo", PropertyOwner::kHostOwner, JBox_MakeNumber(120), kJBox_TransportTempo);
  transport->addProperty("filtered_tempo", PropertyOwner::kHostOwner, JBox_MakeNumber(120), kJBox_TransportFilteredTempo);
  transport->addProperty("tempo_automation", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_TransportTempoAutomation);
  transport->addProperty("time_signature_numerator", PropertyOwner::kHostOwner, JBox_MakeNumber(4), kJBox_TransportTimeSignatureNumerator);
  transport->addProperty("time_signature_denominator", PropertyOwner::kHostOwner, JBox_MakeNumber(4), kJBox_TransportTimeSignatureDenominator);
  transport->addProperty("loop_enabled", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_TransportLoopEnabled);
  transport->addProperty("loop_start_pos", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_TransportLoopStartPos);
  transport->addProperty("loop_end_pos", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_TransportLoopEndPos);
  transport->addProperty("bar_start_pos", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_TransportBarStartPos);
  transport->addProperty("request_reset_audio", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_TransportRequestResetAudio);
  transport->addProperty("request_run", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_TransportRequestRun);
  transport->addProperty("request_stop", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_TransportRequestStop);
}

//------------------------------------------------------------------------
// Motherboard::~Motherboard
//------------------------------------------------------------------------
Motherboard::~Motherboard()
{
//  DLOG_F(INFO, "~Motherboard(%p)", this);
}

//------------------------------------------------------------------------
// Motherboard::getObjectRef
//------------------------------------------------------------------------
TJBox_ObjectRef Motherboard::getObjectRef(std::string const &iObjectPath) const
{
//  DLOG_F(INFO, "Motherboard::getObjectRef(%p, %s)", this, *iObjectPath);
  auto ref = fJboxObjectRefs.find(iObjectPath);
  RE_MOCK_ASSERT(ref != fJboxObjectRefs.end(), "Could not find object [%s] (did you configure it?)", iObjectPath.c_str());
  return ref->second;
}

//------------------------------------------------------------------------
// Motherboard::getPropertyTag
//------------------------------------------------------------------------
TJBox_Tag Motherboard::getPropertyTag(TJBox_PropertyRef const &iPropertyRef) const
{
  return getObject(iPropertyRef.fObject)->getProperty(iPropertyRef.fKey)->fTag;
}

//------------------------------------------------------------------------
// Motherboard::getPropertyRef
//------------------------------------------------------------------------
TJBox_PropertyRef Motherboard::getPropertyRef(TJBox_ObjectRef iObject, TJBox_Tag iTag) const
{
  return getObject(iObject)->getProperty(iTag)->fPropertyRef;
}

//------------------------------------------------------------------------
// Motherboard::getPropertyPath
//------------------------------------------------------------------------
std::string Motherboard::getPropertyPath(TJBox_PropertyRef const &iPropertyRef) const
{
  return getObject(iPropertyRef.fObject)->getProperty(iPropertyRef.fKey)->fPropertyPath;
}

//------------------------------------------------------------------------
// Motherboard::getPropertyRef
//------------------------------------------------------------------------
TJBox_PropertyRef Motherboard::getPropertyRef(std::string const &iPropertyPath) const
{
  auto lastSlash = iPropertyPath.find_last_of('/');
  RE_MOCK_ASSERT(lastSlash != std::string::npos, "Invalid property path (missing /) [%s]", iPropertyPath.c_str());
  return JBox_MakePropertyRef(getObjectRef(iPropertyPath.substr(0, lastSlash)),
                              iPropertyPath.substr(lastSlash + 1).c_str());
}

//------------------------------------------------------------------------
// Motherboard::loadProperty
//------------------------------------------------------------------------
TJBox_Value Motherboard::loadProperty(TJBox_PropertyRef const &iProperty) const
{
  return fJboxObjects.get(iProperty.fObject)->loadValue(iProperty.fKey);
}

//------------------------------------------------------------------------
// Motherboard::loadProperty
//------------------------------------------------------------------------
TJBox_Value Motherboard::loadProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag) const
{
  return getObject(iObject)->loadValue(iTag);
}

//------------------------------------------------------------------------
// Motherboard::storeProperty
//------------------------------------------------------------------------
void Motherboard::storeProperty(TJBox_PropertyRef const &iProperty, TJBox_Value const &iValue, TJBox_UInt16 iAtFrameIndex)
{
  auto diff = fJboxObjects.get(iProperty.fObject)->storeValue(iProperty.fKey, iValue);
  if(diff)
    diff.value().fAtFrameIndex = iAtFrameIndex;
  handlePropertyDiff(diff);
}

//------------------------------------------------------------------------
// Motherboard::storeProperty
//------------------------------------------------------------------------
void Motherboard::storeProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag, TJBox_Value const &iValue, TJBox_UInt16 iAtFrameIndex)
{
  auto diff = getObject(iObject)->storeValue(iTag, iValue);
  if(diff)
    diff.value().fAtFrameIndex = iAtFrameIndex;
  handlePropertyDiff(diff);
}

//------------------------------------------------------------------------
// Motherboard::handlePropertyDiff
//------------------------------------------------------------------------
void Motherboard::handlePropertyDiff(std::optional<TJBox_PropertyDiff> const &iPropertyDiff)
{
  if(iPropertyDiff)
  {
    auto binding = fRTCBindings.find(iPropertyDiff->fPropertyRef);
    if(binding != fRTCBindings.end())
      fRealtimeController->invokeBinding(this,
                                         binding->second,
                                         getPropertyPath(iPropertyDiff->fPropertyRef),
                                         iPropertyDiff->fCurrentValue);

    if(fRTCNotify.find(iPropertyDiff->fPropertyRef) != fRTCNotify.end())
      addPropertyDiff(*iPropertyDiff);
  }
}

//------------------------------------------------------------------------
// Motherboard::create
//------------------------------------------------------------------------
std::unique_ptr<Motherboard> Motherboard::create(int iInstanceId, int iSampleRate)
{
  auto res = std::unique_ptr<Motherboard>(new Motherboard());

  // /environment/instance_id
  auto idProp = std::make_shared<lua::jbox_number_property>();
  idProp->fPropertyTag = kJBox_EnvironmentInstanceID;
  idProp->fDefaultValue = iInstanceId;
  res->addProperty(res->fEnvironmentRef, "instance_id", PropertyOwner::kHostOwner, idProp);

  // /environment/system_sample_rate
  auto sampleRateProp = std::make_shared<lua::jbox_number_property>();
  sampleRateProp->fPropertyTag = kJBox_EnvironmentSystemSampleRate;
  sampleRateProp->fDefaultValue = iSampleRate;
  res->addProperty(res->fEnvironmentRef, "system_sample_rate", PropertyOwner::kHostOwner, sampleRateProp);

  return res;
}

struct MockJBoxVisitor
{
  void operator()(ConfigString const &iString) {
    fCode += iString.fString + "\n";
    fMockJBox.loadString(iString.fString);
  }
  void operator()(ConfigFile const &iFile) { fMockJBox.loadFile(iFile.fFilename); }

  lua::MockJBox &fMockJBox;
  std::string fCode{};
};

//------------------------------------------------------------------------
// Motherboard::init
//------------------------------------------------------------------------
void Motherboard::init(Config const &iConfig)
{
  // lua::MotherboardDef
  lua::MotherboardDef def{};
  MockJBoxVisitor defVisitor{def};
  for(auto &v: iConfig.fMotherboardDefs)
    std::visit(defVisitor, v);

  if(iConfig.fDebug)
  {
    std::cout << "---- MotherboardDef -----\n";
    std::cout << defVisitor.fCode << "\n";
    std::cout << "------------------------" << std::endl;
  }

  // lua::RealtimeController
  fRealtimeController = std::make_unique<lua::RealtimeController>();
  MockJBoxVisitor rtcVisitor{*fRealtimeController};
  for(auto &v: iConfig.fRealtimeControllers)
    std::visit(rtcVisitor, v);

  if(iConfig.fDebug)
  {
    std::cout << "---- RealtimeController -----\n";
    std::cout << rtcVisitor.fCode << "\n";
    std::cout << "------------------------" << std::endl;
  }

  // Realtime
  if(iConfig.fRealtime)
    iConfig.fRealtime(fRealtime);

  // audio_inputs
  {
    auto inputs = def.getAudioInputs();
    for(auto const &input: inputs->fNames)
      addAudioInput(input);
  }

  // audio_outputs
  {
    auto outputs = def.getAudioOutputs();
    for(auto const &output: outputs->fNames)
      addAudioOutput(output);
  }

  // cv_inputs
  {
    auto inputs = def.getCVInputs();
    for(auto const &input: inputs->fNames)
      addCVInput(input);
  }

  // cv_outputs
  {
    auto outputs = def.getCVOutputs();
    for(auto const &output: outputs->fNames)
      addCVOutput(output);
  }

  auto customProperties = def.getCustomProperties();

  // gui_owner.properties
  fGUIProperties = customProperties->gui_owner;

  // document_owner.properties
  for(auto &&prop: customProperties->document_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kDocOwner, stl::variant_cast(prop.second));

  // rt_owner.properties
  for(auto &&prop: customProperties->rt_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kRTOwner, stl::variant_cast(prop.second));

  // rtc_owner.properties
  for(auto &&prop: customProperties->rtc_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kRTCOwner, stl::variant_cast(prop.second));

  // load the default patch if there is one
  if(iConfig.fDefaultPatch)
    std::visit([this](auto &patch) { loadPatch(patch); }, *iConfig.fDefaultPatch);

  // rt_input_setup.notify
  for(auto &&propertyPath: fRealtimeController->getRTInputSetupNotify())
  {
    registerRTCNotify(propertyPath);
  }

  // rtc_bindings
  for(auto const &[propertyPath, bindingKey]: fRealtimeController->getBindings())
  {
    auto diff = registerRTCBinding(propertyPath, bindingKey);
    fRealtimeController->invokeBinding(this, bindingKey, getPropertyPath(diff.fPropertyRef), diff.fCurrentValue);
  }

  // extra properties based on device type
  switch(iConfig.fDeviceType)
  {
    case DeviceType::kStudioFX:
    case DeviceType::kCreativeFX:
    {
      auto prop = std::make_shared<lua::jbox_number_property>();
      prop->fPropertyTag = kJBox_CustomPropertiesOnOffBypass;
      prop->fDefaultValue = kJBox_EnabledOn;
      addProperty(fCustomPropertiesRef, "builtin_onoffbypass", PropertyOwner::kDocOwner, prop);
      break;
    }

    case DeviceType::kNotePlayer:
    {
      auto prop = std::make_shared<lua::jbox_boolean_property>();
      prop->fPropertyTag = kJBox_EnvironmentPlayerBypassed;
      prop->fDefaultValue = false;
      addProperty(fEnvironmentRef, "player_bypassed", PropertyOwner::kHostOwner, prop);
      break;
    }

    default:
      // no extra properties
      break;
  }

}

//------------------------------------------------------------------------
// Motherboard::addProperty
//------------------------------------------------------------------------
void Motherboard::addProperty(TJBox_ObjectRef iParentObject,
                              std::string const &iPropertyName,
                              PropertyOwner iOwner,
                              lua::jbox_property const &iProperty)
{
  RE_MOCK_ASSERT(fGUIProperties.find(iPropertyName) == fGUIProperties.end(), "duplicate property [%s] (already defined in gui_owner)", iPropertyName.c_str());

  struct DefaultValueVisitor
  {
    TJBox_Value operator()(const std::shared_ptr<lua::jbox_boolean_property>& o) const { return JBox_MakeBoolean(o->fDefaultValue); }
    TJBox_Value operator()(const std::shared_ptr<lua::jbox_number_property>& o) const { return JBox_MakeNumber(o->fDefaultValue); }
    TJBox_Value operator()(const std::shared_ptr<lua::jbox_performance_property>& o) const {
      RE_MOCK_ASSERT(o->fType != lua::jbox_performance_property::Type::UNKNOWN);
      TJBox_Float64 defaultValue = 0;
      if(o->fType == lua::jbox_performance_property::Type::PITCH_BEND)
        defaultValue = 0.5;
      return JBox_MakeNumber(defaultValue);
    }
    TJBox_Value operator()(const std::shared_ptr<lua::jbox_native_object>& o) const {
      if(!o->fDefaultValue.operation.empty())
      {
        struct TJBox_Value_Visitor {
          TJBox_Value operator()(bool v) const { return JBox_MakeBoolean(v); }
          TJBox_Value operator()(TJBox_Float64 v) const { return JBox_MakeNumber(v); }
        };

        std::vector<TJBox_Value> params{};
        std::transform(o->fDefaultValue.params.begin(),
                       o->fDefaultValue.params.end(),
                       std::back_inserter(params),
                       [](auto &v) { return std::visit(TJBox_Value_Visitor{}, v); });

        return fMotherboard->makeNativeObjectRW(o->fDefaultValue.operation, params);
      }
      else
        return JBox_MakeNil();
    }
    TJBox_Value operator()(const std::shared_ptr<lua::jbox_string_property>& o) const {
      switch(fOwner)
      {
        case PropertyOwner::kRTOwner:
          RE_MOCK_ASSERT(o->fPropertyTag == 0, "[RTString] property_tag not available");
          return fMotherboard->makeRTString(o->fMaxSize);

        default:
          RE_MOCK_ASSERT(o->fMaxSize == 0, "[String] max_size not available");
          return fMotherboard->makeString(o->fDefaultValue);
      }
    }
    Motherboard *fMotherboard;
    PropertyOwner fOwner;
  };

  auto propertyTag = std::visit([](auto &p) { return p->fPropertyTag; }, iProperty);
  auto defaultValue = std::visit(DefaultValueVisitor{this, iOwner}, iProperty);
  auto persistence = std::visit([](auto &p) { return p->fPersistence; }, iProperty);

  fJboxObjects.get(iParentObject)->addProperty(iPropertyName,
                                               iOwner,
                                               defaultValue,
                                               propertyTag,
                                               *persistence);
}

//------------------------------------------------------------------------
// Motherboard::registerRTCNotify
//------------------------------------------------------------------------
void Motherboard::registerRTCNotify(std::string const &iPropertyPath)
{
  auto wildcardProperty = iPropertyPath.rfind("/*");

  if(wildcardProperty != std::string::npos)
  {
    // handle listening to ALL properties for an object
    auto objectRef = getObject(iPropertyPath.substr(0, wildcardProperty));
    auto diffs = objectRef->watchAllPropertiesForChange();
    for(auto &diff: diffs)
    {
      addPropertyDiff(diff);
      fRTCNotify.emplace(diff.fPropertyRef);
    }
  }
  else
  {
    auto ref = getPropertyRef(iPropertyPath);
    addPropertyDiff(fJboxObjects.get(ref.fObject)->watchPropertyForChange(ref.fKey));
    fRTCNotify.emplace(ref);
  }
}

//------------------------------------------------------------------------
// Motherboard::registerRTCBinding
//------------------------------------------------------------------------
TJBox_PropertyDiff Motherboard::registerRTCBinding(std::string const &iPropertyPath, std::string const &iBindingKey)
{
  auto ref = getPropertyRef(iPropertyPath);
  fRTCBindings[ref] = iBindingKey;
  return fJboxObjects.get(ref.fObject)->watchPropertyForChange(ref.fKey);
}

//------------------------------------------------------------------------
// jbox_get_dsp_id
//------------------------------------------------------------------------
inline int jbox_get_dsp_id(TJBox_Value const &iJboxValue)
{
  return impl::jbox_get_value<int>(kJBox_DSPBuffer, iJboxValue);
}

//------------------------------------------------------------------------
// Motherboard::addAudioInput
//------------------------------------------------------------------------
void Motherboard::addAudioInput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_inputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_AudioInputConnected);
  auto buffer = createDSPBuffer();
  o->addProperty("buffer", PropertyOwner::kHostOwner, buffer, kJBox_AudioInputBuffer);
  fInputDSPBuffers.emplace(jbox_get_dsp_id(buffer));
}

//------------------------------------------------------------------------
// Motherboard::addAudioOutput
//------------------------------------------------------------------------
void Motherboard::addAudioOutput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_outputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_AudioOutputConnected);
  o->addProperty("dsp_latency", PropertyOwner::kRTCOwner, JBox_MakeNumber(0), kJBox_AudioOutputDSPLatency);
  auto buffer = createDSPBuffer();
  o->addProperty("buffer", PropertyOwner::kHostOwner, buffer, kJBox_AudioOutputBuffer);
  fOutputDSPBuffers.emplace(jbox_get_dsp_id(buffer));
}

//------------------------------------------------------------------------
// Motherboard::addCVInput
//------------------------------------------------------------------------
void Motherboard::addCVInput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/cv_inputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_CVInputConnected);
  o->addProperty("value", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_CVInputValue);
}

//------------------------------------------------------------------------
// Motherboard::addCVOutput
//------------------------------------------------------------------------
void Motherboard::addCVOutput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/cv_outputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_CVOutputConnected);
  o->addProperty("dsp_latency", PropertyOwner::kRTCOwner, JBox_MakeNumber(0), kJBox_CVOutputDSPLatency);
  o->addProperty("value", PropertyOwner::kRTOwner, JBox_MakeNumber(0), kJBox_CVOutputValue);
}

//------------------------------------------------------------------------
// Motherboard::getObject
//------------------------------------------------------------------------
impl::JboxObject *Motherboard::getObject(TJBox_ObjectRef iObjectRef) const
{
  return fJboxObjects.get(iObjectRef).get();
}

//------------------------------------------------------------------------
// Motherboard::getProperty
//------------------------------------------------------------------------
impl::JboxProperty *Motherboard::getProperty(std::string const &iPropertyPath) const
{
  auto ref = getPropertyRef(iPropertyPath);
  return getObject(ref.fObject)->getProperty(ref.fKey);
}


//------------------------------------------------------------------------
// Motherboard::addObject
//------------------------------------------------------------------------
impl::JboxObject *Motherboard::addObject(std::string const &iObjectPath)
{
  auto id = fJboxObjects.add([&iObjectPath](auto id) -> auto {
    return std::make_unique<impl::JboxObject>(iObjectPath, id);
  });
  fJboxObjectRefs[iObjectPath] = id;
  return fJboxObjects.get(id).get();
}

//------------------------------------------------------------------------
// Motherboard::nextFrame
//------------------------------------------------------------------------
void Motherboard::nextFrame()
{
  // clearing note out events
  fNoteOutEvents.clear();

  // clearing output buffers (make sure that if the device doesn't do anything it is set to 0)
  for(auto id: fOutputDSPBuffers)
    fDSPBuffers.get(id).fill(0);

  if(fRealtime.render_realtime)
  {
    // The diffs are supposed to be sorted by frame index
    if(fCurrentFramePropertyDiffs.size() > 1)
    {
      std::sort(fCurrentFramePropertyDiffs.begin(),
                fCurrentFramePropertyDiffs.end(),
                [](PropertyDiff const &l, PropertyDiff const &r) {
                  if(l.fAtFrameIndex == r.fAtFrameIndex)
                    return l.fInsertIndex < r.fInsertIndex;
                  else
                    return l.fAtFrameIndex < r.fAtFrameIndex;
                });
    }

    std::vector<TJBox_PropertyDiff> diffs{};
    diffs.reserve(fCurrentFramePropertyDiffs.size());
    for(auto const &diff: fCurrentFramePropertyDiffs)
      diffs.emplace_back(diff.toJBoxPropertyDiff());

    fRealtime.render_realtime(getInstance<void *>(), diffs.data(), diffs.size());
  }

  // clearing diffs (consumed)
  fCurrentFramePropertyDiffs.clear();

  // clearing input buffers (consumed)
  for(auto id: fInputDSPBuffers)
    fDSPBuffers.get(id).fill(0);
}

//------------------------------------------------------------------------
// jbox_get_native_object_id
//------------------------------------------------------------------------
inline int jbox_get_native_object_id(TJBox_Value const &iJboxValue)
{
  return impl::jbox_get_value<int>(kJBox_NativeObject, iJboxValue);
}

//------------------------------------------------------------------------
// jbox_get_string_id
//------------------------------------------------------------------------
inline int jbox_get_string_id(TJBox_Value const &iJboxValue)
{
  return impl::jbox_get_value<int>(kJBox_String, iJboxValue);
}

//------------------------------------------------------------------------
// Motherboard::setDSPBuffer
//------------------------------------------------------------------------
void Motherboard::setDSPBuffer(std::string const &iAudioSocketPath, Motherboard::DSPBuffer iBuffer)
{
  auto id = jbox_get_dsp_id(getValue(fmt::printf("%s/buffer", iAudioSocketPath)));
  fDSPBuffers.replace(id, std::move(iBuffer));
}

//------------------------------------------------------------------------
// Motherboard::getDSPBuffer
//------------------------------------------------------------------------
Motherboard::DSPBuffer Motherboard::getDSPBuffer(std::string const &iAudioSocketPath) const
{
  auto id = jbox_get_dsp_id(getValue(fmt::printf("%s/buffer", iAudioSocketPath)));
  return fDSPBuffers.get(id);
}

//------------------------------------------------------------------------
// Motherboard::getDSPBuffer
//------------------------------------------------------------------------
Motherboard::DSPBuffer Motherboard::getDSPBuffer(TJBox_ObjectRef iAudioSocket) const
{
  auto id = jbox_get_dsp_id(fJboxObjects.get(iAudioSocket)->loadValue("buffer"));
  return fDSPBuffers.get(id);
}

//------------------------------------------------------------------------
// Motherboard::setDSPBuffer
//------------------------------------------------------------------------
void Motherboard::setDSPBuffer(TJBox_ObjectRef iAudioSocket, Motherboard::DSPBuffer iBuffer)
{
  auto id = jbox_get_dsp_id(fJboxObjects.get(iAudioSocket)->loadValue("buffer"));
  fDSPBuffers.replace(id, std::move(iBuffer));
}

//------------------------------------------------------------------------
// Motherboard::createDSPBuffer
//------------------------------------------------------------------------
TJBox_Value Motherboard::createDSPBuffer()
{
  return impl::jbox_make_value<int>(kJBox_DSPBuffer, fDSPBuffers.add(DSPBuffer{}));
}

//------------------------------------------------------------------------
// Motherboard::getDSPBuffer
//------------------------------------------------------------------------
Motherboard::DSPBuffer const &Motherboard::getDSPBuffer(TJBox_Value const &iValue) const
{
  auto id = jbox_get_dsp_id(iValue);
  return fDSPBuffers.get(id);
}

//------------------------------------------------------------------------
// Motherboard::getDSPBuffer
//------------------------------------------------------------------------
Motherboard::DSPBuffer &Motherboard::getDSPBuffer(TJBox_Value const &iValue)
{
  auto id = jbox_get_dsp_id(iValue);
  return fDSPBuffers.get(id);
}

//------------------------------------------------------------------------
// Motherboard::getDSPBufferData
//------------------------------------------------------------------------
void Motherboard::getDSPBufferData(TJBox_Value const &iValue,
                                   TJBox_AudioFramePos iStartFrame,
                                   TJBox_AudioFramePos iEndFrame,
                                   TJBox_AudioSample *oAudio) const
{
  RE_MOCK_ASSERT(iStartFrame >= 0 && iEndFrame >= 0 && iEndFrame <= DSP_BUFFER_SIZE && iStartFrame <= iEndFrame);
  auto const &buffer = getDSPBuffer(iValue);
  std::copy(std::begin(buffer) + iStartFrame, std::begin(buffer) + iEndFrame, oAudio);
}

//------------------------------------------------------------------------
// Motherboard::getDSPBufferData
//------------------------------------------------------------------------
void Motherboard::setDSPBufferData(TJBox_Value const &iValue,
                                   TJBox_AudioFramePos iStartFrame,
                                   TJBox_AudioFramePos iEndFrame,
                                   TJBox_AudioSample const *iAudio)
{
  RE_MOCK_ASSERT(iStartFrame >= 0 && iEndFrame >= 0 && iEndFrame <= DSP_BUFFER_SIZE && iStartFrame <= iEndFrame);
  auto &buffer = getDSPBuffer(iValue);
  std::copy(iAudio, iAudio + iEndFrame - iStartFrame, std::begin(buffer) + iStartFrame);
}

//------------------------------------------------------------------------
// Motherboard::getDSPBufferData
//------------------------------------------------------------------------
TJBox_DSPBufferInfo Motherboard::getDSPBufferInfo(TJBox_Value const &iValue) const
{
  getDSPBuffer(iValue); // meant to check that iValue refers to a valid dsp buffer
  return {/* .fSampleCount = */ DSP_BUFFER_SIZE};
}

//------------------------------------------------------------------------
// Motherboard::makeNativeObject
//------------------------------------------------------------------------
TJBox_Value Motherboard::makeNativeObject(std::string const &iOperation,
                                          std::vector<TJBox_Value> const &iParams,
                                          impl::NativeObject::AccessMode iAccessMode)
{
  if(fRealtime.create_native_object)
  {
    auto nativeObject = fRealtime.create_native_object(iOperation.c_str(), iParams.data(), iParams.size());
    if(nativeObject)
    {
      auto uno = std::unique_ptr<impl::NativeObject>(new impl::NativeObject{
        /* .fNativeObject = */ nativeObject,
        /* .fOperation = */    iOperation,
        /* .fParams = */       iParams,
        /* .fDeleter = */      fRealtime.destroy_native_object,
        /* .fAccessMode = */   iAccessMode
      });
      return impl::jbox_make_value<int>(kJBox_NativeObject, fNativeObjects.add(std::move(uno)));
    }
  }
  return JBox_MakeNil();
}

//------------------------------------------------------------------------
// Motherboard::makeString
//------------------------------------------------------------------------
TJBox_Value Motherboard::makeString(std::string iValue)
{
  auto s =  std::unique_ptr<impl::String>(new impl::String{
    /* .fMaxSize */ 0,
    /* .fValue */ std::move(iValue)
  });
  return impl::jbox_make_value<int>(kJBox_String, fStrings.add(std::move(s)));
}

//------------------------------------------------------------------------
// Motherboard::makeRTString
//------------------------------------------------------------------------
TJBox_Value Motherboard::makeRTString(int iMaxSize)
{
  RE_MOCK_ASSERT(iMaxSize > 0 && iMaxSize <= 2048, "RTString invalid max_size [%d]", iMaxSize);
  auto s = std::unique_ptr<impl::String>(new impl::String{
    /* .fMaxSize */ iMaxSize,
    /* .fValue */ {}
  });
  return impl::jbox_make_value<int>(kJBox_String, fStrings.add(std::move(s)));
}

//------------------------------------------------------------------------
// Motherboard::makeNativeObjectRO
//------------------------------------------------------------------------
TJBox_Value Motherboard::makeNativeObjectRO(std::string const &iOperation, std::vector<TJBox_Value> const &iParams)
{
  return makeNativeObject(iOperation, iParams, impl::NativeObject::kReadOnly);
}

//------------------------------------------------------------------------
// Motherboard::makeNativeObjectRW
//------------------------------------------------------------------------
TJBox_Value Motherboard::makeNativeObjectRW(std::string const &iOperation, std::vector<TJBox_Value> const &iParams)
{
  return makeNativeObject(iOperation, iParams, impl::NativeObject::kReadWrite);
}

//------------------------------------------------------------------------
// Motherboard::getNativeObjectRO
//------------------------------------------------------------------------
const void *Motherboard::getNativeObjectRO(TJBox_Value const &iValue) const
{
  if(JBox_GetType(iValue) == kJBox_Nil)
    return nullptr;
  else
    return fNativeObjects.get(jbox_get_native_object_id(iValue))->fNativeObject;
}

//------------------------------------------------------------------------
// Motherboard::getNativeObjectRW
//------------------------------------------------------------------------
void *Motherboard::getNativeObjectRW(TJBox_Value const &iValue) const
{
  if(JBox_GetType(iValue) == kJBox_Nil)
    return nullptr;
  else
  {
    auto &no = fNativeObjects.get(jbox_get_native_object_id(iValue));
    RE_MOCK_ASSERT(no->fAccessMode == impl::NativeObject::kReadWrite, "Trying to access RO native object in RW mode");
    return no->fNativeObject;
  }
}

//------------------------------------------------------------------------
// Motherboard::setRTStringData
//------------------------------------------------------------------------
void Motherboard::setRTStringData(TJBox_PropertyRef const &iProperty,
                                  TJBox_SizeT iSize,
                                  TJBox_UInt8 const *iData)
{
  auto property = getObject(iProperty.fObject)->getProperty(iProperty.fKey);
  RE_MOCK_ASSERT(property->fOwner == PropertyOwner::kRTOwner);
  auto &rtString = fStrings.get(jbox_get_string_id(property->loadValue()));
  RE_MOCK_ASSERT(rtString->isRTString());
  RE_MOCK_ASSERT(iSize >= 0 && iSize <= rtString->fMaxSize);
  rtString->fValue.clear();
  std::copy(iData, iData + iSize, std::back_inserter(rtString->fValue));
}

//------------------------------------------------------------------------
// Motherboard::setNoteInEvent
//------------------------------------------------------------------------
void Motherboard::setNoteInEvent(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iVelocity, TJBox_UInt16 iAtFrameIndex)
{
  RE_MOCK_ASSERT(iNoteNumber >= FIRST_MIDI_NOTE && iNoteNumber <= LAST_MIDI_NOTE);
  RE_MOCK_ASSERT(iVelocity >= 0 && iVelocity <= 127);
  RE_MOCK_ASSERT(iAtFrameIndex >= 0 && iAtFrameIndex <= 63);
  auto const &ref = getPropertyRef(fmt::printf("/note_states/%d", iNoteNumber));
  storeProperty(ref, JBox_MakeNumber(iVelocity), iAtFrameIndex);
}

//------------------------------------------------------------------------
// Motherboard::setNoteInEvents
//------------------------------------------------------------------------
void Motherboard::setNoteInEvents(Motherboard::NoteEvents const &iNoteEvents)
{
  for(auto &event: iNoteEvents)
    setNoteInEvent(event);
}

//------------------------------------------------------------------------
// Motherboard::connectSocket
//------------------------------------------------------------------------
void Motherboard::connectSocket(TJBox_ObjectRef iSocket)
{
  storeProperty(JBox_MakePropertyRef(iSocket, "connected"), JBox_MakeBoolean(true));
}

//------------------------------------------------------------------------
// Motherboard::disconnectSocket
//------------------------------------------------------------------------
void Motherboard::disconnectSocket(TJBox_ObjectRef iSocket)
{
  storeProperty(JBox_MakePropertyRef(iSocket, "connected"), JBox_MakeBoolean(false));
}

//------------------------------------------------------------------------
// Motherboard::getCVSocketValue
//------------------------------------------------------------------------
TJBox_Float64 Motherboard::getCVSocketValue(TJBox_ObjectRef iCVSocket) const
{
  return JBox_GetNumber(fJboxObjects.get(iCVSocket)->loadValue("value"));
}

//------------------------------------------------------------------------
// Motherboard::setCVSocketValue
//------------------------------------------------------------------------
void Motherboard::setCVSocketValue(TJBox_ObjectRef iCVSocket, TJBox_Float64 iValue)
{
  storeProperty(JBox_MakePropertyRef(iCVSocket, "value"), JBox_MakeNumber(iValue));
}

//------------------------------------------------------------------------
// Motherboard::toString
//------------------------------------------------------------------------
std::string Motherboard::toString(TJBox_Value const &iValue) const
{
  switch(iValue.fSecret[0])
  {
    case kJBox_Nil:
      return "Nil";

    case kJBox_Number:
      return fmt::printf("%f", JBox_GetNumber(iValue));

    case kJBox_Boolean:
      return fmt::printf("%s", JBox_GetBoolean(iValue) ? "true" : "false");

    case kJBox_DSPBuffer:
      return fmt::printf("DSPBuffer[%d]", jbox_get_dsp_id(iValue));

    case kJBox_NativeObject:
    {
      auto const id = jbox_get_native_object_id(iValue);
      auto &no = fNativeObjects.get(id);
      return fmt::printf("R%sNativeObject[%d]",
                         no->fAccessMode == impl::NativeObject::kReadWrite ? "W" : "O", id);
    }

    case kJBox_Incompatible:
      return "<incompatible>";

    case kJBox_String:
    {
      auto &s = fStrings.get(jbox_get_string_id(iValue));
      return s->fValue;
    }

    case kJBox_Sample:
    case kJBox_BLOB:
    default:
      return "<TBD>";
  }
}

//------------------------------------------------------------------------
// Motherboard::toString
//------------------------------------------------------------------------
std::string Motherboard::toString(TJBox_PropertyRef const &iPropertyRef) const
{
  return fmt::printf("%s/%s", getObjectPath(iPropertyRef.fObject), iPropertyRef.fKey);
}

//------------------------------------------------------------------------
// Motherboard::getPath
//------------------------------------------------------------------------
std::string Motherboard::getObjectPath(TJBox_ObjectRef iObjectRef) const
{
  return getObject(iObjectRef)->fObjectPath;
}

//------------------------------------------------------------------------
// Motherboard::copy
//------------------------------------------------------------------------
void Motherboard::copy(TJBox_Value const &iFromValue, TJBox_Value &oToValue)
{
  std::copy(std::begin(iFromValue.fSecret), std::end(iFromValue.fSecret), oToValue.fSecret);
}

//------------------------------------------------------------------------
// Motherboard::clone
//------------------------------------------------------------------------
TJBox_Value Motherboard::clone(TJBox_Value const &iValue)
{
  TJBox_Value v{};
  copy(iValue, v);
  return v;
}

//------------------------------------------------------------------------
// Motherboard::getRTString
//------------------------------------------------------------------------
std::string Motherboard::getRTString(std::string const &iPropertyPath) const
{
  auto property = getProperty(iPropertyPath);
  RE_MOCK_ASSERT(property->fOwner == PropertyOwner::kRTOwner);
  auto &s = fStrings.get(jbox_get_string_id(property->loadValue()));
  RE_MOCK_ASSERT(s->isRTString());
  return s->fValue;
}

//------------------------------------------------------------------------
// Motherboard::setRTString
//------------------------------------------------------------------------
void Motherboard::setRTString(std::string const &iPropertyPath, std::string const &iValue)
{
  auto property = getProperty(iPropertyPath);
  RE_MOCK_ASSERT(property->fOwner == PropertyOwner::kRTOwner);
  std::vector<TJBox_UInt8> buf{};
  buf.reserve(iValue.size());
  std::copy(iValue.begin(), iValue.end(), std::back_inserter(buf));
  setRTStringData(property->fPropertyRef, buf.size(), buf.data());
}

//------------------------------------------------------------------------
// Motherboard::getString
//------------------------------------------------------------------------
std::string Motherboard::getString(std::string const &iPropertyPath) const
{
  auto property = getProperty(iPropertyPath);
  RE_MOCK_ASSERT(property->fOwner != PropertyOwner::kRTOwner);
  auto &s = fStrings.get(jbox_get_string_id(property->loadValue()));
  RE_MOCK_ASSERT(!s->isRTString());
  return s->fValue;
}

//------------------------------------------------------------------------
// Motherboard::setString
//------------------------------------------------------------------------
void Motherboard::setString(std::string const &iPropertyPath, std::string iValue)
{
  auto property = getProperty(iPropertyPath);
  RE_MOCK_ASSERT(property->fOwner != PropertyOwner::kRTOwner);
  auto &s = fStrings.get(jbox_get_string_id(property->loadValue()));
  RE_MOCK_ASSERT(!s->isRTString());
  // Implementation note: the next line is commented out on purpose. Although this introduces a leak, it is
  // necessary to keep the old string at least until the next frame (since it can be queried via the fPreviousValue
  // field of the diff). Due to the nature of this code/framework, it is currently not an issue. A proper
  // implementation would be to do cleanup after nextFrame.
  // fStrings.remove(jbox_get_string_id(property->loadValue())); commented ON PURPOSE!!!!
  storeProperty(property->fPropertyRef, makeString(iValue));
}

//------------------------------------------------------------------------
// Motherboard::getStringLength
//------------------------------------------------------------------------
TJBox_UInt32 Motherboard::getStringLength(TJBox_Value const &iValue) const
{
  auto &s = fStrings.get(jbox_get_string_id(iValue));
  RE_MOCK_ASSERT(!s->isRTString());
  return s->fValue.size();
}

//------------------------------------------------------------------------
// Motherboard::getSubstring
//------------------------------------------------------------------------
void Motherboard::getSubstring(TJBox_Value iValue, TJBox_SizeT iStart, TJBox_SizeT iEnd, char *oString) const
{
  RE_MOCK_ASSERT(iStart <= iEnd);
  if(iStart == iEnd)
  {
    oString[0] = '\0';
    return;
  }

  auto &s = fStrings.get(jbox_get_string_id(iValue));
  RE_MOCK_ASSERT(!s->isRTString());
  RE_MOCK_ASSERT(iStart >= 0 && iStart < s->fValue.size());
  RE_MOCK_ASSERT(iEnd >= 0 && iEnd < s->fValue.size());
  std::copy(s->fValue.begin() + iStart, s->fValue.begin() + iEnd + 1, oString);
  oString[iEnd - iStart + 1] = '\0';
}

//------------------------------------------------------------------------
// Motherboard::asNoteEvent
//------------------------------------------------------------------------
TJBox_NoteEvent Motherboard::asNoteEvent(TJBox_PropertyDiff const &iPropertyDiff)
{
  RE_MOCK_ASSERT(iPropertyDiff.fPropertyRef.fObject == fNoteStatesRef);
  RE_MOCK_ASSERT(iPropertyDiff.fPropertyTag >= FIRST_MIDI_NOTE && iPropertyDiff.fPropertyTag <= LAST_MIDI_NOTE);

  return {
    /* .fNoteNumber = */   static_cast<TJBox_UInt8>(iPropertyDiff.fPropertyTag),
    /* .fVelocity = */     static_cast<TJBox_UInt8>(JBox_GetNumber(iPropertyDiff.fCurrentValue)),
    /* .fAtFrameIndex = */ iPropertyDiff.fAtFrameIndex
  };
}

//------------------------------------------------------------------------
// Motherboard::outputNoteEvent
//------------------------------------------------------------------------
void Motherboard::outputNoteEvent(TJBox_NoteEvent const &iNoteEvent)
{
  RE_MOCK_ASSERT(iNoteEvent.fNoteNumber >= FIRST_MIDI_NOTE && iNoteEvent.fNoteNumber <= LAST_MIDI_NOTE);
  RE_MOCK_ASSERT(iNoteEvent.fVelocity >= 0 && iNoteEvent.fVelocity <= 127);
  RE_MOCK_ASSERT(iNoteEvent.fAtFrameIndex >= 0 && iNoteEvent.fAtFrameIndex <= 63);

  fNoteOutEvents.emplace_back(iNoteEvent);
}

//------------------------------------------------------------------------
// Motherboard::addPropertyDiff
//------------------------------------------------------------------------
void Motherboard::addPropertyDiff(TJBox_PropertyDiff const &iDiff)
{
  PropertyDiff diff{};
  TJBox_PropertyDiff *diffPtr = &diff;
  *diffPtr = iDiff; // copy the base class
  diff.fInsertIndex = fCurrentFramePropertyDiffs.size();
  fCurrentFramePropertyDiffs.emplace_back(diff);
}

//------------------------------------------------------------------------
// Motherboard::loadPatch
//------------------------------------------------------------------------
void Motherboard::loadPatch(Patch const &iPatch)
{
  for(auto const &[propertyName, property]: iPatch.fProperties)
  {
    // it is a GUI property... ignored
    if(fGUIProperties.find(propertyName) != fGUIProperties.end())
      continue;

    auto name = fmt::printf("/custom_properties/%s", propertyName);

    RE_MOCK_ASSERT(getProperty(name)->fPersistence == lua::EPersistence::kPatch);

    struct visitor
    {
      std::string fName{};
      Motherboard *fMotherboard{};

      void operator()(patch_boolean_property const &o) { fMotherboard->setBool(fName, o.fValue); }
      void operator()(patch_number_property const &o) { fMotherboard->setNum(fName, o.fValue); }
      void operator()(patch_string_property const &o) { fMotherboard->setString(fName, o.fValue); }
    };

    std::visit(visitor{name, this}, property);
  }
}

//------------------------------------------------------------------------
// JboxObject::JboxObject
//------------------------------------------------------------------------
impl::JboxObject::JboxObject(std::string const &iObjectPath, TJBox_ObjectRef iObjectRef) :
  fObjectPath{iObjectPath}, fObjectRef{iObjectRef} {}

//------------------------------------------------------------------------
// JboxObject::getProperty
//------------------------------------------------------------------------
impl::JboxProperty *impl::JboxObject::getProperty(std::string const &iPropertyName) const
{
  RE_MOCK_ASSERT(fProperties.find(iPropertyName) != fProperties.end(), "missing property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
  return fProperties.at(iPropertyName).get();
}

//------------------------------------------------------------------------
// JboxObject::getProperty
//------------------------------------------------------------------------
impl::JboxProperty *impl::JboxObject::getProperty(TJBox_Tag iPropertyTag) const
{
  auto iter = std::find_if(fProperties.begin(),
                           fProperties.end(),
                           [iPropertyTag](auto const &p) { return p.second->fTag == iPropertyTag; } );
  RE_MOCK_ASSERT(iter != fProperties.end(), "missing property tag [%d] for object [%s]", iPropertyTag, fObjectPath.c_str());
  return iter->second.get();
}

//------------------------------------------------------------------------
// JboxObject::loadValue
//------------------------------------------------------------------------
TJBox_Value impl::JboxObject::loadValue(std::string const &iPropertyName) const
{
  return getProperty(iPropertyName)->loadValue();
}

//------------------------------------------------------------------------
// JboxObject::loadValue
//------------------------------------------------------------------------
TJBox_Value impl::JboxObject::loadValue(TJBox_Tag iPropertyTag) const
{
  return getProperty(iPropertyTag)->loadValue();
}


//------------------------------------------------------------------------
// JboxObject::storeValue
//------------------------------------------------------------------------
std::optional<TJBox_PropertyDiff> impl::JboxObject::storeValue(std::string const &iPropertyName, TJBox_Value const &iValue)
{
  return getProperty(iPropertyName)->storeValue(iValue);
}

//------------------------------------------------------------------------
// JboxObject::storeValue
//------------------------------------------------------------------------
std::optional<TJBox_PropertyDiff> impl::JboxObject::storeValue(TJBox_Tag iPropertyTag, TJBox_Value const &iValue)
{
  return getProperty(iPropertyTag)->storeValue(iValue);
}

//------------------------------------------------------------------------
// JboxObject::addProperty
//------------------------------------------------------------------------
void impl::JboxObject::addProperty(const std::string& iPropertyName,
                                   PropertyOwner iOwner,
                                   TJBox_Value const &iInitialValue,
                                   TJBox_Tag iPropertyTag,
                                   lua::EPersistence iPersistence)
{
  RE_MOCK_ASSERT(fProperties.find(iPropertyName) == fProperties.end(), "duplicate property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
  fProperties[iPropertyName] =
    std::make_unique<JboxProperty>(JBox_MakePropertyRef(fObjectRef, iPropertyName.c_str()),
                                   fmt::printf("%s/%s", fObjectPath, iPropertyName),
                                   iOwner,
                                   iInitialValue,
                                   iPropertyTag,
                                   iPersistence);
}

//------------------------------------------------------------------------
// JboxObject::watchPropertyForChange
//------------------------------------------------------------------------
TJBox_PropertyDiff impl::JboxObject::watchPropertyForChange(std::string const &iPropertyName)
{
  RE_MOCK_ASSERT(fProperties.find(iPropertyName) != fProperties.end(), "missing property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
  return fProperties[iPropertyName]->watchForChange();
}

//------------------------------------------------------------------------
// JboxObject::watchAllPropertiesForChange
//------------------------------------------------------------------------
std::vector<TJBox_PropertyDiff> impl::JboxObject::watchAllPropertiesForChange()
{
  std::vector<TJBox_PropertyDiff> res{};

  for(auto &[k, property] : fProperties)
    res.emplace_back(property->watchForChange());

  return res;
}

//------------------------------------------------------------------------
// JboxProperty::JboxProperty
//------------------------------------------------------------------------
impl::JboxProperty::JboxProperty(TJBox_PropertyRef const &iPropertyRef,
                                 std::string iPropertyPath,
                                 PropertyOwner iOwner,
                                 TJBox_Value const &iInitialValue,
                                 TJBox_Tag iTag,
                                 lua::EPersistence iPersistence) :
  fPropertyRef{iPropertyRef},
  fPropertyPath{std::move(iPropertyPath)},
  fOwner{iOwner},
  fTag{iTag},
  fInitialValue{iInitialValue},
  fValue{iInitialValue},
  fPersistence{iPersistence}
{}

//------------------------------------------------------------------------
// JboxProperty::storeValue
//------------------------------------------------------------------------
std::optional<TJBox_PropertyDiff> impl::JboxProperty::storeValue(TJBox_Value const &iValue)
{
  RE_MOCK_ASSERT(iValue.fSecret[0] == TJBox_ValueType::kJBox_Nil ||
          fValue.fSecret[0] == TJBox_ValueType::kJBox_Nil ||
          iValue.fSecret[0] == fValue.fSecret[0],
                 "invalid property type for [%s]", fPropertyPath.c_str());

  if(fWatched)
  {
    auto previousValue = fValue;
    fValue = iValue;
    return TJBox_PropertyDiff{
      /* .fPreviousValue = */ previousValue,
      /* .fCurrentValue = */  fValue,
      /* .fPropertyRef = */   fPropertyRef,
      /* .fPropertyTag = */   fTag
    };
  }
  else
  {
    fValue = iValue;
    return std::nullopt;
  }

}

//------------------------------------------------------------------------
// JboxProperty::watchForChange
//------------------------------------------------------------------------
TJBox_PropertyDiff impl::JboxProperty::watchForChange()
{
  fWatched = true;
  return TJBox_PropertyDiff{
    /* .fPreviousValue = */ fInitialValue,
    /* .fCurrentValue = */  fValue,
    /* .fPropertyRef = */   fPropertyRef,
    /* .fPropertyTag = */   fTag
  };
}

//------------------------------------------------------------------------
// Motherboard::PropertyDiff::toJBoxPropertyDiff
//------------------------------------------------------------------------
TJBox_PropertyDiff Motherboard::PropertyDiff::toJBoxPropertyDiff() const
{
  return { fPreviousValue, fCurrentValue, fPropertyRef, fPropertyTag, fAtFrameIndex };
}

}

