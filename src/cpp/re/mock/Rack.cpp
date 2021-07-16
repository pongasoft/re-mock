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

#include <logging/logging.h>
#include "Rack.h"
#include "stl.h"

namespace re::mock {

static thread_local Motherboard *sThreadLocalInstance{};

// Handle loguru fatal error by throwing an exception (testable)
void loguru_fatal_handler(const loguru::Message& message)
{
  LOG_F(ERROR, "Fatal Error at %s:%d | %s", message.filename, message.line, message.message);
  throw Error(message.message);
}

//------------------------------------------------------------------------
// Rack::currentMotherboard
//------------------------------------------------------------------------
Motherboard &Rack::currentMotherboard()
{
  CHECK_F(sThreadLocalInstance != nullptr, "Not called in the context of a device. You should call rack.useRE(...)!");
  return *sThreadLocalInstance;
}

//------------------------------------------------------------------------
// Rack::Rack
//------------------------------------------------------------------------
Rack::Rack(int iSampleRate) : fSampleRate{iSampleRate}
{
  // set up loguru
  loguru::set_fatal_handler(loguru_fatal_handler);
}

//------------------------------------------------------------------------
// Rack::newExtension
//------------------------------------------------------------------------
Rack::Extension Rack::newExtension(Extension::Configuration iConfig)
{
  auto id = fExtensions.add([this, &iConfig](int id) {
    auto motherboard = Motherboard::create(fSampleRate, iConfig);
    return std::shared_ptr<ExtensionImpl>(new ExtensionImpl(id, this, std::move(motherboard)));
  });

  auto res = fExtensions.get(id);
  res->use([](Motherboard *m) { m->init(); });
  return Rack::Extension{res};
}

//------------------------------------------------------------------------
// Rack::nextFrame
//------------------------------------------------------------------------
void Rack::nextFrame()
{
  std::set<int> processedExtensions{};

  for(auto &extension: fExtensions)
  {
    nextFrame(*extension.second, processedExtensions);
  }
}

//------------------------------------------------------------------------
// Rack::nextFrame
//------------------------------------------------------------------------
void Rack::nextFrame(ExtensionImpl &iExtension, std::set<int> &iProcessedExtensions)
{
  if(stl::contains(iProcessedExtensions, iExtension.fId))
    // already processed
    return;

  // we start by adding it to break any cycle
  iProcessedExtensions.emplace(iExtension.fId);

  // we process all dependent extensions first
  for(auto id: iExtension.getDependents())
  {
    nextFrame(*fExtensions.get(id), iProcessedExtensions);
  }

  // we process the extension
  nextFrame(iExtension);
}

//------------------------------------------------------------------------
// Rack::nextFrame
//------------------------------------------------------------------------
void Rack::nextFrame(ExtensionImpl &iExtension)
{
  iExtension.use([](Motherboard *m) { m->nextFrame(); });

  for(auto &wire: iExtension.fAudioWires)
    copyAudioBuffers(wire);

  for(auto &wire: iExtension.fCVWires)
    copyCVValue(wire);
}

//------------------------------------------------------------------------
// Rack::copyAudioBuffers
//------------------------------------------------------------------------
void Rack::copyAudioBuffers(Extension::AudioWire const &iWire)
{
  auto outExtension = fExtensions.get(iWire.fFromSocket.fExtensionId);
  auto inExtension = fExtensions.get(iWire.fToSocket.fExtensionId);

  auto buffer = outExtension->fMotherboard->getDSPBuffer(iWire.fFromSocket.fSocketRef);
  inExtension->fMotherboard->setDSPBuffer(iWire.fToSocket.fSocketRef, buffer);
}

//------------------------------------------------------------------------
// Rack::copyCVValue
//------------------------------------------------------------------------
void Rack::copyCVValue(Extension::CVWire const &iWire)
{
  auto outExtension = fExtensions.get(iWire.fFromSocket.fExtensionId);
  auto inExtension = fExtensions.get(iWire.fToSocket.fExtensionId);

  auto value = outExtension->fMotherboard->getCVSocketValue(iWire.fFromSocket.fSocketRef);
  inExtension->fMotherboard->setCVSocketValue(iWire.fToSocket.fSocketRef, value);
}

//------------------------------------------------------------------------
// Rack::wire
//------------------------------------------------------------------------
void Rack::wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket)
{
  auto extension = fExtensions.get(iInSocket.fExtensionId);
  extension->fMotherboard->connectSocket(iInSocket.fSocketRef);
  extension->wire(iOutSocket, iInSocket);

  extension = fExtensions.get(iOutSocket.fExtensionId);
  extension->fMotherboard->connectSocket(iOutSocket.fSocketRef);

  // case when an extension is connected to itself
  if(iInSocket.fExtensionId != iOutSocket.fExtensionId)
  {
    extension->wire(iOutSocket, iInSocket);
  }
}

//------------------------------------------------------------------------
// Rack::wire
//------------------------------------------------------------------------
void Rack::wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket)
{
  auto extension = fExtensions.get(iInSocket.fExtensionId);
  extension->fMotherboard->connectSocket(iInSocket.fSocketRef);
  extension->wire(iOutSocket, iInSocket);

  extension = fExtensions.get(iOutSocket.fExtensionId);
  extension->fMotherboard->connectSocket(iOutSocket.fSocketRef);

  // case when an extension is connected to itself
  if(iInSocket.fExtensionId != iOutSocket.fExtensionId)
  {
    extension->wire(iOutSocket, iInSocket);
  }
}

//------------------------------------------------------------------------
// InternalThreadLocalRAII - to add/remove the "current" motherboard
//------------------------------------------------------------------------
struct InternalThreadLocalRAII
{
  explicit InternalThreadLocalRAII(Motherboard *iMotherboard) : fPrevious{sThreadLocalInstance} { sThreadLocalInstance = iMotherboard; }
  InternalThreadLocalRAII(InternalThreadLocalRAII &&) = delete;
  InternalThreadLocalRAII(InternalThreadLocalRAII const &) = delete;
  ~InternalThreadLocalRAII() { sThreadLocalInstance = fPrevious; }
private:
  Motherboard *fPrevious;
};

//------------------------------------------------------------------------
// Rack::ExtensionImpl::use
//------------------------------------------------------------------------
void Rack::ExtensionImpl::use(std::function<void(Motherboard *)> iCallback)
{
  InternalThreadLocalRAII raii{fMotherboard.get()};
  iCallback(fMotherboard.get());
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::getAudioOutSocket
//------------------------------------------------------------------------
Rack::Extension::AudioOutSocket Rack::ExtensionImpl::getAudioOutSocket(std::string const &iSocketName) const
{
  return {{{fId, fMotherboard->getObjectRef(fmt::printf("/audio_outputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::AudioInSocket
//------------------------------------------------------------------------
Rack::Extension::AudioInSocket Rack::ExtensionImpl::getAudioInSocket(std::string const &iSocketName) const
{
  return {{{fId, fMotherboard->getObjectRef(fmt::printf("/audio_inputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::getCVOutSocket
//------------------------------------------------------------------------
Rack::Extension::CVOutSocket Rack::ExtensionImpl::getCVOutSocket(std::string const &iSocketName) const
{
  return {{{fId, fMotherboard->getObjectRef(fmt::printf("/cv_outputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::getCVInSocket
//------------------------------------------------------------------------
Rack::Extension::CVInSocket Rack::ExtensionImpl::getCVInSocket(std::string const &iSocketName) const
{
  return {{{fId, fMotherboard->getObjectRef(fmt::printf("/cv_inputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::wire
//------------------------------------------------------------------------
void Rack::ExtensionImpl::wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket)
{
  CHECK_F(iInSocket.fExtensionId == fId || iOutSocket.fExtensionId == fId); // sanity check...
  auto newWire = Extension::AudioWire{.fFromSocket = iOutSocket, .fToSocket = iInSocket };

  // check for duplicate
  CHECK_F(!stl::find_if(fAudioWires, [&newWire](auto &wire) { return Rack::Extension::AudioWire::overlap(wire, newWire); }), "Audio socket in use");

  if(iOutSocket.fExtensionId == fId)
    fAudioWires.emplace_back(newWire);
  if(iOutSocket.fExtensionId != fId)
    fDependents.emplace(iOutSocket.fExtensionId);
}

//------------------------------------------------------------------------
// Rack::Extension::AudioOutSocket ==
//------------------------------------------------------------------------
bool operator==(Rack::Extension::AudioOutSocket const &lhs, Rack::Extension::AudioOutSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

//------------------------------------------------------------------------
// Rack::Extension::AudioInSocket ==
//------------------------------------------------------------------------
bool operator==(Rack::Extension::AudioInSocket const &lhs, Rack::Extension::AudioInSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

//------------------------------------------------------------------------
// Extension::AudioWire::overlap
//------------------------------------------------------------------------
bool Rack::Extension::AudioWire::overlap(AudioWire const &iWire1, AudioWire const &iWire2)
{
  return iWire1.fFromSocket == iWire2.fFromSocket || iWire1.fToSocket == iWire2.fToSocket;
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::wire
//------------------------------------------------------------------------
void Rack::ExtensionImpl::wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket)
{
  CHECK_F(iInSocket.fExtensionId == fId || iOutSocket.fExtensionId == fId); // sanity check...
  auto newWire = Extension::CVWire{.fFromSocket = iOutSocket, .fToSocket = iInSocket };

  // check for duplicate
  CHECK_F(!stl::find_if(fCVWires, [&newWire](auto &wire) { return Rack::Extension::CVWire::overlap(wire, newWire); }), "CV socket in use");

  if(iOutSocket.fExtensionId == fId)
    fCVWires.emplace_back(newWire);
  if(iOutSocket.fExtensionId != fId)
    fDependents.emplace(iOutSocket.fExtensionId);
}

//------------------------------------------------------------------------
// Rack::Extension::CVOutSocket ==
//------------------------------------------------------------------------
bool operator==(Rack::Extension::CVOutSocket const &lhs, Rack::Extension::CVOutSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

//------------------------------------------------------------------------
// Rack::Extension::CVInSocket ==
//------------------------------------------------------------------------
bool operator==(Rack::Extension::CVInSocket const &lhs, Rack::Extension::CVInSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

//------------------------------------------------------------------------
// Extension::CVWire::overlap
//------------------------------------------------------------------------
bool Rack::Extension::CVWire::overlap(CVWire const &iWire1, CVWire const &iWire2)
{
  return iWire1.fFromSocket == iWire2.fFromSocket || iWire1.fToSocket == iWire2.fToSocket;
}

//------------------------------------------------------------------------
// RealtimeController::defaultBindings - defined here because depends on jbox
//------------------------------------------------------------------------
RealtimeController RealtimeController::byDefault()
{
  RealtimeController rtc{};

  rtc.rtc_bindings["/environment/system_sample_rate"] = "/global_rtc/init_instance";

  rtc.global_rtc["init_instance"] = [](std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue) {
    auto sample_rate = jbox.load_property("/environment/system_sample_rate");
    auto new_no = jbox.make_native_object_rw("Instance", { sample_rate });
    jbox.store_property("/custom_properties/instance", new_no);
  };

  return rtc;
}

}