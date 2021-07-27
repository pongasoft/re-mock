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

#include "LuaState.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <re/mock/fmt.h>
#include <re/mock/Errors.h>

extern "C" {
static int lua_panic(lua_State *L)
{
  re::mock::lua::LuaState::dumpStack(L, "panic");
  if(lua_gettop(L) > 0)
    throw std::runtime_error(lua_tostring(L, -1));
  else
    throw std::runtime_error("panic");
}
}

namespace re::mock::lua {

//------------------------------------------------------------------------
// LuaState::LuaState
//------------------------------------------------------------------------
LuaState::LuaState() : L{luaL_newstate()}
{
  lua_atpanic(L, lua_panic);
  luaL_openlibs(L);
}

//------------------------------------------------------------------------
// LuaState::~LuaState
//------------------------------------------------------------------------
LuaState::~LuaState()
{
  if(L)
    lua_close(L);
}

//------------------------------------------------------------------------
// LuaState::dumpStack
//------------------------------------------------------------------------
void LuaState::dumpStack(lua_State *L, char const *iMessage, std::ostream &oStream)
{
  oStream << getStackString(L, iMessage) << std::endl;
}

//------------------------------------------------------------------------
// LuaState::getStackString
//------------------------------------------------------------------------
std::string LuaState::getStackString(lua_State *L, char const *iMessage)
{
  std::ostringstream stream{};
  int top = lua_gettop(L);

  if(iMessage)
    stream << fmt::printf("%s: ", iMessage);

  if(top == 0)
    stream << "<empty>";
  else
  {
    for(int i = 1; i <= top; i++)
    {  /* repeat for each level */
      if(i > 1)
        stream << " | ";
      int t = lua_type(L, i);
      switch(t)
      {
        case LUA_TSTRING:  /* strings */
          stream << fmt::printf("[%d] '%s' ", i, lua_tostring(L, i));
          break;

        case LUA_TBOOLEAN:  /* booleans */
          stream << fmt::printf("[%d] %s ", i, lua_toboolean(L, i) ? "true" : "false");
          break;

        case LUA_TNUMBER:  /* numbers */
          stream << fmt::printf("[%d] %g ", i, lua_tonumber(L, i));
          break;

        default:  /* other values */
          stream << fmt::printf("[%d] <%s> ", i, lua_typename(L, t));
          break;
      }
    }
  }

  return stream.str();
}

//------------------------------------------------------------------------
// LuaState::getTableValueAsNumber
//------------------------------------------------------------------------
lua_Number LuaState::getTableValueAsNumber(char const *iKey, int idx)
{
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_getfield(L, idx, iKey);
  auto res = lua_tonumber(L, -1);
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// LuaState::getTableValueAsInteger
//------------------------------------------------------------------------
lua_Integer LuaState::getTableValueAsInteger(char const *iKey, int idx)
{
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_getfield(L, idx, iKey);
  auto res = lua_tointeger(L, -1);
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// LuaState::getTableValueAsBoolean
//------------------------------------------------------------------------
bool LuaState::getTableValueAsBoolean(char const *iKey, int idx)
{
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_getfield(L, idx, iKey);
  auto res = lua_toboolean(L, -1);
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// LuaState::getTableValueAsString
//------------------------------------------------------------------------
std::string LuaState::getTableValueAsString(char const *iKey, int idx)
{
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_getfield(L, idx, iKey);
  auto res = std::string(lua_tostring(L, -1));
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// LuaState::getTableSize
//------------------------------------------------------------------------
lua_Unsigned LuaState::getTableSize(int idx)
{
  luaL_checktype(L, idx, LUA_TTABLE);
  return lua_rawlen(L, idx);
}


//------------------------------------------------------------------------
// LuaState::runLuaFile
//------------------------------------------------------------------------
int LuaState::runLuaFile(std::string const &iFilename)
{
  auto const res = luaL_dofile(L, iFilename.c_str());

  if(res != LUA_OK)
  {
    std::string errorMsg{lua_tostring(L, -1)};
    lua_pop(L, 1);
    RE_MOCK_ASSERT(res == LUA_OK, "%s", errorMsg.c_str());
  }

  return res;
}

//------------------------------------------------------------------------
// LuaState::runLuaCode
//------------------------------------------------------------------------
int LuaState::runLuaCode(std::string const &iSource)
{
  auto const res = luaL_dostring(L, iSource.c_str());

  if(res != LUA_OK)
  {
    std::string errorMsg{lua_tostring(L, -1)};
    lua_pop(L, 1);
    RE_MOCK_ASSERT(res == LUA_OK, "%s", errorMsg.c_str());
  }

  return res;
}


}