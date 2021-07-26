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

extern "C" {
static int lua_panic(lua_State *L)
{
  re::mock::lua::LuaState::dumpStack(L, "panic");
  throw std::runtime_error("panic");
}
}

namespace re::mock::lua {

//------------------------------------------------------------------------
// LuaState::LuaState
//------------------------------------------------------------------------
LuaState::LuaState() : fLuaState{luaL_newstate()}
{
  lua_atpanic(fLuaState, lua_panic);
  luaL_openlibs(fLuaState);
}

//------------------------------------------------------------------------
// LuaState::~LuaState
//------------------------------------------------------------------------
LuaState::~LuaState()
{
  if(fLuaState)
    lua_close(fLuaState);
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

}