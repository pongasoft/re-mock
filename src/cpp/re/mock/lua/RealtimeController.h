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

#ifndef __Pongasoft_re_mock_lua_realtime_controller_h__
#define __Pongasoft_re_mock_lua_realtime_controller_h__

#include "MockJBox.h"
#include <re/mock/ObjectManager.hpp>
#include <map>
#include <set>

namespace re::mock {
class Motherboard;
class JboxValue;
}

namespace re::mock::lua {

class RealtimeController: public MockJBox
{
public:
  RealtimeController();

  int luaTrace();

  int luaLoadProperty();
  int luaStoreProperty();

  int luaMakeNil();

  int luaMakeNativeObject(bool iReadOnly);
  int luaIsNativeObject();

  int luaIsBlob();
  int luaLoadBlobAsync();
  int luaGetBlobInfo();

  std::map<std::string, std::string> getBindings();
  std::set<std::string> getRTInputSetupNotify();

  void invokeBinding(Motherboard *iMotherboard,
                     std::string const &iBindingName,
                     std::string const &iSourcePropertyPath,
                     std::shared_ptr<const JboxValue> const &iNewValue);

  static RealtimeController *loadFromRegistry(lua_State *L);
  static std::unique_ptr<RealtimeController> fromFile(std::string const &iLuaFilename);
  static std::unique_ptr<RealtimeController> fromString(std::string const &iLuaCode);

protected:
  Motherboard *getCurrentMotherboard() const;
  std::shared_ptr<const JboxValue> toJBoxValue(int idx = -1);
  void pushJBoxValue(std::shared_ptr<const JboxValue> iJBoxValue);

  void putBindingOnTopOfStack(std::string const &iBindingName);

private:
  Motherboard *fMotherboard{};
  ObjectManager<std::shared_ptr<const JboxValue>> fJboxValues{};
};

}

#endif //__Pongasoft_re_mock_lua_realtime_controller_h__