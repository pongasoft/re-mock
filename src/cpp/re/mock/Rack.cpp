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
std::shared_ptr<Rack::Extension> Rack::newExtension(Extension::Configuration iConfig)
{
  auto id = fExtensions.add([this, &iConfig](int id) {
    auto motherboard = Motherboard::create(fSampleRate, iConfig);
    return std::shared_ptr<Extension>(new Extension(id, this, std::move(motherboard)));
  });

  auto res = fExtensions.get(id);
  res->use([](Motherboard *m) { m->init(); });
  return res;
}

//------------------------------------------------------------------------
// Rack::nextFrame
//------------------------------------------------------------------------
void Rack::nextFrame()
{
  // first we copy out -> in
  for(auto &extension: fExtensions)
  {
    auto re = extension.second;

    for(auto &wire: re->fAudioWires)
      copyAudioBuffers(wire);

    for(auto &wire: re->fCVWires)
      copyCVValue(wire);
  }

  // once everything is copied, we can safely generate the next frame for each device
  for(auto &extension: fExtensions)
    extension.second->use([](Motherboard *m) { m->nextFrame(); });
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
  fExtensions.get(iOutSocket.fExtensionId)->wire(iOutSocket, iInSocket);
}

//------------------------------------------------------------------------
// Rack::wire
//------------------------------------------------------------------------
void Rack::wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket)
{
  fExtensions.get(iOutSocket.fExtensionId)->wire(iOutSocket, iInSocket);
}

//------------------------------------------------------------------------
// InternalThreadLocalRAII - to add/remove the "current" motherboard
//------------------------------------------------------------------------
struct InternalThreadLocalRAII
{
  explicit InternalThreadLocalRAII(Motherboard *iMotherboard) { sThreadLocalInstance = iMotherboard; }
  InternalThreadLocalRAII(InternalThreadLocalRAII &&) = delete;
  InternalThreadLocalRAII(InternalThreadLocalRAII const &) = delete;
  ~InternalThreadLocalRAII() { sThreadLocalInstance = nullptr; }
};

//------------------------------------------------------------------------
// Rack::Extension::use
//------------------------------------------------------------------------
void Rack::Extension::use(std::function<void(Motherboard *)> iCallback)
{
  InternalThreadLocalRAII raii{fMotherboard.get()};
  iCallback(fMotherboard.get());
}

//------------------------------------------------------------------------
// Rack::Extension::getAudioOutSocket
//------------------------------------------------------------------------
Rack::Extension::AudioOutSocket Rack::Extension::getAudioOutSocket(std::string const &iSocketName) const
{
  return {{{fId, fMotherboard->getObjectRef(fmt::printf("/audio_outputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::Extension::AudioInSocket
//------------------------------------------------------------------------
Rack::Extension::AudioInSocket Rack::Extension::getAudioInSocket(std::string const &iSocketName) const
{
  return {{{fId, fMotherboard->getObjectRef(fmt::printf("/audio_inputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::Extension::getCVOutSocket
//------------------------------------------------------------------------
Rack::Extension::CVOutSocket Rack::Extension::getCVOutSocket(std::string const &iSocketName) const
{
  return {{{fId, fMotherboard->getObjectRef(fmt::printf("/cv_outputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::Extension::getCVInSocket
//------------------------------------------------------------------------
Rack::Extension::CVInSocket Rack::Extension::getCVInSocket(std::string const &iSocketName) const
{
  return {{{fId, fMotherboard->getObjectRef(fmt::printf("/cv_inputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::Extension::wire
//------------------------------------------------------------------------
void Rack::Extension::wire(AudioOutSocket const &iOutSocket, AudioInSocket const &iInSocket)
{
  CHECK_F(iOutSocket.fExtensionId == fId); // sanity check... out MUST be a socket of this extension!!!
  auto newWire = AudioWire{.fFromSocket = iOutSocket, .fToSocket = iInSocket };
  CHECK_F(std::find_if(fAudioWires.begin(), fAudioWires.end(), [&newWire](auto &wire) {
    return Rack::Extension::AudioWire::overlap(wire, newWire);
  }) == fAudioWires.end(), "Audio socket in use");
  fAudioWires.emplace_back(newWire);
}

bool operator==(Rack::Extension::AudioOutSocket const &lhs, Rack::Extension::AudioOutSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

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
// Rack::Extension::wire
//------------------------------------------------------------------------
void Rack::Extension::wire(CVOutSocket const &iOutSocket, CVInSocket const &iInSocket)
{
  CHECK_F(iOutSocket.fExtensionId == fId); // sanity check... out MUST be a socket of this extension!!!
  auto newWire = CVWire{.fFromSocket = iOutSocket, .fToSocket = iInSocket };
  CHECK_F(std::find_if(fCVWires.begin(), fCVWires.end(), [&newWire](auto &wire) {
    return Rack::Extension::CVWire::overlap(wire, newWire);
  }) == fCVWires.end(), "CV socket in use");
  fCVWires.emplace_back(newWire);
}

bool operator==(Rack::Extension::CVOutSocket const &lhs, Rack::Extension::CVOutSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

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

}