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
// Rack::configureRE
//------------------------------------------------------------------------
Rack::REConfId Rack::configureRE(Rack::REConf iREConf)
{
  return { .fId = fREConfigurations.add(std::move(iREConf)) };
}

//------------------------------------------------------------------------
// Rack::instantiateRE
//------------------------------------------------------------------------
Rack::REInstId Rack::instantiateRE(Rack::REConfId id)
{
  auto config = fREConfigurations.get(id.fId);
  return { .fId = fREInstances.add(Motherboard::init(fSampleRate, config)) };
}

struct InternalThreadLocalRAII
{
  explicit InternalThreadLocalRAII(Motherboard *iMotherboard) { sThreadLocalInstance = iMotherboard; }
  InternalThreadLocalRAII(InternalThreadLocalRAII &&) = delete;
  InternalThreadLocalRAII(InternalThreadLocalRAII const &) = delete;
  ~InternalThreadLocalRAII() { sThreadLocalInstance = nullptr; }
};

//------------------------------------------------------------------------
// Rack::useRE
//------------------------------------------------------------------------
void Rack::useRE(Rack::REInstId id, std::function<void(Motherboard *)> iCallback)
{
  auto &motherboard = fREInstances.get(id.fId);
  InternalThreadLocalRAII raii{motherboard.get()};
  iCallback(motherboard.get());
}


}