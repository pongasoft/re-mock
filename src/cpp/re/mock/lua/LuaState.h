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
#ifndef __Pongasoft_re_mock_lua_lua_state_h__
#define __Pongasoft_re_mock_lua_lua_state_h__

// lua must be included as extern "C" to work
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <string>
#include <iostream>

namespace re::mock::lua {

class LuaState
{
public:
  LuaState();
  ~LuaState();

  int runLuaFile(std::string const &iFilename);
  int runLuaCode(std::string const &iSource);

  lua_State *getLuaState() { return L; }

  /**
   * Make this class behave like a `lua_State *` */
  operator lua_State *() { return L; }

  void dumpStack(char const *iMessage = nullptr, std::ostream &oStream = std::cout) { dumpStack(L, iMessage, oStream); }
  std::string getStackString(char const *iMessage = nullptr) { return getStackString(L, iMessage); }

  lua_Number getTableValueAsNumber(char const *iKey, int idx = -1);
  lua_Integer getTableValueAsInteger(char const *iKey, int idx = -1);
  bool getTableValueAsBoolean(char const *iKey, int idx = -1);
  std::string getTableValueAsString(char const *iKey, int idx = -1);
  lua_Unsigned getTableSize(int idx = -1);

  static std::string getStackString(lua_State *L, char const *iMessage = nullptr);
  static void dumpStack(lua_State *L, char const *iMessage = nullptr, std::ostream &oStream = std::cout);

private:
  lua_State *L{}; // using common naming in all lua apis...
};

}

#endif //__Pongasoft_re_mock_lua_lua_state_h__