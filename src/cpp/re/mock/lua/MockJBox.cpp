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

#include "MockJBox.h"
#include <re/mock/Errors.h>
#include <Jukebox.h>

namespace re::mock::lua {

char MockJBox::REGISTRY_KEY = 'K';

//------------------------------------------------------------------------
// MockJBox::MockJBox
//------------------------------------------------------------------------
MockJBox::MockJBox()
{
  lua_pushlightuserdata(L, static_cast<void *>(&REGISTRY_KEY));
  lua_pushlightuserdata(L, static_cast<void *>(this));
  lua_settable(L, LUA_REGISTRYINDEX);
}

//------------------------------------------------------------------------
// MockJBox::loadFromRegistry
//------------------------------------------------------------------------
MockJBox *MockJBox::loadFromRegistry(lua_State *L)
{
  lua_pushlightuserdata(L, static_cast<void *>(&REGISTRY_KEY));
  lua_gettable(L, LUA_REGISTRYINDEX);
  auto ud = lua_touserdata(L, -1);
  lua_pop(L, 1);
  auto res = reinterpret_cast<MockJBox *>(ud);
  RE_MOCK_ASSERT(res->L.getLuaState() == L, "sanity check");
  return res;
}

//------------------------------------------------------------------------
// MockJBox::toJBoxValue
//------------------------------------------------------------------------
TJBox_Value MockJBox::toJBoxValue(int idx)
{
  int t = lua_type(L, idx);
  switch(t)
  {
    case LUA_TBOOLEAN:
      return JBox_MakeBoolean(lua_toboolean(L, idx));

    case LUA_TNUMBER:
      return JBox_MakeNumber(lua_tonumber(L, idx));

    default:  /* other values */
      return JBox_MakeNil();
  }
}

}