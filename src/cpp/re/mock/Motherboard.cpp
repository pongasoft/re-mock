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
#include <Jukebox.h>

namespace re::mock {

//------------------------------------------------------------------------
// Motherboard::Motherboard
//------------------------------------------------------------------------
Motherboard::Motherboard()
{
//  DLOG_F(INFO, "Motherboard(%p)", this);

  // /custom_properties
  fCustomPropertiesRef = addObject("/custom_properties")->fObjectRef;

//  // /custom_properties/instance (for the "privateState")
//  addProperty(fCustomPropertiesRef, "instance", PropertyOwner::kRTCOwner, lua::jbox_native_object{});

  // global_rtc
  addObject("/global_rtc");
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
void Motherboard::storeProperty(TJBox_PropertyRef const &iProperty, TJBox_Value const &iValue)
{
  handlePropertyDiff(fJboxObjects.get(iProperty.fObject)->storeValue(iProperty.fKey, iValue));
}

//------------------------------------------------------------------------
// Motherboard::storeProperty
//------------------------------------------------------------------------
void Motherboard::storeProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag, TJBox_Value const &iValue)
{
  handlePropertyDiff(getObject(iObject)->storeValue(iTag, iValue));
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
      fCurrentFramePropertyDiffs.emplace_back(*iPropertyDiff);
  }
}

//------------------------------------------------------------------------
// Motherboard::create
//------------------------------------------------------------------------
std::unique_ptr<Motherboard> Motherboard::create(int iInstanceId, int iSampleRate)
{
  auto res = std::unique_ptr<Motherboard>(new Motherboard());

  auto environment = res->addObject("/environment");

  // /environment/instance_id
  auto idProp = std::make_shared<lua::jbox_number_property>();
  idProp->property_tag = kJBox_EnvironmentInstanceID;
  idProp->default_value = iInstanceId;
  res->addProperty(environment->fObjectRef, "instance_id", PropertyOwner::kHostOwner, idProp);

  // /environment/system_sample_rate
  auto sampleRateProp = std::make_shared<lua::jbox_number_property>();
  sampleRateProp->property_tag = kJBox_EnvironmentSystemSampleRate;
  sampleRateProp->default_value = iSampleRate;
  res->addProperty(environment->fObjectRef, "system_sample_rate", PropertyOwner::kHostOwner, sampleRateProp);

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
  MockJBoxVisitor rtcVisitor{*fRealtimeController.get()};
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
    for(auto const &input: inputs->names)
      addAudioInput(input);
  }

  // audio_outputs
  {
    auto outputs = def.getAudioOutputs();
    for(auto const &output: outputs->names)
      addAudioOutput(output);
  }

  // cv_inputs
  {
    auto inputs = def.getCVInputs();
    for(auto const &input: inputs->names)
      addCVInput(input);
  }

  // cv_outputs
  {
    auto outputs = def.getCVOutputs();
    for(auto const &output: outputs->names)
      addCVOutput(output);
  }

  auto customProperties = def.getCustomProperties();

  // document_owner.properties
  for(auto &&prop: customProperties->document_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kDocOwner, prop.second);

  // rt_owner.properties
  for(auto &&prop: customProperties->rt_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kRTOwner, prop.second);

  // rtc_owner.properties
  for(auto &&prop: customProperties->rtc_owner)
    addProperty(fCustomPropertiesRef, prop.first, PropertyOwner::kRTCOwner, prop.second);

  // rt_input_setup.notify
  for(auto &&propertyPath: fRealtimeController->getRTInputSetupNotify())
  {
    registerRTCNotify(propertyPath);
  }

  // rtc_bindings
  auto bindings1 = fRealtimeController->getBindings();
  auto bindings2 = fRealtimeController->getBindings();
  for(auto &&[propertyPath, bindingKey]: fRealtimeController->getBindings())
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
  struct DefaultValueVisitor
  {

    DefaultValueVisitor(Motherboard *motherboard) : fMotherboard(motherboard) {}

    TJBox_Value operator()(std::shared_ptr<lua::jbox_boolean_property> o) { return JBox_MakeBoolean(o->default_value); }
    TJBox_Value operator()(std::shared_ptr<lua::jbox_number_property> o) { return JBox_MakeNumber(o->default_value); }
    TJBox_Value operator()(std::shared_ptr<lua::jbox_native_object> o) {
      if(!o->default_value.operation.empty())
        return fMotherboard->makeNativeObjectRW(o->default_value.operation, o->default_value.params);
      else
        return JBox_MakeNil();
    }
    Motherboard *fMotherboard;
  };

  auto propertyTag = std::visit([](auto &p) { return p->property_tag; }, iProperty);
  auto defaultValue = std::visit(DefaultValueVisitor{this}, iProperty);

  fJboxObjects.get(iParentObject)->addProperty(iPropertyName,
                                               iOwner,
                                               defaultValue,
                                               propertyTag);
}

//------------------------------------------------------------------------
// Motherboard::registerRTCNotify
//------------------------------------------------------------------------
void Motherboard::registerRTCNotify(std::string const &iPropertyPath)
{
  if(iPropertyPath.find_last_of('*') != std::string::npos)
    throw Exception(fmt::printf("wildcard properties not implemented yet: [%s]", iPropertyPath));

  auto ref = getPropertyRef(iPropertyPath);
  fCurrentFramePropertyDiffs.emplace_back(fJboxObjects.get(ref.fObject)->watchPropertyForChange(ref.fKey));
  fRTCNotify.emplace(ref);
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
// Motherboard::addAudioInput
//------------------------------------------------------------------------
void Motherboard::addAudioInput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_inputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_AudioInputConnected);
  o->addProperty("buffer", PropertyOwner::kHostOwner, createDSPBuffer(), kJBox_AudioInputBuffer);
}

//------------------------------------------------------------------------
// Motherboard::addAudioOutput
//------------------------------------------------------------------------
void Motherboard::addAudioOutput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_outputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_AudioOutputConnected);
  o->addProperty("dsp_latency", PropertyOwner::kRTCOwner, JBox_MakeNumber(0), kJBox_AudioOutputDSPLatency);
  o->addProperty("buffer", PropertyOwner::kHostOwner, createDSPBuffer(), kJBox_AudioOutputBuffer);
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
  if(fRealtime.render_realtime)
    fRealtime.render_realtime(getInstance<void *>(),
                              fCurrentFramePropertyDiffs.data(),
                              fCurrentFramePropertyDiffs.size());

  fCurrentFramePropertyDiffs.clear();
}

//------------------------------------------------------------------------
// jbox_get_dsp_id
//------------------------------------------------------------------------
inline int jbox_get_dsp_id(TJBox_Value const &iJboxValue)
{
  return impl::jbox_get_value<int>(kJBox_DSPBuffer, iJboxValue);
}

//------------------------------------------------------------------------
// jbox_get_native_object_id
//------------------------------------------------------------------------
inline int jbox_get_native_object_id(TJBox_Value const &iJboxValue)
{
  return impl::jbox_get_value<int>(kJBox_NativeObject, iJboxValue);
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
  return {.fSampleCount = DSP_BUFFER_SIZE};
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
        .fNativeObject = nativeObject,
        .fOperation = iOperation,
        .fParams = iParams,
        .fDeleter = fRealtime.destroy_native_object,
        .fAccessMode = iAccessMode
      });
      return impl::jbox_make_value<int>(kJBox_NativeObject, fNativeObjects.add(std::move(uno)));
    }
  }
  return JBox_MakeNil();
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
void impl::JboxObject::addProperty(std::string iPropertyName,
                                   PropertyOwner iOwner,
                                   TJBox_Value const &iInitialValue,
                                   TJBox_Tag iPropertyTag)
{
  RE_MOCK_ASSERT(fProperties.find(iPropertyName) == fProperties.end(), "duplicate property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
  fProperties[iPropertyName] =
    std::make_unique<JboxProperty>(JBox_MakePropertyRef(fObjectRef, iPropertyName.c_str()),
                                   fmt::printf("%s/%s", fObjectPath, iPropertyName),
                                   iOwner,
                                   iInitialValue,
                                   iPropertyTag);
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
// JboxProperty::JboxProperty
//------------------------------------------------------------------------
impl::JboxProperty::JboxProperty(TJBox_PropertyRef const &iPropertyRef, std::string const &iPropertyPath, PropertyOwner iOwner, TJBox_Value const &iInitialValue, TJBox_Tag iTag) :
  fPropertyRef{iPropertyRef},
  fPropertyPath{iPropertyPath},
  fOwner{iOwner},
  fTag{iTag},
  fValue{iInitialValue}
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
      .fPreviousValue = previousValue,
      .fCurrentValue = fValue,
      .fPropertyRef = fPropertyRef,
      .fPropertyTag = fTag
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
    .fPreviousValue = fValue,
    .fCurrentValue = fValue,
    .fPropertyRef = fPropertyRef,
    .fPropertyTag = fTag
  };
}

}

