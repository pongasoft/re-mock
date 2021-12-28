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
// getValueType (similar to JBox_GetType but without copy!)
//------------------------------------------------------------------------
constexpr TJBox_ValueType getValueType(TJBox_Value const &iValue)
{
  return static_cast<TJBox_ValueType>(iValue.fSecret[0]);
}

//------------------------------------------------------------------------
// Motherboard::Motherboard
//------------------------------------------------------------------------
Motherboard::Motherboard(Config const &iConfig) : fConfig{iConfig}
{
//  DLOG_F(INFO, "Motherboard(%p)", this);

  fResourceLoadingContexts = fConfig.fResourceLoadingContexts;

  // /custom_properties
  fCustomPropertiesRef = addObject("/custom_properties")->fObjectRef;

  // /environment
  auto env = addObject("/environment");
  fEnvironmentRef = env->fObjectRef;

  env->addProperty("master_tune", PropertyOwner::kHostOwner, makeNumber(0), kJBox_EnvironmentMasterTune);
  env->addProperty("devicevisible", PropertyOwner::kHostOwner, makeBoolean(false), kJBox_EnvironmentDeviceVisible);

//  // /custom_properties/instance (for the "privateState")
//  addProperty(fCustomPropertiesRef, "instance", PropertyOwner::kRTCOwner, lua::jbox_native_object{});

  // global_rtc
  addObject("/global_rtc");

  if(iConfig.info().fAcceptNotes || iConfig.info().fDeviceType == DeviceType::kNotePlayer)
  {
    // note_states
    auto noteStates = addObject("/note_states");
    fNoteStatesRef = noteStates->fObjectRef;
    for(int i = FIRST_MIDI_NOTE; i <= LAST_MIDI_NOTE; i++)
    {
      noteStates->addProperty(std::to_string(i),
                              PropertyOwner::kHostOwner,
                              makeNumber(0),
                              static_cast<TJBox_Tag>(i));
    }
  }

  // transport
  auto transport = addObject("/transport");
  transport->addProperty("playing", PropertyOwner::kHostOwner, makeBoolean(false), kJBox_TransportPlaying);
  transport->addProperty("play_pos", PropertyOwner::kHostOwner, makeNumber(0), kJBox_TransportPlayPos);
  transport->addProperty("tempo", PropertyOwner::kHostOwner, makeNumber(120), kJBox_TransportTempo);
  transport->addProperty("filtered_tempo", PropertyOwner::kHostOwner, makeNumber(120), kJBox_TransportFilteredTempo);
  transport->addProperty("tempo_automation", PropertyOwner::kHostOwner, makeBoolean(false), kJBox_TransportTempoAutomation);
  transport->addProperty("time_signature_numerator", PropertyOwner::kHostOwner, makeNumber(4), kJBox_TransportTimeSignatureNumerator);
  transport->addProperty("time_signature_denominator", PropertyOwner::kHostOwner, makeNumber(4), kJBox_TransportTimeSignatureDenominator);
  transport->addProperty("loop_enabled", PropertyOwner::kHostOwner, makeBoolean(false), kJBox_TransportLoopEnabled);
  transport->addProperty("loop_start_pos", PropertyOwner::kHostOwner, makeNumber(0), kJBox_TransportLoopStartPos);
  transport->addProperty("loop_end_pos", PropertyOwner::kHostOwner, makeNumber(0), kJBox_TransportLoopEndPos);
  transport->addProperty("bar_start_pos", PropertyOwner::kHostOwner, makeNumber(0), kJBox_TransportBarStartPos);
  transport->addProperty("request_reset_audio", PropertyOwner::kHostOwner, makeNumber(0), kJBox_TransportRequestResetAudio);
  transport->addProperty("request_run", PropertyOwner::kHostOwner, makeNumber(0), kJBox_TransportRequestRun);
  transport->addProperty("request_stop", PropertyOwner::kHostOwner, makeNumber(0), kJBox_TransportRequestStop);
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
  RE_MOCK_ASSERT(ref != fJboxObjectRefs.end(), "Could not find object [%s] (did you configure it?)", iObjectPath);
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
  RE_MOCK_ASSERT(lastSlash != std::string::npos, "Invalid property path (missing /) [%s]", iPropertyPath);
  return JBox_MakePropertyRef(getObjectRef(iPropertyPath.substr(0, lastSlash)),
                              iPropertyPath.substr(lastSlash + 1).c_str());
}

//------------------------------------------------------------------------
// Motherboard::getJboxValue
//------------------------------------------------------------------------
std::shared_ptr<const JboxValue> Motherboard::getJboxValue(std::string const &iPropertyPath) const
{
  auto ref = getPropertyRef(iPropertyPath);
  return fJboxObjects.get(ref.fObject)->loadValue(ref.fKey);
}

//------------------------------------------------------------------------
// Motherboard::getJboxValue
//------------------------------------------------------------------------
std::shared_ptr<JboxValue> Motherboard::getJboxValue(std::string const &iPropertyPath)
{
  auto ref = getPropertyRef(iPropertyPath);
  return fJboxObjects.get(ref.fObject)->loadValue(ref.fKey);
}

//------------------------------------------------------------------------
// Motherboard::to_TJBox_Value
//------------------------------------------------------------------------
TJBox_Value Motherboard::to_TJBox_Value(std::shared_ptr<const JboxValue> const &iValue) const
{
  switch(iValue->getValueType())
  {
    case kJBox_Nil:
      return JBox_MakeNil();

    case kJBox_Number:
      return JBox_MakeNumber(iValue->getNumber());

    case kJBox_Boolean:
      return JBox_MakeBoolean(iValue->getBoolean());

    case kJBox_Incompatible:
      return impl::jbox_make_value(kJBox_Incompatible, 0);

    default:
    {
      fCurrentValues[iValue->getUniqueId()] = iValue;
      return impl::jbox_make_value(iValue->getValueType(), iValue->getUniqueId());
    }
  }
}

//------------------------------------------------------------------------
// Motherboard::to_TJBox_Value
//------------------------------------------------------------------------
std::shared_ptr<const JboxValue> Motherboard::from_TJBox_Value(TJBox_Value const &iValue) const
{
  switch(getValueType(iValue))
  {
    case kJBox_Nil:
      return makeNil();

    case kJBox_Number:
      return makeNumber(JBox_GetNumber(iValue));

    case kJBox_Boolean:
      return makeBoolean(JBox_GetBoolean(iValue));
      break;

    case kJBox_Incompatible:
      return makeIncompatible();

    default:
    {
      auto uniqueId = impl::jbox_get_value<TJBox_UInt64>(getValueType(iValue), iValue);
      auto res = fCurrentValues.find(uniqueId);
      RE_MOCK_ASSERT(res != fCurrentValues.end(), "Could not find current value of type %d", getValueType(iValue));
      return res->second;
    }
  }
}

//------------------------------------------------------------------------
// Motherboard::loadProperty
//------------------------------------------------------------------------
TJBox_Value Motherboard::loadProperty(TJBox_PropertyRef const &iProperty) const
{
  return to_TJBox_Value(fJboxObjects.get(iProperty.fObject)->loadValue(iProperty.fKey));
}

//------------------------------------------------------------------------
// Motherboard::loadProperty
//------------------------------------------------------------------------
TJBox_Value Motherboard::loadProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag) const
{
  return to_TJBox_Value(getObject(iObject)->loadValue(iTag));
}

//------------------------------------------------------------------------
// Motherboard::storeProperty
//------------------------------------------------------------------------
void Motherboard::storeProperty(TJBox_PropertyRef const &iProperty, std::shared_ptr<const JboxValue> const &iValue, TJBox_UInt16 iAtFrameIndex)
{
  auto property = fJboxObjects.get(iProperty.fObject)->getProperty(iProperty.fKey);
  auto diff = property->storeValue(std::const_pointer_cast<JboxValue>(iValue));
  diff.fAtFrameIndex = iAtFrameIndex;
  handlePropertyDiff(diff, property->isWatched());
}

//------------------------------------------------------------------------
// Motherboard::storeProperty
//------------------------------------------------------------------------
void Motherboard::storeProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag, std::shared_ptr<const JboxValue> const &iValue, TJBox_UInt16 iAtFrameIndex)
{
  auto property = getObject(iObject)->getProperty(iTag);
  auto diff = property->storeValue(std::const_pointer_cast<JboxValue>(iValue));
  diff.fAtFrameIndex = iAtFrameIndex;
  handlePropertyDiff(diff, property->isWatched());
}

//------------------------------------------------------------------------
// Motherboard::handlePropertyDiff
//------------------------------------------------------------------------
void Motherboard::handlePropertyDiff(impl::JboxPropertyDiff const &iPropertyDiff, bool iWatched)
{
  if(iWatched)
  {
    if(fRTCNotify.find(iPropertyDiff.fPropertyRef) != fRTCNotify.end())
      addRTCNotifyDiff(iPropertyDiff);

    if(fRTCBindings.find(iPropertyDiff.fPropertyRef) != fRTCBindings.end())
      fRTCBindingsDiffs.emplace_back(iPropertyDiff);
  }
}

//------------------------------------------------------------------------
// Motherboard::create
//------------------------------------------------------------------------
std::unique_ptr<Motherboard> Motherboard::create(int iInstanceId, int iSampleRate, Config const &iConfig)
{
  auto res = std::unique_ptr<Motherboard>(new Motherboard(iConfig));

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
// Motherboard::addDeviceHostProperties
//------------------------------------------------------------------------
void Motherboard::addDeviceHostProperties()
{
  auto deviceHost = addObject("/device_host");
  deviceHost->addProperty("sample_context", PropertyOwner::kHostOwner, makeNumber(0), kJBox_DeviceHostSampleContext, lua::EPersistence::kPatch);
  deviceHost->addProperty("delete_sample", PropertyOwner::kHostOwner, makeNumber(0), kJBox_DeviceHostDeleteSample);
  deviceHost->addProperty("edit_sample", PropertyOwner::kHostOwner, makeNumber(0), kJBox_DeviceHostEditSample);
}

//------------------------------------------------------------------------
// Motherboard::addPatterns
//------------------------------------------------------------------------
void Motherboard::addPatterns(int iPatternCount)
{
  RE_MOCK_ASSERT(iPatternCount >= 1 && iPatternCount <= 32, "num_patterns [%d] must be between 1 and 32", iPatternCount);

  auto transport = getObject("/transport");
  transport->addProperty("pattern_index", PropertyOwner::kHostOwner, makeNumber(kJBox_NoPatternIndex), kJBox_TransportPatternIndex, lua::EPersistence::kPatch);
  transport->addProperty("pattern_start_pos", PropertyOwner::kHostOwner, makeNumber(0), kJBox_TransportPatternStartPos, lua::EPersistence::kPatch);

  for(int i = 0; i < iPatternCount; i++)
  {
    auto pattern = addObject(fmt::printf("/patterns/%d", i));
    pattern->addProperty("length", PropertyOwner::kRTOwner, makeNumber(0), kJBox_PatternsLength);
  }
}

//------------------------------------------------------------------------
// Motherboard::init
//------------------------------------------------------------------------
void Motherboard::init()
{
  // lua::MotherboardDef
  lua::MotherboardDef def{};
  MockJBoxVisitor defVisitor{def};
  for(auto &v: fConfig.fMotherboardDefs)
    std::visit(defVisitor, v);

  if(fConfig.fDebug)
  {
    std::cout << "---- MotherboardDef -----\n";
    std::cout << defVisitor.fCode << "\n";
    std::cout << "------------------------" << std::endl;
  }

  // lua::RealtimeController
  fRealtimeController = std::make_unique<lua::RealtimeController>();
  MockJBoxVisitor rtcVisitor{*fRealtimeController};
  for(auto &v: fConfig.fRealtimeControllers)
    std::visit(rtcVisitor, v);

  if(fConfig.fDebug)
  {
    std::cout << "---- RealtimeController -----\n";
    std::cout << rtcVisitor.fCode << "\n";
    std::cout << "------------------------" << std::endl;
  }

  // Realtime
  if(fConfig.fRealtime)
    fConfig.fRealtime(fRealtime);

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
  for(auto &&prop: customProperties->gui_owner)
    fGUIProperties[fmt::printf("/custom_properties/%s", prop.first)] = prop.second;

  // document_owner.properties
  for(auto &&prop: customProperties->document_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kDocOwner, stl::variant_cast(prop.second));

  // rt_owner.properties
  for(auto &&prop: customProperties->rt_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kRTOwner, stl::variant_cast(prop.second));

  // rtc_owner.properties
  for(auto &&prop: customProperties->rtc_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kRTCOwner, stl::variant_cast(prop.second));

  // user_samples
  if(!customProperties->user_samples.empty())
  {
    addDeviceHostProperties();
    for(int i = 0; i < customProperties->user_samples.size(); i++)
      addUserSample(i, customProperties->user_samples[i]);
  }

  // patterns
  if(def.getNumPatterns() > 0)
  {
    addPatterns(def.getNumPatterns());
  }

  // extra properties based on device type
  switch(fConfig.info().fDeviceType)
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

    case DeviceType::kUnknown:
      RE_MOCK_ASSERT(fConfig.info().fDeviceType != DeviceType::kUnknown);
      break;

    default:
      // no extra properties
      break;
  }

  // load the default patch if there is one
  if(fConfig.info().fSupportPatches)
  {
    RE_MOCK_ASSERT(!fConfig.info().fDefaultPatch.empty(), "support_patches is set to true but no default patch provided");
    loadPatch(fConfig.info().fDefaultPatch);
  }

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
}

//------------------------------------------------------------------------
// Motherboard::addProperty
//------------------------------------------------------------------------
void Motherboard::addProperty(TJBox_ObjectRef iParentObject,
                              std::string const &iPropertyName,
                              PropertyOwner iOwner,
                              lua::jbox_property const &iProperty)
{
  auto &o = fJboxObjects.get(iParentObject);

  auto fullPropertyPath = fmt::printf("%s/%s", o->fObjectPath, iPropertyName);
  RE_MOCK_ASSERT(fGUIProperties.find(fullPropertyPath) == fGUIProperties.end(), "duplicate property [%s] (already defined in gui_owner)", fullPropertyPath);

  struct DefaultValueVisitor
  {
    // lua::jbox_boolean_property
    std::unique_ptr<JboxValue> operator()(const std::shared_ptr<lua::jbox_boolean_property>& o) const { return fMotherboard->makeBoolean(o->fDefaultValue); }

    // lua::jbox_number_property
    std::unique_ptr<JboxValue> operator()(const std::shared_ptr<lua::jbox_number_property>& o) const { return fMotherboard->makeNumber(o->fDefaultValue); }

    // lua::jbox_performance_property
    std::unique_ptr<JboxValue> operator()(const std::shared_ptr<lua::jbox_performance_property>& o) const {
      RE_MOCK_ASSERT(o->fType != lua::jbox_performance_property::Type::UNKNOWN);
      TJBox_Float64 defaultValue = 0;
      if(o->fType == lua::jbox_performance_property::Type::PITCH_BEND)
        defaultValue = 0.5;
      return fMotherboard->makeNumber(defaultValue);
    }

    // lua::jbox_native_object
    std::unique_ptr<JboxValue> operator()(const std::shared_ptr<lua::jbox_native_object>& o) const {
      if(!o->fDefaultValue.operation.empty())
      {
        struct JboxValueVisitor {
          std::shared_ptr<const JboxValue> operator()(bool v) const { return fMotherboard->makeBoolean(v); }
          std::shared_ptr<const JboxValue> operator()(TJBox_Float64 v) const { return fMotherboard->makeNumber(v); }
          Motherboard *fMotherboard;
        };

        std::vector<std::shared_ptr<const JboxValue>> params{};
        std::transform(o->fDefaultValue.params.begin(),
                       o->fDefaultValue.params.end(),
                       std::back_inserter(params),
                       [this](auto &v) { return std::visit(JboxValueVisitor{fMotherboard}, v); });

        return fMotherboard->makeNativeObjectRW(o->fDefaultValue.operation, params);
      }
      else
        return fMotherboard->makeNil();
    }

    // lua::jbox_string_property
    std::unique_ptr<JboxValue> operator()(const std::shared_ptr<lua::jbox_string_property>& o) const {
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

    // lua::jbox_blob_property
    std::unique_ptr<JboxValue> operator()(const std::shared_ptr<lua::jbox_blob_property>& o) const {
      RE_MOCK_ASSERT(fOwner == PropertyOwner::kRTCOwner, "Blob must be owned by RTC");
      if(o->fDefaultValue)
        return fMotherboard->loadBlobAsync(*o->fDefaultValue);
      else
        return fMotherboard->makeEmptyBlob();
    }

    // lua::jbox_sample_property
    std::unique_ptr<JboxValue> operator()(const std::shared_ptr<lua::jbox_sample_property>& o) const {
      RE_MOCK_ASSERT(fOwner == PropertyOwner::kRTCOwner, "Sample must be owned by RTC");
      if(o->fDefaultValue)
        return fMotherboard->loadSampleAsync(*o->fDefaultValue);
      else
        return fMotherboard->makeEmptySample();
    }

    Motherboard *fMotherboard;
    PropertyOwner fOwner;
  };

  auto propertyTag = std::visit([](auto &p) { return p->fPropertyTag; }, iProperty);
  auto defaultValue = std::visit(DefaultValueVisitor{this, iOwner}, iProperty);
  auto persistence = std::visit([](auto &p) { return p->fPersistence; }, iProperty);
  auto valueType = std::visit([](auto &p) { return p->value_type(); }, iProperty);

  o->addProperty(iPropertyName,
                 iOwner,
                 valueType,
                 std::move(defaultValue),
                 propertyTag,
                 *persistence);
}

namespace impl {


struct SampleParameterDefaultValue {
  TJBox_UserSampleTag fTag;
  TJBox_Float64 fDefaultValue;
};

// YP implementation note: the default values were extracted from running code in Recon
static const std::map<std::string, SampleParameterDefaultValue> SAMPLE_PARAMETERS{
  {"root_key",             {kJBox_UserSampleRootKey,            60.0}},
  {"tune_cents",           {kJBox_UserSampleTuneCents,          50.0}},
  {"play_range_start",     {kJBox_UserSamplePlayRangeStart,     0}},
  {"play_range_end",       {kJBox_UserSamplePlayRangeEnd,       1.0}},
  {"loop_range_start",     {kJBox_UserSampleLoopRangeStart,     0}},
  {"loop_range_end",       {kJBox_UserSampleLoopRangeEnd,       1.0}},
  {"loop_mode",            {kJBox_UserSampleLoopMode,           0.0}},
  {"preview_volume_level", {kJBox_UserSamplePreviewVolumeLevel, 100}},
};

}

//------------------------------------------------------------------------
// Motherboard::addUserSample
//------------------------------------------------------------------------
void Motherboard::addUserSample(int iSampleIndex, std::shared_ptr<lua::jbox_user_sample_property> const &iProperty)
{
  auto objectName = iProperty->fName ?
                    fmt::printf("/user_samples/%s", *iProperty->fName) :
                    fmt::printf("/user_samples/%d", iSampleIndex);

  auto userSample = addObject(objectName);

  userSample->addProperty("item",
                          PropertyOwner::kDocOwner,
                          kJBox_Sample,
                          makeEmptySample(userSample->fObjectRef),
                          kJBox_UserSampleItem,
                          *iProperty->fPersistence);

  fUserSamplePropertyPaths.emplace_back(userSample->getProperty("item")->fPropertyPath);

  auto userSampleRef = userSample->fObjectRef;

  for(auto &sampleParameter: iProperty->fSampleParameters)
  {
    auto defaultValue = impl::SAMPLE_PARAMETERS.find(sampleParameter);
    RE_MOCK_ASSERT(defaultValue != impl::SAMPLE_PARAMETERS.end(), "unknown sample parameter %s", sampleParameter);
    userSample->addProperty(sampleParameter,
                            PropertyOwner::kDocOwner,
                            kJBox_Number,
                            makeNumber(defaultValue->second.fDefaultValue),
                            defaultValue->second.fTag,
                            *iProperty->fPersistence);
  }
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
      addRTCNotifyDiff(diff);
      fRTCNotify.emplace(diff.fPropertyRef);
    }
  }
  else
  {
    auto ref = getPropertyRef(iPropertyPath);
    addRTCNotifyDiff(fJboxObjects.get(ref.fObject)->watchPropertyForChange(ref.fKey));
    fRTCNotify.emplace(ref);
  }
}

//------------------------------------------------------------------------
// Motherboard::registerRTCBinding
//------------------------------------------------------------------------
impl::JboxPropertyDiff Motherboard::registerRTCBinding(std::string const &iPropertyPath, std::string const &iBindingKey)
{
  auto ref = getPropertyRef(iPropertyPath);
  fRTCBindings[ref] = iBindingKey;
  return fJboxObjects.get(ref.fObject)->watchPropertyForChange(ref.fKey);
}

//------------------------------------------------------------------------
// Motherboard::addAudioInput
//------------------------------------------------------------------------
void Motherboard::addAudioInput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_inputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, makeBoolean(false), kJBox_AudioInputConnected);
  std::shared_ptr<JboxValue> buffer = makeDSPBuffer();
  o->addProperty("buffer", PropertyOwner::kHostOwner, buffer, kJBox_AudioInputBuffer);
  fInputDSPBuffers.emplace_back(buffer);
}

//------------------------------------------------------------------------
// Motherboard::addAudioOutput
//------------------------------------------------------------------------
void Motherboard::addAudioOutput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_outputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, makeBoolean(false), kJBox_AudioOutputConnected);
  o->addProperty("dsp_latency", PropertyOwner::kRTCOwner, makeNumber(0), kJBox_AudioOutputDSPLatency);
  std::shared_ptr<JboxValue> buffer = makeDSPBuffer();
  o->addProperty("buffer", PropertyOwner::kHostOwner, buffer, kJBox_AudioOutputBuffer);
  fOutputDSPBuffers.emplace_back(buffer);
}

//------------------------------------------------------------------------
// Motherboard::addCVInput
//------------------------------------------------------------------------
void Motherboard::addCVInput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/cv_inputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, makeBoolean(false), kJBox_CVInputConnected);
  o->addProperty("value", PropertyOwner::kHostOwner, makeNumber(0), kJBox_CVInputValue);
}

//------------------------------------------------------------------------
// Motherboard::addCVOutput
//------------------------------------------------------------------------
void Motherboard::addCVOutput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/cv_outputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, makeBoolean(false), kJBox_CVOutputConnected);
  o->addProperty("dsp_latency", PropertyOwner::kRTCOwner, makeNumber(0), kJBox_CVOutputDSPLatency);
  o->addProperty("value", PropertyOwner::kRTOwner, makeNumber(0), kJBox_CVOutputValue);
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
// Motherboard::nextBatch
//------------------------------------------------------------------------
void Motherboard::nextBatch()
{
  // clearing note out events
  fNoteOutEvents.clear();

  // clearing output buffers (make sure that if the device doesn't do anything it is set to 0)
  for(auto buffer: fOutputDSPBuffers)
    buffer->getDSPBuffer().fill(0);

  // first we handle rtc bindings
  auto diffs = std::move(fRTCBindingsDiffs);

  for(auto &diff : diffs)
  {
    fRealtimeController->invokeBinding(this,
                                       fRTCBindings.at(diff.fPropertyRef),
                                       getPropertyPath(diff.fPropertyRef),
                                       diff.fCurrentValue);
  }

  // next we call render_realtime
  diffs = std::move(fRTCNotifyDiffs);

  auto const instance = getInstance<void *>();

  if(instance != nullptr && fRealtime.render_realtime)
  {
    // The diffs are supposed to be sorted by frame index
    if(diffs.size() > 1)
    {
      std::sort(diffs.begin(),
                diffs.end(),
                [](impl::JboxPropertyDiff const &l, impl::JboxPropertyDiff const &r) {
                  if(l.fAtFrameIndex == r.fAtFrameIndex)
                    return l.fInsertIndex < r.fInsertIndex;
                  else
                    return l.fAtFrameIndex < r.fAtFrameIndex;
                });
    }

    std::vector<TJBox_PropertyDiff> rtDiffs{};
    rtDiffs.reserve(diffs.size());
    for(auto const &diff: diffs)
    {
      auto d = TJBox_PropertyDiff {
        /* .fPreviousValue = */ to_TJBox_Value(diff.fPreviousValue),
        /* .fCurrentValue = */ to_TJBox_Value(diff.fCurrentValue),
        /* .fPropertyRef = */ diff.fPropertyRef,
        /* .fPropertyTag = */ diff.fPropertyTag,
        /* .fAtFrameIndex = */ diff.fAtFrameIndex
      };
      rtDiffs.emplace_back(d);
    }

    fRealtime.render_realtime(instance, rtDiffs.data(), rtDiffs.size());
  }

  // clearing current values
  fCurrentValues.clear();

  // clearing input buffers (consumed)
  for(auto buffer: fInputDSPBuffers)
    buffer->getDSPBuffer().fill(0);
}

//------------------------------------------------------------------------
// Motherboard::getDSPBuffer
//------------------------------------------------------------------------
Motherboard::DSPBuffer Motherboard::getDSPBuffer(std::string const &iAudioSocketPath) const
{
  return getJboxValue(fmt::printf("%s/buffer", iAudioSocketPath))->getDSPBuffer();
}

//------------------------------------------------------------------------
// Motherboard::getDSPBuffer
//------------------------------------------------------------------------
Motherboard::DSPBuffer Motherboard::getDSPBuffer(TJBox_ObjectRef iAudioSocket) const
{
  return fJboxObjects.get(iAudioSocket)->loadValue("buffer")->getDSPBuffer();
}

//------------------------------------------------------------------------
// Motherboard::setDSPBuffer
//------------------------------------------------------------------------
void Motherboard::setDSPBuffer(std::string const &iAudioSocketPath, Motherboard::DSPBuffer iBuffer)
{
  getJboxValue(fmt::printf("%s/buffer", iAudioSocketPath))->getDSPBuffer() = std::move(iBuffer);
}

//------------------------------------------------------------------------
// Motherboard::setDSPBuffer
//------------------------------------------------------------------------
void Motherboard::setDSPBuffer(TJBox_ObjectRef iAudioSocket, Motherboard::DSPBuffer iBuffer)
{
  fJboxObjects.get(iAudioSocket)->loadValue("buffer")->getDSPBuffer() = std::move(iBuffer);
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
  auto const &buffer = from_TJBox_Value(iValue)->getDSPBuffer();
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
  auto &buffer = std::const_pointer_cast<JboxValue>(from_TJBox_Value(iValue))->getDSPBuffer();
  std::copy(iAudio, iAudio + iEndFrame - iStartFrame, std::begin(buffer) + iStartFrame);
}

//------------------------------------------------------------------------
// Motherboard::getDSPBufferData
//------------------------------------------------------------------------
TJBox_DSPBufferInfo Motherboard::getDSPBufferInfo(TJBox_Value const &iValue) const
{
  auto const &buffer = from_TJBox_Value(iValue)->getDSPBuffer();
  return {/* .fSampleCount = */ static_cast<TJBox_Int64>(buffer.size()) };
}

//------------------------------------------------------------------------
// Motherboard::makeNativeObject
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeNativeObject(std::string const &iOperation,
                                                         std::vector<std::shared_ptr<const JboxValue>> const &iParams,
                                                         impl::NativeObject::AccessMode iAccessMode)
{
  if(fRealtime.create_native_object)
  {
    std::vector<TJBox_Value> params{};
    for(auto &p: iParams)
      params.emplace_back(to_TJBox_Value(p));
    auto nativeObject = fRealtime.create_native_object(iOperation.c_str(), params.data(), params.size());
    if(nativeObject)
    {
      auto res = std::make_unique<JboxValue>();
      res->fValueType = kJBox_NativeObject;
      res->fMotherboardValue = std::unique_ptr<impl::NativeObject>(new impl::NativeObject{
        /* .fNativeObject = */ nativeObject,
        /* .fOperation = */    iOperation,
        /* .fDeleter = */      fRealtime.destroy_native_object,
        /* .fAccessMode = */   iAccessMode
      });
      return res;
    }
  }
  return makeNil();
}

//------------------------------------------------------------------------
// Motherboard::loadBlobAsync
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::loadBlobAsync(std::string const &iBlobPath)
{
  RE_MOCK_ASSERT(stl::starts_with(iBlobPath, "/Private/"), "loadBlobAsync path must start with /Private [%s]", iBlobPath);

  auto b = std::make_unique<impl::Blob>();
  b->fBlobPath = iBlobPath;
  auto blobResource = fConfig.findBlobResource(iBlobPath);

  if(!blobResource)
  {
    b->fLoadingContext = Resource::LoadingContext{LoadStatus::kMissing};
  }
  else
  {
    auto loadingContext = fResourceLoadingContexts.find(iBlobPath);
    if(loadingContext != fResourceLoadingContexts.end())
      b->fLoadingContext = loadingContext->second;
    else
      b->fLoadingContext = Resource::LoadingContext{LoadStatus::kResident, blobResource->fData.size() };

    if(b->fLoadingContext.isLoadOk())
      b->fData = std::move(blobResource->fData);
  }

  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_BLOB;
  res->fMotherboardValue = std::move(b);
  return res;
}

//------------------------------------------------------------------------
// Motherboard::loadMoreBlob
//------------------------------------------------------------------------
bool Motherboard::loadMoreBlob(std::string const &iPropertyPath, long iCount)
{
  auto property = getProperty(iPropertyPath);
  auto blobValue = property->loadValue();

  RE_MOCK_ASSERT(blobValue->fValueType == kJBox_BLOB, "[%s] is not a blob", iPropertyPath);

  auto &blob = blobValue->getBlob();

  RE_MOCK_ASSERT(blob.fLoadingContext.isLoadOk(), "loadMoreBlob: Invalid status [%s]", blob.fLoadingContext.getStatusAsString());

  auto status = blob.fLoadingContext.fStatus;

  if(status == LoadStatus::kPartiallyResident)
  {
    auto residentSize = blob.getResidentSize();
    auto newResidentSize = iCount < 0 ? blob.getSize() : std::min(residentSize + iCount, blob.getSize());

    if(newResidentSize == blob.getSize())
      status = LoadStatus::kResident;

    if(residentSize != newResidentSize)
    {
      auto newBlobValue = makeEmptyBlob();
      auto &newBlob = newBlobValue->getBlob();
      newBlob = std::move(blob);
      newBlob.fLoadingContext = {status, static_cast<size_t>(newResidentSize) };

      // this will trigger a notify/diff if being watched
      storeProperty(property->fPropertyRef, std::move(newBlobValue));
    }
  }

  return status == LoadStatus::kResident;
}

//------------------------------------------------------------------------
// Motherboard::getBLOBInfo
//------------------------------------------------------------------------
impl::Blob::Info Motherboard::getBLOBInfo(JboxValue const &iValue) const
{
  auto const &b = iValue.getBlob();

  return {
    /* .fSize = */     b.fData.size(),
    /* .fLoadingContext = */ b.fLoadingContext
  };
}

//------------------------------------------------------------------------
// Motherboard::getBLOBData
//------------------------------------------------------------------------
void Motherboard::getBLOBData(TJBox_Value const &iValue, TJBox_SizeT iStart, TJBox_SizeT iEnd, TJBox_UInt8 *oData) const
{
  RE_MOCK_ASSERT(iStart >= 0 && iEnd >= 0 && iStart <= iEnd);
  if(iStart == iEnd)
    return;
  auto blobValue = from_TJBox_Value(iValue);
  auto const &b = blobValue->getBlob();
  RE_MOCK_ASSERT(b.fLoadingContext.isLoadOk(), "getBLOBData: Cannot get blob data: Invalid status [%s]", b.fLoadingContext.getStatusAsString());
  RE_MOCK_ASSERT(iEnd <= b.getResidentSize(), "getBLOBData: Not enough data iEnd=%l > fResidentSize=%l", iEnd, b.getResidentSize());
  RE_MOCK_ASSERT(b.getResidentSize() <= b.getSize()); // sanity check
  std::copy(std::begin(b.fData) + iStart, std::begin(b.fData) + iEnd, oData);
}

//------------------------------------------------------------------------
// Motherboard::makeString
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeString(std::string iValue) const
{
  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_String;
  res->fMotherboardValue = std::unique_ptr<impl::String>(new impl::String{
    /* .fMaxSize */ 0,
    /* .fValue */ std::move(iValue)
  });
  return res;
}

//------------------------------------------------------------------------
// Motherboard::makeRTString
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeRTString(int iMaxSize) const
{
  RE_MOCK_ASSERT(iMaxSize > 0 && iMaxSize <= 2048, "RTString invalid max_size [%d]", iMaxSize);
  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_String;
  res->fMotherboardValue = std::unique_ptr<impl::String>(new impl::String{
    /* .fMaxSize */ iMaxSize,
    /* .fValue */ {}
  });
  return res;
}

//------------------------------------------------------------------------
// Motherboard::makeNativeObjectRO
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeNativeObjectRO(std::string const &iOperation,
                                                           std::vector<std::shared_ptr<const JboxValue>> const &iParams)
{
  return makeNativeObject(iOperation, iParams, impl::NativeObject::kReadOnly);
}

//------------------------------------------------------------------------
// Motherboard::makeNativeObjectRW
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeNativeObjectRW(std::string const &iOperation,
                                                           std::vector<std::shared_ptr<const JboxValue>> const &iParams)
{
  return makeNativeObject(iOperation, iParams, impl::NativeObject::kReadWrite);
}

//------------------------------------------------------------------------
// Motherboard::getNativeObjectRO
//------------------------------------------------------------------------
const void *Motherboard::getNativeObjectRO(TJBox_Value const &iValue) const
{
  if(getValueType(iValue) == kJBox_Nil)
    return nullptr;
  else
    return from_TJBox_Value(iValue)->getNativeObject().fNativeObject;
}

//------------------------------------------------------------------------
// Motherboard::getNativeObjectRW
//------------------------------------------------------------------------
void *Motherboard::getNativeObjectRW(TJBox_Value const &iValue) const
{
  if(getValueType(iValue) == kJBox_Nil)
    return nullptr;
  else
  {
    auto &no = from_TJBox_Value(iValue)->getNativeObject();
    RE_MOCK_ASSERT(no.fAccessMode == impl::NativeObject::kReadWrite, "Trying to access RO native object in RW mode");
    return no.fNativeObject;
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
  auto &rtString = property->loadValue()->getString();
  RE_MOCK_ASSERT(rtString.isRTString());
  RE_MOCK_ASSERT(iSize >= 0 && iSize <= rtString.fMaxSize);
  auto newRTString = makeRTString(rtString.fMaxSize);
  std::copy(iData, iData + iSize, std::back_inserter(newRTString->getString().fValue));
  property->storeValue(std::move(newRTString));
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
  storeProperty(ref, makeNumber(iVelocity), iAtFrameIndex);
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
  storeProperty(JBox_MakePropertyRef(iSocket, "connected"), makeBoolean(true));
}

//------------------------------------------------------------------------
// Motherboard::disconnectSocket
//------------------------------------------------------------------------
void Motherboard::disconnectSocket(TJBox_ObjectRef iSocket)
{
  storeProperty(JBox_MakePropertyRef(iSocket, "connected"), makeBoolean(false));
}

//------------------------------------------------------------------------
// Motherboard::getCVSocketValue
//------------------------------------------------------------------------
TJBox_Float64 Motherboard::getCVSocketValue(TJBox_ObjectRef iCVSocket) const
{
  return fJboxObjects.get(iCVSocket)->loadValue("value")->getNumber();
}

//------------------------------------------------------------------------
// Motherboard::setCVSocketValue
//------------------------------------------------------------------------
void Motherboard::setCVSocketValue(TJBox_ObjectRef iCVSocket, TJBox_Float64 iValue)
{
  storeProperty(JBox_MakePropertyRef(iCVSocket, "value"), makeNumber(iValue));
}

//------------------------------------------------------------------------
// Motherboard::isSameValue
//------------------------------------------------------------------------
bool Motherboard::isSameValue(TJBox_Value const &lhs, TJBox_Value const &rhs) const
{
  if(getValueType(lhs) != getValueType(rhs))
    return false;

  switch(getValueType(lhs))
  {
    case kJBox_Nil:
    case kJBox_Incompatible:
      return true;

    case kJBox_Number:
      return stl::almost_equal(JBox_GetNumber(lhs), JBox_GetNumber(rhs));

    case kJBox_String:
      return toString(lhs) == toString(rhs);

    case kJBox_Boolean:
      return JBox_GetBoolean(lhs) == JBox_GetBoolean(rhs);

    default:
      return from_TJBox_Value(lhs) == from_TJBox_Value(rhs);
  }
}

//------------------------------------------------------------------------
// Motherboard::toString
//------------------------------------------------------------------------
std::string Motherboard::toString(JboxValue const &iValue, char const *iFormat) const
{
  switch(iValue.getValueType())
  {
    case kJBox_Nil:
      return "Nil";

    case kJBox_Number:
      return fmt::printf(iFormat ? iFormat : "%f", iValue.getNumber());

    case kJBox_Boolean:
      return fmt::printf(iFormat ? iFormat : "%s", iValue.getBoolean() ? "true" : "false");

    case kJBox_DSPBuffer:
      return fmt::printf(iFormat ? iFormat : "DSPBuffer{%ld}", iValue.getUniqueId());

    case kJBox_NativeObject:
      return fmt::printf(iFormat ? iFormat : "R%sNativeObject{%ld}",
                         iValue.getNativeObject().fAccessMode == impl::NativeObject::kReadWrite ? "W" : "O", iValue.getUniqueId());

    case kJBox_Incompatible:
      return "<incompatible>";

    case kJBox_String:
      return fmt::printf(iFormat ? iFormat : "%s", iValue.getString().fValue);

    case kJBox_BLOB:
    {
      auto const &blob = iValue.getBlob();
      return fmt::printf(iFormat ? iFormat : "Blob{.fSize=%ld,.fResidentSize=%ld,.fLoadStatus=%s,.fBlobPath=[%s]}",
                         blob.getSize(),
                         blob.getResidentSize(),
                         blob.fLoadingContext.getStatusAsString(),
                         blob.fBlobPath);
    }

    case kJBox_Sample:
    {
      auto const &sample = iValue.getSample();
      return fmt::printf(iFormat ? iFormat : "%s{.fChannels=%d,.fSampleRate=%d,.fFrameCount=%d,.fResidentFrameCount=%d,.fLoadStatus=%s,.fSamplePath=[%s]}",
                         sample.isUserSample() ? "UserSample" : "Sample",
                         sample.fChannels,
                         sample.fSampleRate,
                         sample.getFrameCount(),
                         sample.getResidentFrameCount(),
                         sample.fLoadingContext.getStatusAsString(),
                         sample.fSamplePath);
    }

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
  auto const &s = property->loadValue()->getString();
  RE_MOCK_ASSERT(s.isRTString());
  return s.fValue;
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
  auto const &s = property->loadValue()->getString();
  RE_MOCK_ASSERT(!s.isRTString());
  return s.fValue;
}

//------------------------------------------------------------------------
// Motherboard::setString
//------------------------------------------------------------------------
void Motherboard::setString(std::string const &iPropertyPath, std::string iValue)
{
  auto property = getProperty(iPropertyPath);
  RE_MOCK_ASSERT(property->fOwner != PropertyOwner::kRTOwner);
  auto const &s = property->loadValue()->getString();
  RE_MOCK_ASSERT(!s.isRTString());
  if(s.fValue != iValue)
    storeProperty(property->fPropertyRef, makeString(iValue));
}

//------------------------------------------------------------------------
// Motherboard::getStringLength
//------------------------------------------------------------------------
TJBox_UInt32 Motherboard::getStringLength(TJBox_Value const &iValue) const
{
  auto const &s = from_TJBox_Value(iValue)->getString();
  RE_MOCK_ASSERT(!s.isRTString());
  return s.fValue.size();
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

  auto const &s = from_TJBox_Value(iValue)->getString();
  RE_MOCK_ASSERT(!s.isRTString());
  RE_MOCK_ASSERT(iStart >= 0 && iStart < s.fValue.size());
  RE_MOCK_ASSERT(iEnd >= 0 && iEnd <= s.fValue.size());
  std::copy(s.fValue.begin() + iStart, s.fValue.begin() + iEnd, oString);
  oString[iEnd - iStart] = '\0';
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
// Motherboard::stopAllNotes
//------------------------------------------------------------------------
void Motherboard::stopAllNotes()
{
  if(fNoteStatesRef > 0)
  {
    for(int i = FIRST_MIDI_NOTE; i <= LAST_MIDI_NOTE; i++)
      storeProperty(fNoteStatesRef, static_cast<TJBox_Tag>(i), makeNumber(0));
  }
}

//------------------------------------------------------------------------
// Motherboard::addPropertyDiff
//------------------------------------------------------------------------
void Motherboard::addRTCNotifyDiff(impl::JboxPropertyDiff const &iDiff)
{
  impl::JboxPropertyDiff diff{iDiff};
  diff.fInsertIndex = fRTCNotifyDiffs.size();
  fRTCNotifyDiffs.emplace_back(diff);
}

//------------------------------------------------------------------------
// Motherboard::loadPatch
//------------------------------------------------------------------------
void Motherboard::loadPatch(std::string const &iPatchPath)
{
  RE_MOCK_ASSERT(stl::starts_with(iPatchPath, "/"), "patch path must start with / [%s]", iPatchPath);
  auto patchResource = fConfig.findPatchResource(iPatchPath);
  RE_MOCK_ASSERT(patchResource != std::nullopt, "loadPatch: Cannot find patch [%s]", iPatchPath);
  loadPatch(*patchResource);
}

//------------------------------------------------------------------------
// Motherboard::loadPatch
//------------------------------------------------------------------------
void Motherboard::loadPatch(ConfigFile const &iPatchFile)
{
  loadPatch(PatchParser::from(iPatchFile));
}

//------------------------------------------------------------------------
// Motherboard::loadPatch
//------------------------------------------------------------------------
void Motherboard::loadPatch(ConfigString const &iPatchString)
{
  loadPatch(PatchParser::from(iPatchString));
}

//------------------------------------------------------------------------
// Motherboard::loadPatch
//------------------------------------------------------------------------
void Motherboard::loadPatch(Resource::Patch const &iPatch)
{
  for(auto const &[propertyPath, property]: iPatch.fProperties)
  {
    // it is a GUI property... ignored
    if(fGUIProperties.find(propertyPath) != fGUIProperties.end())
      continue;

    RE_MOCK_ASSERT(getProperty(propertyPath)->fPersistence == lua::EPersistence::kPatch,
                   "Property [%s] must have a persistence set to \"patch\"", propertyPath);

    struct visitor
    {
      std::string fPropertyPath{};
      Motherboard *fMotherboard{};

      void operator()(Resource::Patch::boolean_property const &o) {
        if(fMotherboard->getBool(fPropertyPath) != o.fValue)
          fMotherboard->setBool(fPropertyPath, o.fValue);
      }
      void operator()(Resource::Patch::number_property const &o) {
        if(!stl::almost_equal(fMotherboard->getNum(fPropertyPath), o.fValue))
          fMotherboard->setNum(fPropertyPath, o.fValue);
      }
      void operator()(Resource::Patch::string_property const &o) {
        if(fMotherboard->getString(fPropertyPath) != o.fValue)
          fMotherboard->setString(fPropertyPath, o.fValue);
      }
      void operator()(Resource::Patch::sample_property const &o) {
        auto property = fMotherboard->getProperty(fPropertyPath);
        if(property->loadValue()->getSample().fSamplePath != o.fValue)
        {
          auto newSample = fMotherboard->loadSampleAsync(o.fValue);
          newSample->getSample().fSampleItem = property->loadValue()->getSample().fSampleItem;
          fMotherboard->storeProperty(property->fPropertyRef, std::move(newSample));
        }
      }
    };

    std::visit(visitor{propertyPath, this}, property);
  }
}

//------------------------------------------------------------------------
// Motherboard::makeNil
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeNil() const
{
  return std::make_unique<JboxValue>();
}

//------------------------------------------------------------------------
// Motherboard::makeIncompatible
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeIncompatible() const
{
  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_Incompatible;
  res->fMotherboardValue = JboxValue::Incompatible{};
  return res;
}

//------------------------------------------------------------------------
// Motherboard::makeNumber
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeNumber(TJBox_Float64 iValue) const
{
  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_Number;
  res->fMotherboardValue = iValue;
  return res;
}

//------------------------------------------------------------------------
// Motherboard::makeBoolean
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeBoolean(bool iValue) const
{
  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_Boolean;
  res->fMotherboardValue = iValue;
  return res;
}

//------------------------------------------------------------------------
// Motherboard::makeDSPBuffer
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeDSPBuffer() const
{
  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_DSPBuffer;
  res->fMotherboardValue = std::make_unique<DSPBuffer>();
  return res;
}

//------------------------------------------------------------------------
// Motherboard::makeEmptyBlob
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeEmptyBlob() const
{
  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_BLOB;
  res->fMotherboardValue = std::make_unique<impl::Blob>();
  return res;
}

//------------------------------------------------------------------------
// Motherboard::makeEmptySample
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::makeEmptySample(TJBox_ObjectRef iSampleItem) const
{
  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_Sample;
  res->fMotherboardValue = std::make_unique<impl::Sample>();
  res->getSample().fSampleItem = iSampleItem;
  return res;
}

//------------------------------------------------------------------------
// Motherboard::getSampleInfo
//------------------------------------------------------------------------
TJBox_SampleInfo Motherboard::getSampleInfo(JboxValue const &iValue) const
{
  return iValue.getSample().getSampleInfo();
}

//------------------------------------------------------------------------
// Motherboard::getSampleMetadata
//------------------------------------------------------------------------
impl::Sample::Metadata Motherboard::getSampleMetadata(JboxValue const &iValue) const
{
  auto sample = iValue.getSample();

  impl::Sample::Metadata res;
  auto &md = res.fMain;

  res.fLoadingContext = sample.fLoadingContext;
  res.fSamplePath = sample.fSamplePath;

  md.fSpec = {
    /* .fFrameCount = */ sample.getFrameCount(),
    /* .fChannels = */   sample.fChannels,
    /* .fSampleRate = */ sample.fSampleRate
  };
  md.fResidentFrameCount = sample.getResidentFrameCount();
  md.fStatus = sample.fLoadingContext.getStatusAsInt();

  if(sample.isUserSample())
  {
    auto const item = getObject(sample.fSampleItem);

    md.fParameters.fRootNote =
      item->hasProperty(kJBox_UserSampleRootKey) ?
      item->loadValue(kJBox_UserSampleRootKey)->getNumber() :
      impl::SAMPLE_PARAMETERS.at("root_key").fDefaultValue;

    md.fParameters.fTuneCents =
      item->hasProperty(kJBox_UserSampleTuneCents) ?
      item->loadValue(kJBox_UserSampleTuneCents)->getNumber() :
      impl::SAMPLE_PARAMETERS.at("tune_cents").fDefaultValue;

    md.fParameters.fPlayRangeStart =
      item->hasProperty(kJBox_UserSamplePlayRangeStart) ?
      item->loadValue(kJBox_UserSamplePlayRangeStart)->getNumber() :
      impl::SAMPLE_PARAMETERS.at("play_range_start").fDefaultValue;

    md.fParameters.fPlayRangeEnd =
      item->hasProperty(kJBox_UserSamplePlayRangeEnd) ?
      item->loadValue(kJBox_UserSamplePlayRangeEnd)->getNumber() :
      impl::SAMPLE_PARAMETERS.at("play_range_end").fDefaultValue;

    md.fParameters.fLoopRangeStart =
      item->hasProperty(kJBox_UserSampleLoopRangeStart) ?
      item->loadValue(kJBox_UserSampleLoopRangeStart)->getNumber() :
      impl::SAMPLE_PARAMETERS.at("loop_range_start").fDefaultValue;

    md.fParameters.fLoopRangeEnd =
      item->hasProperty(kJBox_UserSampleLoopRangeEnd) ?
      item->loadValue(kJBox_UserSampleLoopRangeEnd)->getNumber() :
      impl::SAMPLE_PARAMETERS.at("loop_range_end").fDefaultValue;

    md.fParameters.fLoopMode =
      item->hasProperty(kJBox_UserSampleLoopMode) ?
      item->loadValue(kJBox_UserSampleLoopMode)->getNumber() :
      impl::SAMPLE_PARAMETERS.at("loop_mode").fDefaultValue;

    md.fParameters.fVolumeLevel =
      item->hasProperty(kJBox_UserSamplePreviewVolumeLevel) ?
      item->loadValue(kJBox_UserSamplePreviewVolumeLevel)->getNumber() :
      impl::SAMPLE_PARAMETERS.at("preview_volume_level").fDefaultValue;
  }
  else
  {
    md.fParameters.fRootNote = impl::SAMPLE_PARAMETERS.at("root_key").fDefaultValue;
    md.fParameters.fTuneCents = impl::SAMPLE_PARAMETERS.at("tune_cents").fDefaultValue;
    md.fParameters.fPlayRangeStart = impl::SAMPLE_PARAMETERS.at("play_range_start").fDefaultValue;
    md.fParameters.fPlayRangeEnd = impl::SAMPLE_PARAMETERS.at("play_range_end").fDefaultValue;
    md.fParameters.fLoopRangeStart = impl::SAMPLE_PARAMETERS.at("loop_range_start").fDefaultValue;
    md.fParameters.fLoopRangeEnd = impl::SAMPLE_PARAMETERS.at("loop_range_end").fDefaultValue;
    md.fParameters.fLoopMode = impl::SAMPLE_PARAMETERS.at("loop_mode").fDefaultValue;
    md.fParameters.fVolumeLevel = impl::SAMPLE_PARAMETERS.at("preview_volume_level").fDefaultValue;
  }

  return res;
}

//------------------------------------------------------------------------
// Motherboard::loadSampleAsync
//------------------------------------------------------------------------
std::unique_ptr<JboxValue> Motherboard::loadSampleAsync(std::string const &iSamplePath)
{
  RE_MOCK_ASSERT(stl::starts_with(iSamplePath, "/"), "loadSampleAsync path must start with / [%s]", iSamplePath);

  auto sample =  std::make_unique<impl::Sample>();
  sample->fSamplePath = iSamplePath;
  auto sampleResource = fConfig.findSampleResource(iSamplePath);

  if(!sampleResource)
  {
    sample->fLoadingContext = Resource::LoadingContext{LoadStatus::kMissing};
  }
  else
  {
    RE_MOCK_ASSERT(sampleResource->fChannels > 0);

    auto loadingContext = fResourceLoadingContexts.find(iSamplePath);
    if(loadingContext != fResourceLoadingContexts.end())
      sample->fLoadingContext = loadingContext->second;
    else
      sample->fLoadingContext = Resource::LoadingContext{LoadStatus::kResident, sampleResource->fData.size() / sampleResource->fChannels };

    if(sample->fLoadingContext.isLoadOk())
    {
      sample->fSampleRate = sampleResource->fSampleRate;
      sample->fChannels = sampleResource->fChannels;
      sample->fData = std::move(sampleResource->fData);
    }
  }

  auto res = std::make_unique<JboxValue>();
  res->fValueType = kJBox_Sample;
  res->fMotherboardValue = std::move(sample);
  return res;
}

//------------------------------------------------------------------------
// Motherboard::loadMoreSample
//------------------------------------------------------------------------
bool Motherboard::loadMoreSample(std::string const &iPropertyPath, long iFrameCount)
{
  auto property = getProperty(iPropertyPath);
  auto sampleValue = property->loadValue();

  RE_MOCK_ASSERT(sampleValue->fValueType == kJBox_Sample, "[%s] is not a sample", iPropertyPath);

  auto &sample = sampleValue->getSample();

  RE_MOCK_ASSERT(sample.fLoadingContext.isLoadOk(), "loadMoreSample: Invalid status [%s]", sample.fLoadingContext.getStatusAsString());

  auto status = sample.fLoadingContext.fStatus;

  if(status == LoadStatus::kPartiallyResident)
  {
    auto residentFrameCount = sample.getResidentFrameCount();
    auto newResidentFrameCount = iFrameCount < 0 ?
                                 sample.getFrameCount() :
                                 std::min(residentFrameCount + static_cast<TJBox_AudioFramePos>(iFrameCount), sample.getFrameCount());

    if(newResidentFrameCount == sample.getFrameCount())
      status = LoadStatus::kResident;

    if(residentFrameCount != newResidentFrameCount)
    {
      auto newSampleValue = makeEmptySample(sample.fSampleItem);
      auto &newSample = newSampleValue->getSample();
      newSample = std::move(sample);
      newSample.fLoadingContext = {status, static_cast<size_t>(newResidentFrameCount) };

      // this will trigger a notify/diff if being watched
      storeProperty(property->fPropertyRef, std::move(newSampleValue));
    }
  }

  return status == LoadStatus::kResident;
}

//------------------------------------------------------------------------
// Motherboard::loadUserSampleAsync
//------------------------------------------------------------------------
void Motherboard::loadUserSampleAsync(std::string const &iPropertyPath,
                                      std::string const &iResourcePath,
                                      std::optional<Resource::LoadingContext> iCtx)
{
  auto sampleItem = getJboxValue(iPropertyPath)->getSample().fSampleItem;

  if(iCtx)
    setResourceLoadingContext(iResourcePath, *iCtx);

  auto newSample = loadSampleAsync(iResourcePath);
  newSample->getSample().fSampleItem = sampleItem;

  storeProperty(getPropertyRef(iPropertyPath), std::move(newSample));
}

//------------------------------------------------------------------------
// Motherboard::deleteUserSample
//------------------------------------------------------------------------
void Motherboard::deleteUserSample(std::string const &iPropertyPath)
{
  auto sampleItem = getJboxValue(iPropertyPath)->getSample().fSampleItem;
  storeProperty(getPropertyRef(iPropertyPath), makeEmptySample(sampleItem));
}

//------------------------------------------------------------------------
// Motherboard::getSampleData
//------------------------------------------------------------------------
void Motherboard::getSampleData(TJBox_Value iValue,
                                TJBox_AudioFramePos iStartFrame,
                                TJBox_AudioFramePos iEndFrame,
                                TJBox_AudioSample *oAudio) const
{
  RE_MOCK_ASSERT(iStartFrame >= 0 && iEndFrame >= 0 && iStartFrame <= iEndFrame);
  if(iStartFrame == iEndFrame)
    return;
  auto sampleValue = from_TJBox_Value(iValue);
  auto const &s = sampleValue->getSample();
  RE_MOCK_ASSERT(s.fLoadingContext.isLoadOk(), "getSampleData: Cannot get blob data: Invalid status [%s]", s.fLoadingContext.getStatusAsString());
  RE_MOCK_ASSERT(iEndFrame <= s.getResidentFrameCount(), "getSampleData: Not enough data iEndFrame=%l > fResidentFrameCount=%l", iEndFrame, s.getResidentFrameCount());
  RE_MOCK_ASSERT(s.getResidentFrameCount() <= s.getFrameCount()); // sanity check
  std::copy(std::begin(s.fData) + iStartFrame * s.fChannels, std::begin(s.fData) + iEndFrame * s.fChannels, oAudio);
}

//------------------------------------------------------------------------
// Motherboard::trace
//------------------------------------------------------------------------
void Motherboard::trace(const char *iFile, TJBox_Int32 iLine, const char *iMessage) const
{
  if(fConfig.traceEnabled())
    RE_MOCK_LOG_JUKEBOX_INFO("%s:%d | %s", iFile, iLine, iMessage);
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
  RE_MOCK_ASSERT(fProperties.find(iPropertyName) != fProperties.end(), "missing property [%s] for object [%s]", iPropertyName, fObjectPath);
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
  RE_MOCK_ASSERT(iter != fProperties.end(), "missing property tag [%d] for object [%s]", iPropertyTag, fObjectPath);
  return iter->second.get();
}

//------------------------------------------------------------------------
// JboxObject::getProperty
//------------------------------------------------------------------------
bool impl::JboxObject::hasProperty(TJBox_Tag iPropertyTag) const
{
  return std::find_if(fProperties.begin(),
                      fProperties.end(),
                      [iPropertyTag](auto const &p) { return p.second->fTag == iPropertyTag; } ) != fProperties.end();
}

//------------------------------------------------------------------------
// JboxObject::loadValue
//------------------------------------------------------------------------
std::shared_ptr<const JboxValue> impl::JboxObject::loadValue(std::string const &iPropertyName) const
{
  return getProperty(iPropertyName)->loadValue();
}

//------------------------------------------------------------------------
// JboxObject::loadValue
//------------------------------------------------------------------------
std::shared_ptr<const JboxValue> impl::JboxObject::loadValue(TJBox_Tag iPropertyTag) const
{
  return getProperty(iPropertyTag)->loadValue();
}

//------------------------------------------------------------------------
// JboxObject::loadValue
//------------------------------------------------------------------------
std::shared_ptr<JboxValue> impl::JboxObject::loadValue(std::string const &iPropertyName)
{
  return getProperty(iPropertyName)->loadValue();
}

//------------------------------------------------------------------------
// JboxObject::loadValue
//------------------------------------------------------------------------
std::shared_ptr<JboxValue> impl::JboxObject::loadValue(TJBox_Tag iPropertyTag)
{
  return getProperty(iPropertyTag)->loadValue();
}

//------------------------------------------------------------------------
// JboxObject::storeValue
//------------------------------------------------------------------------
impl::JboxPropertyDiff impl::JboxObject::storeValue(std::string const &iPropertyName, std::shared_ptr<JboxValue> iValue)
{
  return getProperty(iPropertyName)->storeValue(std::move(iValue));
}

//------------------------------------------------------------------------
// JboxObject::storeValue
//------------------------------------------------------------------------
impl::JboxPropertyDiff impl::JboxObject::storeValue(TJBox_Tag iPropertyTag, std::shared_ptr<JboxValue> iValue)
{
  return getProperty(iPropertyTag)->storeValue(std::move(iValue));
}

//------------------------------------------------------------------------
// JboxObject::addProperty
//------------------------------------------------------------------------
void impl::JboxObject::addProperty(const std::string& iPropertyName,
                                   PropertyOwner iOwner,
                                   std::shared_ptr<JboxValue> iInitialValue,
                                   TJBox_Tag iPropertyTag,
                                   lua::EPersistence iPersistence)
{
  addProperty(iPropertyName, iOwner, iInitialValue->getValueType(), std::move(iInitialValue), iPropertyTag, iPersistence);
}

//------------------------------------------------------------------------
// JboxObject::addProperty
//------------------------------------------------------------------------
void impl::JboxObject::addProperty(const std::string& iPropertyName,
                                   PropertyOwner iOwner,
                                   TJBox_ValueType iValueType,
                                   std::shared_ptr<JboxValue> iInitialValue,
                                   TJBox_Tag iPropertyTag,
                                   lua::EPersistence iPersistence)
{
  RE_MOCK_ASSERT(fProperties.find(iPropertyName) == fProperties.end(), "duplicate property [%s] for object [%s]", iPropertyName, fObjectPath);
  fProperties[iPropertyName] =
    std::make_unique<JboxProperty>(JBox_MakePropertyRef(fObjectRef, iPropertyName.c_str()),
                                   fmt::printf("%s/%s", fObjectPath, iPropertyName),
                                   iValueType,
                                   iOwner,
                                   iInitialValue,
                                   iPropertyTag,
                                   iPersistence);
}

//------------------------------------------------------------------------
// JboxObject::watchPropertyForChange
//------------------------------------------------------------------------
impl::JboxPropertyDiff impl::JboxObject::watchPropertyForChange(std::string const &iPropertyName)
{
  RE_MOCK_ASSERT(fProperties.find(iPropertyName) != fProperties.end(), "missing property [%s] for object [%s]", iPropertyName, fObjectPath);
  return fProperties[iPropertyName]->watchForChange();
}

//------------------------------------------------------------------------
// JboxObject::watchAllPropertiesForChange
//------------------------------------------------------------------------
std::vector<impl::JboxPropertyDiff> impl::JboxObject::watchAllPropertiesForChange()
{
  std::vector<impl::JboxPropertyDiff> res{};

  for(auto &[k, property] : fProperties)
    res.emplace_back(property->watchForChange());

  return res;
}

//------------------------------------------------------------------------
// JboxProperty::JboxProperty
//------------------------------------------------------------------------
impl::JboxProperty::JboxProperty(TJBox_PropertyRef const &iPropertyRef,
                                 std::string iPropertyPath,
                                 TJBox_ValueType iValueType,
                                 PropertyOwner iOwner,
                                 std::shared_ptr<JboxValue> iInitialValue,
                                 TJBox_Tag iTag,
                                 lua::EPersistence iPersistence) :
  fPropertyRef{iPropertyRef},
  fPropertyPath{std::move(iPropertyPath)},
  fOwner{iOwner},
  fTag{iTag},
  fValueType{iValueType},
  fInitialValue{iInitialValue},
  fValue{iInitialValue},
  fPersistence{iPersistence}
{
  RE_MOCK_ASSERT(iInitialValue->getValueType() == fValueType ||
                 iInitialValue->getValueType() == TJBox_ValueType::kJBox_Nil);
}

//------------------------------------------------------------------------
// JboxProperty::storeValue
//------------------------------------------------------------------------
impl::JboxPropertyDiff impl::JboxProperty::storeValue(std::shared_ptr<JboxValue> iValue)
{
  RE_MOCK_ASSERT(iValue->getValueType() == fValueType ||
                 iValue->getValueType() == TJBox_ValueType::kJBox_Nil,
                 "invalid property type for [%s]", fPropertyPath);

  auto previousValue = fValue;
  fValue = std::move(iValue);
  return JboxPropertyDiff{
    /* .fPreviousValue = */ std::move(previousValue),
    /* .fCurrentValue = */  fValue,
    /* .fPropertyRef = */   fPropertyRef,
    /* .fPropertyTag = */   fTag
  };

}

//------------------------------------------------------------------------
// JboxProperty::watchForChange
//------------------------------------------------------------------------
impl::JboxPropertyDiff impl::JboxProperty::watchForChange()
{
  fWatched = true;
  auto initialValue = fInitialValue;
  fInitialValue = nullptr; // getting rid of initial value
  return impl::JboxPropertyDiff{
    /* .fPreviousValue = */ initialValue,
    /* .fCurrentValue = */  fValue,
    /* .fPropertyRef = */   fPropertyRef,
    /* .fPropertyTag = */   fTag
  };
}

//------------------------------------------------------------------------
// impl::Blob::getBlobInfo
//------------------------------------------------------------------------
TJBox_BLOBInfo impl::Blob::Info::to_TJBox_BLOBInfo() const
{
  return {
    /* .fSize = */         getSize(),
    /* .fResidentSize = */ getResidentSize()
  };
}

//------------------------------------------------------------------------
// Sample::getSampleInfo
//------------------------------------------------------------------------
TJBox_SampleInfo impl::Sample::getSampleInfo() const
{
  return {
    /* .fFrameCount = */         getFrameCount(),
    /* .fResidentFrameCount = */ getResidentFrameCount(),
    /* .fChannels = */           fChannels,
    /* .fSampleRate = */         fSampleRate
  };
}

//------------------------------------------------------------------------
// Sample::Metadata::getLoopModeAsString
//------------------------------------------------------------------------
std::string impl::Sample::Metadata::getLoopModeAsString() const
{
  std::string res;
  switch(fMain.fParameters.fLoopMode)
  {
    case 0:
      res = "loop_off";
      break;

    case 1:
      res = "loop_forward";
      break;

    case 2:
      res = "loop_backward_forward";
      break;
  }
  return res;
}

//------------------------------------------------------------------------
// Sample::Metadata::getSampleName
//------------------------------------------------------------------------
std::string impl::Sample::Metadata::getSampleName() const
{
  auto p = fSamplePath.find_last_of('/');
  if(p == std::string::npos || p == fSamplePath.size() - 1)
    return fSamplePath;
  return fSamplePath.substr(p + 1);
}


}

