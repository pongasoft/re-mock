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
#include <logging/logging.h>
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

  // /custom_properties/instance (for the "privateState")
  addProperty(fCustomPropertiesRef, "instance", PropertyOwner::kRTCOwner, {});

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
  CHECK_F(ref != fJboxObjectRefs.end(), "Could not find object [%s] (did you configure it?)", iObjectPath.c_str());
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
  CHECK_F(lastSlash != std::string::npos, "Invalid property path (missing /) [%s]", iPropertyPath.c_str());
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
      binding->second(getPropertyPath(iPropertyDiff->fPropertyRef), iPropertyDiff->fCurrentValue);

    if(fRTCNofify.find(iPropertyDiff->fPropertyRef) != fRTCNofify.end())
      fCurrentFramePropertyDiffs.emplace_back(*iPropertyDiff);
  }
}

//------------------------------------------------------------------------
// Motherboard::create
//------------------------------------------------------------------------
std::unique_ptr<Motherboard> Motherboard::create(int iSampleRate, Configuration iConfigFunction)
{
  auto res = std::unique_ptr<Motherboard>(new Motherboard());

  MotherboardDef motherboardDef{};
  RealtimeController realtimeController{};
  if(iConfigFunction)
    iConfigFunction(motherboardDef, realtimeController, res->fRealtime);

  // /environment/system_sample_rate
  auto environment = res->addObject("/environment");
  res->addProperty(environment->fObjectRef, "system_sample_rate", PropertyOwner::kHostOwner,
                   { .property_tag = kJBox_EnvironmentSystemSampleRate, .default_value = JBox_MakeNumber(iSampleRate) });

  // audio_inputs
  for(auto &&input: motherboardDef.audio_inputs)
    res->addAudioInput(input.first);

  // audio_outputs
  for(auto &&output: motherboardDef.audio_outputs)
    res->addAudioOutput(output.first);

  // cv_inputs
  for(auto &&input: motherboardDef.cv_inputs)
    res->addCVInput(input.first);

  // cv_outputs
  for(auto &&output: motherboardDef.cv_outputs)
    res->addCVOutput(output.first);

  // document_owner.properties
  for(auto &&prop: motherboardDef.document_owner.properties)
    res->addProperty(res->fCustomPropertiesRef, prop.first, PropertyOwner::kDocOwner, *prop.second);

  // rt_owner.properties
  for(auto &&prop: motherboardDef.rt_owner.properties)
    res->addProperty(res->fCustomPropertiesRef, prop.first, PropertyOwner::kRTOwner, *prop.second);

  // rt_input_setup.notify
  for(auto &&propertyPath: realtimeController.rt_input_setup.notify)
  {
    res->registerRTCNotify(propertyPath);
  }

  // rtc_bindings
  for(auto &&[propertyPath, bindingKey]: realtimeController.rtc_bindings)
  {
    auto bindingRef = res->getPropertyRef(bindingKey);
    auto binding = realtimeController.global_rtc.find(bindingRef.fKey);
    CHECK_F(binding != realtimeController.global_rtc.end(), "Missing binding [%s] for [%s]", bindingKey.c_str(), propertyPath.c_str());
    res->registerRTCBinding(propertyPath, binding->second);
  }

  return res;
}

//------------------------------------------------------------------------
// Motherboard::init
//------------------------------------------------------------------------
void Motherboard::init()
{
  for(auto &&diff: fInitBindings)
  {
    fRTCBindings[diff.fPropertyRef](getPropertyPath(diff.fPropertyRef), diff.fCurrentValue);
  }
  fInitBindings.clear();
}

//------------------------------------------------------------------------
// Motherboard::addProperty
//------------------------------------------------------------------------
void Motherboard::addProperty(TJBox_ObjectRef iParentObject,
                              std::string const &iPropertyName,
                              PropertyOwner iOwner,
                              jbox_property const &iProperty)
{
  fJboxObjects.get(iParentObject)->addProperty(iPropertyName, iOwner, iProperty.default_value, iProperty.property_tag);
}

//------------------------------------------------------------------------
// Motherboard::registerRTCNotify
//------------------------------------------------------------------------
void Motherboard::registerRTCNotify(std::string const &iPropertyPath)
{
  if(iPropertyPath.find_last_of('*') != std::string::npos)
    throw Error(fmt::printf("wildcard properties not implemented yet: [%s]", iPropertyPath));

  auto ref = getPropertyRef(iPropertyPath);
  fCurrentFramePropertyDiffs.emplace_back(fJboxObjects.get(ref.fObject)->watchPropertyForChange(ref.fKey));
  fRTCNofify.emplace(ref);
}

//------------------------------------------------------------------------
// Motherboard::registerRTCBinding
//------------------------------------------------------------------------
void Motherboard::registerRTCBinding(std::string const &iPropertyPath, RTCCallback iCallback)
{
  auto ref = getPropertyRef(iPropertyPath);
  fInitBindings.emplace_back(fJboxObjects.get(ref.fObject)->watchPropertyForChange(ref.fKey));
  fRTCBindings[ref] = std::move(iCallback);
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
void Motherboard::nextFrame(std::function<void(TJBox_PropertyDiff const *, TJBox_UInt32)> iNextFrameCallback)
{
  iNextFrameCallback(fCurrentFramePropertyDiffs.data(), fCurrentFramePropertyDiffs.size());
  fCurrentFramePropertyDiffs.clear();
}

//------------------------------------------------------------------------
// Motherboard::nextFrame
//------------------------------------------------------------------------
void Motherboard::nextFrame()
{
  if(fRealtime.render_realtime)
  {
    auto callback = [this](const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) {
      fRealtime.render_realtime(getInstance<void *>(), iPropertyDiffs, iDiffCount);
    };
    nextFrame(std::move(callback));
  }
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
  CHECK_F(iStartFrame >= 0 && iEndFrame >= 0 && iEndFrame <= DSP_BUFFER_SIZE && iStartFrame <= iEndFrame);
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
  CHECK_F(iStartFrame >= 0 && iEndFrame >= 0 && iEndFrame <= DSP_BUFFER_SIZE && iStartFrame <= iEndFrame);
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
// Motherboard::makeNativeObjectRW
//------------------------------------------------------------------------
TJBox_Value Motherboard::makeNativeObjectRW(std::string const &iOperation, std::vector<TJBox_Value> const &iParams)
{
  if(fRealtime.create_native_object)
  {
    auto nativeObject = fRealtime.create_native_object(iOperation.c_str(), iParams.data(), iParams.size());
    if(nativeObject)
    {
      return impl::jbox_make_value<int>(kJBox_NativeObject, fNativeObjects.add((void *) nativeObject));
    }
  }
  return JBox_MakeNil();
}

//------------------------------------------------------------------------
// Motherboard::getNativeObjectRW
//------------------------------------------------------------------------
void *Motherboard::getNativeObjectRW(TJBox_Value iValue) const
{
  return fNativeObjects.get(jbox_get_native_object_id(iValue));
}

//------------------------------------------------------------------------
// Motherboard::connectSocket
//------------------------------------------------------------------------
void Motherboard::connectSocket(TJBox_ObjectRef iSocket)
{
  fJboxObjects.get(iSocket)->storeValue("connected", JBox_MakeBoolean(true));
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
  fJboxObjects.get(iCVSocket)->storeValue("value", JBox_MakeNumber(iValue));
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
  CHECK_F(fProperties.find(iPropertyName) != fProperties.end(), "missing property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
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
  CHECK_F(iter != fProperties.end(), "missing property tag [%d] for object [%s]", iPropertyTag, fObjectPath.c_str());
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
  CHECK_F(fProperties.find(iPropertyName) == fProperties.end(), "duplicate property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
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
  CHECK_F(fProperties.find(iPropertyName) != fProperties.end(), "missing property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
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
  CHECK_F(iValue.fSecret[0] == TJBox_ValueType::kJBox_Nil ||
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

