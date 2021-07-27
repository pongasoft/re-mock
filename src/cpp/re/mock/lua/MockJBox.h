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
#ifndef __Pongasoft_re_mock_lua_mock_jbox_h__
#define __Pongasoft_re_mock_lua_mock_jbox_h__

#include "LuaState.h"
#include <JukeboxTypes.h>

namespace re::mock::lua {

class MockJBox
{
public:
  MockJBox();

  virtual ~MockJBox() = default;

  std::string getStackString(char const *iMessage = nullptr) { return L.getStackString(iMessage); }

  TJBox_Value toJBoxValue(int idx = -1);
  void pushJBoxValue(TJBox_Value const &iJBoxValue);

protected:
  static MockJBox *loadFromRegistry(lua_State *L);

protected:
  LuaState L{}; // using common naming in all lua apis...

private:
  static char REGISTRY_KEY;
};

}

#endif //__Pongasoft_re_mock_lua_mock_jbox_h__