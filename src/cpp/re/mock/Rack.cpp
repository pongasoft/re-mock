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


}