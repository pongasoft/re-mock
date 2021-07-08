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
  LOG_F(ERROR, "Fatal Error at %s:%d", message.filename, message.line);
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
TJBox_Value Motherboard::loadProperty(TJBox_PropertyRef iProperty)
{
  auto jboxObject = fJboxObjects.find(iProperty.fObject);
  CHECK_F(jboxObject != fJboxObjects.end(), "Could not load property [%s]: Parent not found [%i] (did you configure it?)", iProperty.fKey, iProperty.fObject);
  return jboxObject->second->loadValue(iProperty.fKey);
}

//------------------------------------------------------------------------
// Motherboard::storeProperty
//------------------------------------------------------------------------
void Motherboard::storeProperty(TJBox_PropertyRef iProperty, TJBox_Value const &iValue)
{
  auto jboxObject = fJboxObjects.find(iProperty.fObject);
  CHECK_F(jboxObject != fJboxObjects.end(), "Could not store property [%s]: Parent not found [%i] (did you configure it?)", iProperty.fKey, iProperty.fObject);
  jboxObject->second->storeValue(iProperty.fKey, iValue);
}

//------------------------------------------------------------------------
// Motherboard::init
//------------------------------------------------------------------------
std::unique_ptr<Motherboard> Motherboard::init(std::function<void(MotherboardDef &)> iConfigFunction)
{
  loguru::set_fatal_handler(loguru_fatal_handler);
  auto res = std::unique_ptr<Motherboard>(new Motherboard());

  sThreadLocalInstance = res.get();

  MotherboardDef config{};
  iConfigFunction(config);

  for(auto &&input: config.audio_inputs)
    res->addAudioInput(input.first);

  for(auto &&output: config.audio_outputs)
    res->addAudioOutput(output.first);

  for(auto &&input: config.cv_inputs)
    res->addCVInput(input.first);

  for(auto &&output: config.cv_outputs)
    res->addCVOutput(output.first);

  for(auto &&prop: config.document_owner_properties)
    res->addProperty(res->fCustomPropertiesRef, prop.first, PropertyOwner::kDocOwner, *prop.second);

  for(auto &&prop: config.rt_owner_properties)
    res->addProperty(res->fCustomPropertiesRef, prop.first, PropertyOwner::kRTOwner, *prop.second);

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
// Motherboard::addAudioInput
//------------------------------------------------------------------------
void Motherboard::addAudioInput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_inputs/%s", iSocketName.c_str()));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_AudioInputConnected);
  // buffer / PropertyOwner::kHostOwner / kJBox_AudioInputBuffer
}

//------------------------------------------------------------------------
// Motherboard::addAudioOutput
//------------------------------------------------------------------------
void Motherboard::addAudioOutput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/audio_outputs/%s", iSocketName.c_str()));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_AudioOutputConnected);
  o->addProperty("dsp_latency", PropertyOwner::kRTCOwner, JBox_MakeNumber(0), kJBox_AudioOutputDSPLatency);
  // buffer / PropertyOwner::kHostOwner / kJBox_AudioOutputBuffer
}

//------------------------------------------------------------------------
// Motherboard::addCVInput
//------------------------------------------------------------------------
void Motherboard::addCVInput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/cv_inputs/%s", iSocketName.c_str()));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_CVInputConnected);
  o->addProperty("value", PropertyOwner::kHostOwner, JBox_MakeNumber(0), kJBox_CVInputValue);
}

//------------------------------------------------------------------------
// Motherboard::addCVOutput
//------------------------------------------------------------------------
void Motherboard::addCVOutput(std::string const &iSocketName)
{
  auto o = addObject(fmt::printf("/cv_outputs/%s", iSocketName.c_str()));
  o->addProperty("connected", PropertyOwner::kHostOwner, JBox_MakeBoolean(false), kJBox_CVOutputConnected);
  o->addProperty("dsp_latency", PropertyOwner::kRTCOwner, JBox_MakeNumber(0), kJBox_CVOutputDSPLatency);
  o->addProperty("value", PropertyOwner::kRTOwner, JBox_MakeNumber(0), kJBox_CVOutputValue);
}


//------------------------------------------------------------------------
// Motherboard::connectSocket
//------------------------------------------------------------------------
void Motherboard::connectSocket(std::string const &iSocketPath)
{
  getObject(iSocketPath)->storeValue("connected", JBox_MakeBoolean(true));
}

//------------------------------------------------------------------------
// Motherboard::getObject
//------------------------------------------------------------------------
impl::JboxObject *Motherboard::getObject(std::string const &iObjectPath) const
{
  auto const objectRef = getObjectRef(iObjectPath);
  CHECK_F(fJboxObjects.find(objectRef) != fJboxObjects.end(), "missing object [%s]", iObjectPath.c_str());
  return fJboxObjects.at(objectRef).get();
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

static std::atomic<TJBox_ObjectRef> sObjectRefCounter{1};

//------------------------------------------------------------------------
// JboxObject::JboxObject
//------------------------------------------------------------------------
impl::JboxObject::JboxObject(std::string const &iObjectPath) :
  fObjectPath{iObjectPath}, fObjectRef{sObjectRefCounter++} {}

//------------------------------------------------------------------------
// JboxObject::loadValue
//------------------------------------------------------------------------
TJBox_Value impl::JboxObject::loadValue(std::string const &iPropertyName) const
{
  CHECK_F(fProperties.find(iPropertyName) != fProperties.end(), "missing property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
  return fProperties.at(iPropertyName)->loadValue();
}

//------------------------------------------------------------------------
// JboxObject::storeValue
//------------------------------------------------------------------------
void impl::JboxObject::storeValue(std::string const &iPropertyName, TJBox_Value const &iValue)
{
  CHECK_F(fProperties.find(iPropertyName) != fProperties.end(), "missing property [%s] for object [%s]", iPropertyName.c_str(), fObjectPath.c_str());
  fProperties[iPropertyName]->storeValue(iValue);
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
    std::make_unique<JboxProperty>(fmt::printf("%s/%s", fObjectPath.c_str(), iPropertyName.c_str()), iOwner, iInitialValue, iPropertyTag);
}


//------------------------------------------------------------------------
// JboxProperty::JboxProperty
//------------------------------------------------------------------------
impl::JboxProperty::JboxProperty(std::string const &iPropertyPath, PropertyOwner iOwner, TJBox_Value const &iInitialValue, TJBox_Tag iTag) :
  fPropertyPath{iPropertyPath},
  fOwner{iOwner},
  fTag{iTag},
  fValue{iInitialValue}
{}

//------------------------------------------------------------------------
// JboxProperty::storeValue
//------------------------------------------------------------------------
void impl::JboxProperty::storeValue(TJBox_Value const &iValue)
{
  CHECK_F(iValue.fSecret[0] == fValue.fSecret[0], "invalid property type for [%s]", fPropertyPath.c_str());
  fValue = iValue;
}

}

