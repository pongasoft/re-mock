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
#include <atomic>
#include "Rack.h"

namespace re::mock {

static thread_local Motherboard *sThreadLocalInstance{};

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
Rack::Rack()
{
  // set up loguru
}

static std::atomic<int> sREConfigurationsCounter{1};

//------------------------------------------------------------------------
// Rack::configureRE
//------------------------------------------------------------------------
Rack::REConfId Rack::configureRE(Rack::REConf iREConf)
{
  REConfId id{ .fId = sREConfigurationsCounter++ };
  fREConfigurations[id.fId] = std::move(iREConf);
  return id;
}

static std::atomic<int> sREInstancesCounter{1};

//------------------------------------------------------------------------
// Rack::instantiateRE
//------------------------------------------------------------------------
Rack::REInstId Rack::instantiateRE(Rack::REConfId id)
{
  auto config = fREConfigurations.find(id.fId);
  CHECK_F(config != fREConfigurations.end(), "Unknown RE with configuration [%d] (did you call configureRE?)", id.fId);
  REInstId instanceId{ .fId = sREInstancesCounter++ };
  fREInstances[instanceId.fId] = Motherboard::init(config->second);
  return instanceId;
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
  auto motherboard = fREInstances.find(id.fId);
  CHECK_F(motherboard != fREInstances.end(), "Unknown RE with instance [%d] (did you call instantiateRE?)", id.fId);
  InternalThreadLocalRAII raii{motherboard->second.get()};
  iCallback(motherboard->second.get());
}


}