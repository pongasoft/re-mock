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

static thread_local Motherboard *sThreadLocalInstance{};

// Handle loguru fatal error by throwing an exception (testable)
void loguru_fatal_handler(const loguru::Message& message)
{
  LOG_F(ERROR, "Fatal Error at %s:%d | %s", message.filename, message.line, message.message);
  throw Error(message.message);
}

//------------------------------------------------------------------------
// Motherboard::instance
//------------------------------------------------------------------------
Motherboard &Motherboard::instance()
{
  CHECK_F(sThreadLocalInstance != nullptr, "You must create a local instance of the motherboard: ThreadLocalMotherboard motherboard{};");
  return *sThreadLocalInstance;
}

//------------------------------------------------------------------------
// Motherboard::Motherboard
//------------------------------------------------------------------------
Motherboard::Motherboard()
{
//  DLOG_F(INFO, "Motherboard(%p)", this);
  fCustomPropertiesRef = addObject("/custom_properties")->fObjectRef;
}

//------------------------------------------------------------------------
// Motherboard::~Motherboard
//------------------------------------------------------------------------
Motherboard::~Motherboard()
{
//  DLOG_F(INFO, "~Motherboard(%p)", this);
  sThreadLocalInstance = nullptr;
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
TJBox_Tag Motherboard::getPropertyTag(TJBox_PropertyRef iPropertyRef) const
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
  auto jboxObject = fJboxObjects.find(iProperty.fObject);
  CHECK_F(jboxObject != fJboxObjects.end(), "Could not load property [%s]: Parent not found [%i] (did you configure it?)", iProperty.fKey, iProperty.fObject);
  return jboxObject->second->loadValue(iProperty.fKey);
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
  auto jboxObject = fJboxObjects.find(iProperty.fObject);
  CHECK_F(jboxObject != fJboxObjects.end(), "Could not store property [%s]: Parent not found [%i] (did you configure it?)", iProperty.fKey, iProperty.fObject);
  auto diff = jboxObject->second->storeValue(iProperty.fKey, iValue);
  if(diff)
    fCurrentFramePropertyDiffs.emplace_back(*diff);
}

//------------------------------------------------------------------------
// Motherboard::storeProperty
//------------------------------------------------------------------------
void Motherboard::storeProperty(TJBox_ObjectRef iObject, TJBox_Tag iTag, TJBox_Value const &iValue)
{
  auto diff = getObject(iObject)->storeValue(iTag, iValue);
  if(diff)
    fCurrentFramePropertyDiffs.emplace_back(*diff);
}

//------------------------------------------------------------------------
// Motherboard::init
//------------------------------------------------------------------------
std::unique_ptr<Motherboard> Motherboard::init(std::function<void(MotherboardDef &, RealtimeController &)> iConfigFunction)
{
  loguru::set_fatal_handler(loguru_fatal_handler);
  auto res = std::unique_ptr<Motherboard>(new Motherboard());

  sThreadLocalInstance = res.get();

  MotherboardDef motherboardDef{};
  RealtimeController realtimeController{};
  iConfigFunction(motherboardDef, realtimeController);

  for(auto &&input: motherboardDef.audio_inputs)
    res->addAudioInput(input.first);

  for(auto &&output: motherboardDef.audio_outputs)
    res->addAudioOutput(output.first);

  for(auto &&input: motherboardDef.cv_inputs)
    res->addCVInput(input.first);

  for(auto &&output: motherboardDef.cv_outputs)
    res->addCVOutput(output.first);

  for(auto &&prop: motherboardDef.document_owner.properties)
    res->addProperty(res->fCustomPropertiesRef, prop.first, PropertyOwner::kDocOwner, *prop.second);

  for(auto &&prop: motherboardDef.rt_owner.properties)
    res->addProperty(res->fCustomPropertiesRef, prop.first, PropertyOwner::kRTOwner, *prop.second);

  for(auto &&propertyPath: realtimeController.rt_input_setup.notify)
  {
    res->registerNotifiableProperty(propertyPath);
  }

  return res;
}

//------------------------------------------------------------------------
// Motherboard::addProperty
//------------------------------------------------------------------------
void Motherboard::addProperty(TJBox_ObjectRef iParentObject, std::string const &iPropertyName, PropertyOwner iOwner, jbox_property const &iProperty)
{
  fJboxObjects[iParentObject]->addProperty(iPropertyName, iOwner, iProperty.default_value, iProperty.property_tag);
}

//------------------------------------------------------------------------
// Motherboard::registerNotifiableProperty
//------------------------------------------------------------------------
void Motherboard::registerNotifiableProperty(std::string const &iPropertyPath)
{
  if(iPropertyPath.find_last_of('*') != std::string::npos)
    throw Error(fmt::printf("wildcard properties not implemented yet: [%s]", iPropertyPath));

  auto ref = getPropertyRef(iPropertyPath);
  fCurrentFramePropertyDiffs.emplace_back(fJboxObjects[ref.fObject]->watchPropertyForChange(ref.fKey));
}

//------------------------------------------------------------------------
// Motherboard::addAudioInput
//------------------------------------------------------------------------
void Motherboard::addAudioInput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_inputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_AudioInputConnected);
  // buffer / PropertyOwner::kHostOwner / kJBox_AudioInputBuffer
}

//------------------------------------------------------------------------
// Motherboard::addAudioOutput
//------------------------------------------------------------------------
void Motherboard::addAudioOutput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_outputs/%s", iSocketName));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_AudioOutputConnected);
  o->addProperty("dsp_latency", PropertyOwner::kRTCOwner, JBox_MakeNumber(0), kJBox_AudioOutputDSPLatency);
  // buffer / PropertyOwner::kHostOwner / kJBox_AudioOutputBuffer
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
  CHECK_F(fJboxObjects.find(iObjectRef) != fJboxObjects.end(), "missing object [%d]", iObjectRef);
  return fJboxObjects.at(iObjectRef).get();
}

//------------------------------------------------------------------------
// Motherboard::addObject
//------------------------------------------------------------------------
impl::JboxObject *Motherboard::addObject(std::string const &iObjectPath)
{
  auto s = std::make_unique<impl::JboxObject>(iObjectPath);
  CHECK_F(fJboxObjectRefs.find(iObjectPath) == fJboxObjectRefs.end(), "duplicate object [%s]", iObjectPath.c_str());
  fJboxObjectRefs[iObjectPath] = s->fObjectRef;
  auto res = s.get();
  fJboxObjects[s->fObjectRef] = std::move(s);
  return res;
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
// Motherboard::connectSocket
//------------------------------------------------------------------------
void Motherboard::connectSocket(std::string const &iSocketPath)
{
  getObject(iSocketPath)->storeValue("connected", JBox_MakeBoolean(true));
}

static std::atomic<TJBox_ObjectRef> sObjectRefCounter{1};

//------------------------------------------------------------------------
// JboxObject::JboxObject
//------------------------------------------------------------------------
impl::JboxObject::JboxObject(std::string const &iObjectPath) :
  fObjectPath{iObjectPath}, fObjectRef{sObjectRefCounter++} {}

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
  CHECK_F(iValue.fSecret[0] == fValue.fSecret[0], "invalid property type for [%s]", fPropertyPath.c_str());

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

