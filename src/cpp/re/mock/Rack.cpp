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

#include "Rack.h"
#include "stl.h"

namespace re::mock {

static thread_local Motherboard *sThreadLocalInstance{};

//------------------------------------------------------------------------
// Rack::currentMotherboard
//------------------------------------------------------------------------
Motherboard &Rack::currentMotherboard()
{
  RE_MOCK_ASSERT(sThreadLocalInstance != nullptr, "Not called in the context of a device. You should call re.use(...)!");
  return *sThreadLocalInstance;
}

//------------------------------------------------------------------------
// Rack::Rack
//------------------------------------------------------------------------
Rack::Rack(int iSampleRate) : fSampleRate{iSampleRate}
{
}

//------------------------------------------------------------------------
// Rack::newExtension
//------------------------------------------------------------------------
Rack::Extension Rack::newExtension(Config const &iConfig)
{
  auto id = fExtensions.add([this](int id) {
    auto motherboard = Motherboard::create(id, fSampleRate);
    return std::shared_ptr<ExtensionImpl>(new ExtensionImpl(id, this, std::move(motherboard)));
  });

  auto res = fExtensions.get(id);
  res->use([&iConfig](Motherboard &m) { m.init(iConfig); });
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
  iExtension.use([](Motherboard &m) { m.nextFrame(); });

  for(auto &wire: iExtension.fAudioOutWires)
    copyAudioBuffers(wire);

  for(auto &wire: iExtension.fCVOutWires)
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
  auto inExtension = fExtensions.get(iInSocket.fExtensionId);
  inExtension->wire(iOutSocket, iInSocket);

  auto outExtension = fExtensions.get(iOutSocket.fExtensionId);
  // if the device is self connected, the connection was already established
  // with inExtension->wire(iOutSocket, iInSocket);
  if(iInSocket.fExtensionId != iOutSocket.fExtensionId)
    outExtension->wire(iOutSocket, iInSocket);

  inExtension->fMotherboard->connectSocket(iInSocket.fSocketRef);
  outExtension->fMotherboard->connectSocket(iOutSocket.fSocketRef);
}

//------------------------------------------------------------------------
// Rack::unwire
//------------------------------------------------------------------------
void Rack::unwire(Rack::Extension::AudioOutSocket const &iOutSocket)
{
  auto outExtension = fExtensions.get(iOutSocket.fExtensionId);
  auto inSocket = outExtension->unwire(iOutSocket);
  if(inSocket)
  {
    outExtension->fMotherboard->disconnectSocket(iOutSocket.fSocketRef);
    auto inExtension = fExtensions.get(inSocket->fExtensionId);
    inExtension->unwire(*inSocket);
    inExtension->fMotherboard->disconnectSocket(inSocket->fSocketRef);
  }
}

//------------------------------------------------------------------------
// Rack::wire
//------------------------------------------------------------------------
void Rack::wire(Extension::StereoAudioOutSocket const &iOutSocket, Extension::StereoAudioInSocket const &iInSocket)
{
  wire(iOutSocket.fLeft, iInSocket.fLeft);
  wire(iOutSocket.fRight, iInSocket.fRight);
}

//------------------------------------------------------------------------
// Rack::unwire
//------------------------------------------------------------------------
void Rack::unwire(Extension::StereoAudioOutSocket const &iOutSocket)
{
  unwire(iOutSocket.fLeft);
  unwire(iOutSocket.fRight);
}

//------------------------------------------------------------------------
// Rack::wire
//------------------------------------------------------------------------
void Rack::wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket)
{
  auto inExtension = fExtensions.get(iInSocket.fExtensionId);
  inExtension->wire(iOutSocket, iInSocket);

  auto outExtension = fExtensions.get(iOutSocket.fExtensionId);
  // if the device is self connected, the connection was already established
  // with inExtension->wire(iOutSocket, iInSocket);
  if(iInSocket.fExtensionId != iOutSocket.fExtensionId)
    outExtension->wire(iOutSocket, iInSocket);

  inExtension->fMotherboard->connectSocket(iInSocket.fSocketRef);
  outExtension->fMotherboard->connectSocket(iOutSocket.fSocketRef);
}

//------------------------------------------------------------------------
// Rack::unwire
//------------------------------------------------------------------------
void Rack::unwire(Rack::Extension::CVOutSocket const &iOutSocket)
{
  auto outExtension = fExtensions.get(iOutSocket.fExtensionId);
  auto inSocket = outExtension->unwire(iOutSocket);
  if(inSocket)
  {
    outExtension->fMotherboard->disconnectSocket(iOutSocket.fSocketRef);
    auto inExtension = fExtensions.get(inSocket->fExtensionId);
    inExtension->unwire(*inSocket);
    inExtension->fMotherboard->disconnectSocket(inSocket->fSocketRef);
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
void Rack::ExtensionImpl::use(std::function<void(Motherboard &)> iCallback)
{
  InternalThreadLocalRAII raii{fMotherboard.get()};
  iCallback(*fMotherboard.get());
}

//------------------------------------------------------------------------
// Rack::Extension::getAudioOutSocket
//------------------------------------------------------------------------
Rack::Extension::AudioOutSocket Rack::Extension::getAudioOutSocket(std::string const &iSocketName) const
{
  return {{{fImpl->fId, motherboard().getObjectRef(fmt::printf("/audio_outputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::Extension::AudioInSocket
//------------------------------------------------------------------------
Rack::Extension::AudioInSocket Rack::Extension::getAudioInSocket(std::string const &iSocketName) const
{
  return {{{fImpl->fId, motherboard().getObjectRef(fmt::printf("/audio_inputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::Extension::getCVOutSocket
//------------------------------------------------------------------------
Rack::Extension::CVOutSocket Rack::Extension::getCVOutSocket(std::string const &iSocketName) const
{
  return {{{fImpl->fId, motherboard().getObjectRef(fmt::printf("/cv_outputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::Extension::getCVInSocket
//------------------------------------------------------------------------
Rack::Extension::CVInSocket Rack::Extension::getCVInSocket(std::string const &iSocketName) const
{
  return {{{fImpl->fId, motherboard().getObjectRef(fmt::printf("/cv_inputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Rack::Extension::getStereoAudioOutSocket
//------------------------------------------------------------------------
Rack::Extension::StereoAudioOutSocket Rack::Extension::getStereoAudioOutSocket(std::string const &iLeftSocketName,
                                                                               std::string const &iRightSocketName) const
{
  return { getAudioOutSocket(iLeftSocketName), getAudioOutSocket(iRightSocketName) };
}

//------------------------------------------------------------------------
// Rack::Extension::getStereoAudioInSocket
//------------------------------------------------------------------------
Rack::Extension::StereoAudioInSocket Rack::Extension::getStereoAudioInSocket(std::string const &iLeftSocketName,
                                                                               std::string const &iRightSocketName) const
{
  return { getAudioInSocket(iLeftSocketName), getAudioInSocket(iRightSocketName) };
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::wire
//------------------------------------------------------------------------
void Rack::ExtensionImpl::wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket)
{
  RE_MOCK_ASSERT(iInSocket.fExtensionId == fId || iOutSocket.fExtensionId == fId); // sanity check...
  auto newWire = Extension::AudioWire{ iOutSocket, iInSocket };

  // check for duplicate
  RE_MOCK_ASSERT(!stl::find_if(fAudioOutWires, [&newWire](auto &wire) { return Rack::Extension::AudioWire::overlap(wire, newWire); }), "Audio socket in use");

  if(iOutSocket.fExtensionId == fId)
    fAudioOutWires.emplace_back(newWire);

  if(iOutSocket.fExtensionId != fId)
  {
    fAudioInWires.emplace_back(newWire);
    fDependents = std::nullopt;
  }
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::unwire
//------------------------------------------------------------------------
std::optional<Rack::Extension::AudioInSocket> Rack::ExtensionImpl::unwire(Extension::AudioOutSocket const &iOutSocket)
{
  RE_MOCK_ASSERT(iOutSocket.fExtensionId == fId); // sanity check...

  auto iter =
    std::find_if(fAudioOutWires.begin(), fAudioOutWires.end(), [&iOutSocket](auto &wire) { return wire.fFromSocket == iOutSocket; });

  if(iter != fAudioOutWires.end())
  {
    auto wire = *iter;
    fAudioOutWires.erase(iter);
    return wire.fToSocket;
  }

  return std::nullopt;
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::unwire
//------------------------------------------------------------------------
std::optional<Rack::Extension::AudioOutSocket> Rack::ExtensionImpl::unwire(Extension::AudioInSocket const &iInSocket)
{
  RE_MOCK_ASSERT(iInSocket.fExtensionId == fId); // sanity check...

  auto iter =
    std::find_if(fAudioInWires.begin(), fAudioInWires.end(), [&iInSocket](auto &wire) { return wire.fToSocket == iInSocket; });

  if(iter != fAudioInWires.end())
  {
    auto wire = *iter;
    fAudioInWires.erase(iter);
    fDependents = std::nullopt;
    return wire.fFromSocket;
  }

  return std::nullopt;
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::getDependents
//------------------------------------------------------------------------
std::set<int> const &Rack::ExtensionImpl::getDependents() const
{
  if(!fDependents)
  {
    std::set<int> s{};
    for(auto &w: fAudioInWires)
      s.emplace(w.fFromSocket.fExtensionId);
    for(auto &w: fCVInWires)
      s.emplace(w.fFromSocket.fExtensionId);
    fDependents = s;
  }

  return *fDependents;
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
// Rack::ExtensionImpl::wire
//------------------------------------------------------------------------
void Rack::ExtensionImpl::wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket)
{
  RE_MOCK_ASSERT(iInSocket.fExtensionId == fId || iOutSocket.fExtensionId == fId); // sanity check...
  auto newWire = Extension::CVWire{ iOutSocket, iInSocket };

  // check for duplicate
  RE_MOCK_ASSERT(!stl::find_if(fCVOutWires, [&newWire](auto &wire) { return Rack::Extension::CVWire::overlap(wire, newWire); }), "CV socket in use");

  if(iOutSocket.fExtensionId == fId)
    fCVOutWires.emplace_back(newWire);

  if(iOutSocket.fExtensionId != fId)
  {
    fCVInWires.emplace_back(newWire);
    fDependents = std::nullopt;
  }
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::unwire
//------------------------------------------------------------------------
std::optional<Rack::Extension::CVInSocket> Rack::ExtensionImpl::unwire(Extension::CVOutSocket const &iOutSocket)
{
  RE_MOCK_ASSERT(iOutSocket.fExtensionId == fId); // sanity check...

  auto iter =
    std::find_if(fCVOutWires.begin(), fCVOutWires.end(), [&iOutSocket](auto &wire) { return wire.fFromSocket == iOutSocket; });

  if(iter != fCVOutWires.end())
  {
    auto wire = *iter;
    fCVOutWires.erase(iter);
    return wire.fToSocket;
  }

  return std::nullopt;
}

//------------------------------------------------------------------------
// Rack::ExtensionImpl::unwire
//------------------------------------------------------------------------
std::optional<Rack::Extension::CVOutSocket> Rack::ExtensionImpl::unwire(Extension::CVInSocket const &iInSocket)
{
  RE_MOCK_ASSERT(iInSocket.fExtensionId == fId); // sanity check...

  auto iter =
    std::find_if(fCVInWires.begin(), fCVInWires.end(), [&iInSocket](auto &wire) { return wire.fToSocket == iInSocket; });

  if(iter != fCVInWires.end())
  {
    auto wire = *iter;
    fCVInWires.erase(iter);
    fDependents = std::nullopt;
    return wire.fFromSocket;
  }

  return std::nullopt;
}

}