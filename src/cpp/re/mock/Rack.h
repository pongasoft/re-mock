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

#pragma once
#ifndef __PongasoftCommon_re_mock_rack_h__
#define __PongasoftCommon_re_mock_rack_h__

#include <map>
#include "Motherboard.h"
#include "LuaJBox.h"

namespace re::mock {

extern LuaJbox jbox;

class Rack
{
public:
  struct REConfId { int fId{}; };
  struct REInstId { int fId{}; };

  using REConf = std::function<void (MotherboardDef &, RealtimeController &, Realtime &)>;

  Rack();

  REConfId configureRE(REConf iREConf);
  REInstId instantiateRE(REConfId id);
  inline REInstId instantiateRE(REConf iREConf) { return instantiateRE(configureRE(std::move(iREConf))); }

  void useRE(REInstId id, std::function<void (Motherboard *)> iCallback);
  inline void useRE(REInstId id, std::function<void ()> iCallback) {
    useRE(id, [&iCallback](auto motherboard) { iCallback(); });
  }

  static Motherboard &currentMotherboard();

protected:
  std::map<decltype(REConfId::fId), REConf> fREConfigurations{};
  std::map<decltype(REConfId::fId), std::unique_ptr<Motherboard>> fREInstances{};
};

}

#endif //__PongasoftCommon_re_mock_rack_h__