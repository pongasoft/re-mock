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
#include <string>
#include <functional>
#include <variant>

namespace re::mock::lua {

class MockJBox
{
public:
  using lua_table_key_t = std::variant<std::string, int>;
public:
  MockJBox();

  virtual ~MockJBox() = default;

  std::string getStackString(char const *iMessage = nullptr) { return L.getStackString(iMessage); }

  int loadFile(std::string const &iLuaFilename);
  int loadString(std::string const &iLuaCode);

  /**
   * Iterate over every entry in the map on top of the stack. For each entry, the entry handler is called
   * with the key (which may be a `std::string` or `int`) and it should handle the value which is on top of the
   * stack and pop it unless `iPopValue` is `true`.
   *
   * @note it is safe to call this method with NIL on top of the stack
   * @note this method pops the table itself (or NIL if NIL was on top of the stack) from the stack if iPopTable is true */
  void iterateLuaTable(std::function<void(lua_table_key_t)> const &iKeyHandler,
                       bool iPopValue = false,
                       bool iPopTable = true);

  /**
   * Iterate over every entry in the array on top of the stack. For each index, the index handler is called
   * with the index and it should handle the value which is on top of the stack and pop it unless `iPopValue` is `true`.
   *
   * @note it is safe to call this method with NIL on top of the stack
   * @note this method pops the table itself (or NIL if NIL was on top of the stack) from the stack if iPopTable is true */
  void iterateLuaArray(std::function<void(int)> const &iIndexHandler, bool iPopValue = false, bool iPopTable = true);

protected:
  static MockJBox *loadFromRegistry(lua_State *L);

protected:
  LuaState L{}; // using common naming in all lua apis...

private:
  static char REGISTRY_KEY;
};

}

#endif //__Pongasoft_re_mock_lua_mock_jbox_h__