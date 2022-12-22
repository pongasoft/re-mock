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
#include <optional>
#include "../fs.h"
#include "../fmt.h"
#include "../Errors.h"

namespace re::mock::lua {

struct LuaStackInfo
{
  std::string getSourceFile() const;
  std::optional<std::string> computeSnippet(int lineCount = 0) const;

  std::string fSource{};
  int fLineNumber{-1};

  static std::string computeSnippetFromStream(std::istream &iStream, int lineNumber, int lineCount = 0);
  static std::string computeSnippetFromString(std::string const &iSource, int lineNumber, int lineCount = 0);
  static std::string computeSnippetFromFile(fs::path const &iFilepath, int lineNumber, int lineCount = 0);
};

struct LuaException : public re::mock::Exception {
  explicit LuaException(LuaStackInfo const &iStackInfo, std::string const &s) :
    re::mock::Exception(s.c_str()),
    fStackInfo(iStackInfo)
  {}

  explicit LuaException(LuaStackInfo const &iStackInfo, char const *s) :
    re::mock::Exception(s),
    fStackInfo(iStackInfo)
  {}

  [[ noreturn ]] static void throwException(LuaStackInfo const &iStackInfo, char const *iMessage);

  template<typename ... Args>
  [[ noreturn ]] static void throwException(LuaStackInfo const &iStackInfo, const std::string& format, Args ... args)
  {
    throwException(iStackInfo, fmt::printf(format, args...).c_str());
  }

  LuaStackInfo fStackInfo{};
};


class LuaState
{
public:
  LuaState();
  ~LuaState();

  int runLuaFile(fs::path const &iFilename);
  int runLuaCode(std::string const &iSource);

  lua_State *getLuaState() { return L; }

  /**
   * Make this class behave like a `lua_State *` */
  operator lua_State *() { return L; }

  void dumpStack(char const *iMessage = nullptr, std::ostream &oStream = std::cout) { dumpStack(L, iMessage, oStream); }
  std::string getStackString(char const *iMessage = nullptr) { return getStackString(L, iMessage); }

  bool getGlobalAsBoolean(char const *iKey);
  lua_Integer getGlobalAsInteger(char const *iKey);
  std::string getGlobalAsString(char const *iKey);

  lua_Number getTableValueAsNumber(char const *iKey, int idx = -1);
  std::optional<lua_Number> getTableValueAsOptionalNumber(char const *iKey, int idx = -1);
  lua_Integer getTableValueAsInteger(char const *iKey, int idx = -1);
  std::optional<lua_Integer> getTableValueAsOptionalLuaInteger(char const *iKey, int idx = -1);
  template<typename Int = lua_Integer>
  std::optional<Int> getTableValueAsOptionalInteger(char const *iKey, int idx = -1)
  {
    auto v = getTableValueAsOptionalLuaInteger(iKey, idx);
    if(v)
      return static_cast<Int>(*v);
    else
      return std::nullopt;
  }
  bool getTableValueAsBoolean(char const *iKey, int idx = -1);
  std::optional<bool> getTableValueAsOptionalBoolean(char const *iKey, int idx = -1);
  std::string getTableValueAsString(char const *iKey, int idx = -1);
  std::optional<std::string> getTableValueAsOptionalString(char const *iKey, int idx = -1);
  lua_Unsigned getTableSize(int idx = -1);

  lua_Integer getArrayValueAsInteger(int iKey, int idx = -1);

  void setTableValue(char const *iKey, lua_Number iValue);
  void setTableValue(char const *iKey, std::string const &iValue);

  template<typename ... Args>
  [[ noreturn ]] void parseError(const std::string& format, Args ... args);

  static std::string getStackString(lua_State *L, char const *iMessage = nullptr);
  static void dumpStack(lua_State *L, char const *iMessage = nullptr, std::ostream &oStream = std::cout);

private:
  lua_State *L{}; // using common naming in all lua apis...
};

//------------------------------------------------------------------------
// LuaState::parseError
//------------------------------------------------------------------------
template<typename... Args>
void LuaState::parseError(const std::string& format, Args ... args)
{
  lua_pushstring(L, fmt::printf(format, args...).c_str());
  lua_error(L); // This function does a long jump, and therefore never returns
  throw 0; // lua_error is not marked [[ noreturn ]]
}

#define RE_MOCK_LUA_PARSE_ASSERT(test, format, ...) (test) == true ? (void)0 : L.parseError((format), ##__VA_ARGS__)
#define RE_MOCK_LUA_RUNTIME_ASSERT(test, stack, format, ...) (test) == true ? (void)0 : re::mock::lua::LuaException::throwException((stack), (format), ##__VA_ARGS__)

}

#endif //__Pongasoft_re_mock_lua_lua_state_h__