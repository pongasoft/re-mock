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
#include <re/mock/Motherboard.h>
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
// MockJBox::loadFile
//------------------------------------------------------------------------
int MockJBox::loadFile(std::string const &iLuaFilename)
{
  return L.runLuaFile(iLuaFilename);
}

//------------------------------------------------------------------------
// MockJBox::loadString
//------------------------------------------------------------------------
int MockJBox::loadString(std::string const &iLuaCode)
{
  return L.runLuaCode(iLuaCode);
}

//------------------------------------------------------------------------
// MockJBox::iterateLuaTable
//------------------------------------------------------------------------
void MockJBox::iterateLuaTable(std::function<void(lua_table_key_t)> const &iKeyHandler,
                               bool iPopValue,
                               bool iPopTable)
{
  int mapStackIndex = lua_gettop(L);
  // check for NIL
  if(lua_type(L, mapStackIndex) != LUA_TNIL)
  {
    luaL_checktype(L, mapStackIndex, LUA_TTABLE);

    lua_pushnil(L);  /* first key */
    while(lua_next(L, mapStackIndex) != 0)
    {
      auto keyType = lua_type(L, -2);
      switch(keyType)
      {
        case LUA_TSTRING:
          iKeyHandler(lua_tostring(L, -2));
          break;

        case LUA_TNUMBER:
          iKeyHandler(static_cast<int>(lua_tonumber(L, -2)));
          break;

        default:
          RE_MOCK_ASSERT(false, "table keys are either strings or integers");
          break;
      }
      if(iPopValue)
        lua_pop(L, 1);
    }
  }
  if(iPopTable)
    lua_pop(L, 1);
}

//------------------------------------------------------------------------
// MockJBox::iterateLuaArray
//------------------------------------------------------------------------
void MockJBox::iterateLuaArray(std::function<void(int)> const &iIndexHandler, bool iPopValue, bool iPopTable)
{
  int mapStackIndex = lua_gettop(L);
  // check for NIL
  if(lua_type(L, mapStackIndex) != LUA_TNIL)
  {
    luaL_checktype(L, mapStackIndex, LUA_TTABLE);

    lua_pushnil(L);  /* first key */
    while(lua_next(L, mapStackIndex) != 0)
    {
      auto keyType = lua_type(L, -2);
      switch(keyType)
      {
        case LUA_TSTRING:
          // skip
          break;

        case LUA_TNUMBER:
          iIndexHandler(static_cast<int>(lua_tonumber(L, -2)));
          break;

        default:
          RE_MOCK_ASSERT(false, "table keys are either strings or integers");
          break;
      }
      if(iPopValue)
        lua_pop(L, 1);
    }
  }
  if(iPopTable)
    lua_pop(L, 1);
}

}