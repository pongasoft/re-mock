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
#include <fstream>
#include <sstream>
#include <re/mock/fmt.h>
#include <re/mock/Errors.h>

namespace re::mock::lua {

namespace impl {

//------------------------------------------------------------------------
// lastlevel (copied from lauxlib.c because not exported :( )
//------------------------------------------------------------------------
int lastlevel(lua_State *L)
{
  lua_Debug ar;
  int li = 1, le = 1;
  /* find an upper bound */
  while(lua_getstack(L, le, &ar))
  {
    li = le;
    le *= 2;
  }
  /* do a binary search */
  while(li < le)
  {
    int m = (li + le) / 2;
    if(lua_getstack(L, m, &ar)) li = m + 1;
    else le = m;
  }
  return le - 1;
}

//------------------------------------------------------------------------
// error_handler
// Returns array with { errorMsg, lineNumer }
//------------------------------------------------------------------------
int error_handler(lua_State *L)
{
  // make sure the argument is a string
  if(!lua_isstring(L, 1))
    return 1;  // keep it intact

  auto lastLevel = lastlevel(L);
  int lineNumber = -1;
  lua_Debug ar;
  if(lua_getstack(L, lastLevel, &ar))
  {
    lua_getinfo(L, "l", &ar);
    lineNumber = ar.currentline;
  }

  lua_getglobal(L, "debug");
  if(!lua_istable(L, -1))
  {
    lua_pop(L, 1);
    return 1;
  }

  lua_getfield(L, -1, "traceback");
  if(!lua_isfunction(L, -1))
  {
    lua_pop(L, 2);
    return 1;
  }

  lua_pushvalue(L, 1);  // pass error message
  lua_pushinteger(L, 2);  // skip this function and traceback

  lua_call(L, 2, 1);  // call debug.traceback

  auto msg = lua_gettop(L);
  lua_newtable(L);
  auto t = lua_gettop(L);
  lua_pushvalue(L, msg); // copy msg on top of the stack
  lua_rawseti(L, t, 1); // t[1] = msg;
  lua_pushinteger(L, lineNumber);
  lua_rawseti(L, t, 2); // t[2] = lineNumber;
  return 1; // error message + line number
}

//------------------------------------------------------------------------
// lua_panic
//------------------------------------------------------------------------
static int lua_panic(lua_State *L)
{
  if(lua_gettop(L) > 0)
    throw re::mock::Exception(lua_tostring(L, -1));
  else
    throw re::mock::Exception("panic");
}

}

//------------------------------------------------------------------------
// LuaState::LuaState
//------------------------------------------------------------------------
LuaState::LuaState() : L{luaL_newstate()}
{
  lua_atpanic(L, impl::lua_panic);
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
// LuaState::getGlobalAsBoolean
//------------------------------------------------------------------------
bool LuaState::getGlobalAsBoolean(char const *iKey)
{
  auto res = false;
  if(lua_getglobal(L, iKey) != LUA_TNIL)
    res = lua_toboolean(L, -1);
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// LuaState::getGlobalAsInteger
//------------------------------------------------------------------------
lua_Integer LuaState::getGlobalAsInteger(char const *iKey)
{
  lua_Integer res = 0;
  if(lua_getglobal(L, iKey) != LUA_TNIL)
    res = lua_tointeger(L, -1);
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// LuaState::getGlobalAsString
//------------------------------------------------------------------------
std::string LuaState::getGlobalAsString(char const *iKey)
{
  std::string res = "";

  if(lua_getglobal(L, iKey) != LUA_TNIL)
  {
    auto s = lua_tostring(L, -1);
    if(s != nullptr)
      res = std::string(s);
  }
  lua_pop(L, 1);
  return res;
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
// LuaState::getTableValueAsOptionalNumber
//------------------------------------------------------------------------
std::optional<lua_Number> LuaState::getTableValueAsOptionalNumber(char const *iKey, int idx)
{
  std::optional<lua_Number> res{};
  luaL_checktype(L, idx, LUA_TTABLE);
  if(lua_getfield(L, idx, iKey) != LUA_TNIL)
    res = lua_tonumber(L, -1);
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
// LuaState::getArrayValueAsInteger
//------------------------------------------------------------------------
lua_Integer LuaState::getArrayValueAsInteger(int iKey, int idx)
{
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_geti(L, idx, iKey);
  auto res = lua_tointeger(L, -1);
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// LuaState::getTableValueAsOptionalInteger
//------------------------------------------------------------------------
std::optional<lua_Integer> LuaState::getTableValueAsOptionalInteger(char const *iKey, int idx)
{
  std::optional<lua_Integer> res{};
  luaL_checktype(L, idx, LUA_TTABLE);
  if(lua_getfield(L, idx, iKey) != LUA_TNIL)
    res = lua_tointeger(L, -1);
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
// LuaState::getTableValueAsOptionalBoolean
//------------------------------------------------------------------------
std::optional<bool> LuaState::getTableValueAsOptionalBoolean(char const *iKey, int idx)
{
  std::optional<bool> res{};
  luaL_checktype(L, idx, LUA_TTABLE);
  if(lua_getfield(L, idx, iKey) != LUA_TNIL)
    res = lua_toboolean(L, -1);
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// LuaState::getTableValueAsString
//------------------------------------------------------------------------
std::string LuaState::getTableValueAsString(char const *iKey, int idx)
{
  auto res = getTableValueAsOptionalString(iKey, idx);
  if(res)
    return *res;
  else
    return "";
}

//------------------------------------------------------------------------
// LuaState::getTableValueAsOptionalString
//------------------------------------------------------------------------
std::optional<std::string> LuaState::getTableValueAsOptionalString(char const *iKey, int idx)
{
  std::optional<std::string> res{};
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_getfield(L, idx, iKey);
  auto s = lua_tostring(L, -1);
  if(s != nullptr)
    res = std::string(s);
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
// LuaState::setTableValue
//------------------------------------------------------------------------
void LuaState::setTableValue(char const *iKey, lua_Number iValue)
{
  luaL_checktype(L, -1, LUA_TTABLE);
  lua_pushnumber(L, iValue);
  lua_setfield(L, -2, iKey);
}

//------------------------------------------------------------------------
// LuaState::setTableValue
//------------------------------------------------------------------------
void LuaState::setTableValue(char const *iKey, std::string const &iValue)
{
  luaL_checktype(L, -1, LUA_TTABLE);
  lua_pushstring(L, iValue.c_str());
  lua_setfield(L, -2, iKey);
}

//------------------------------------------------------------------------
// LuaState::runLuaFile
//------------------------------------------------------------------------
int LuaState::runLuaFile(fs::path const &iFilename)
{
  // pushing error handler on the stack
  lua_pushcfunction(L, impl::error_handler);
  auto msgh = lua_gettop(L);

  // 1. load the file (fails if file does not exist)
  auto res = luaL_loadfile(L, iFilename.string().c_str());

  if(res != LUA_OK)
  {
    std::string errorMsg{lua_tostring(L, -1)};
    lua_pop(L, 1);
    throw re::mock::Exception(errorMsg);
  }

  // 2. execute the file (with error handling)
  res = lua_pcall(L, 0, LUA_MULTRET, msgh);

  if(res != LUA_OK)
  {
    lua_rawgeti(L, -1, 1);
    std::string errorMsg{lua_tostring(L, -1)};
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    auto lineNumber = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    LuaException::throwException({fmt::printf("@%s", iFilename), lineNumber}, errorMsg);
  }

  // 3. remove the error_handler from the stack
  lua_pop(L, 1);

  return res;
}

//------------------------------------------------------------------------
// LuaState::runLuaCode
//------------------------------------------------------------------------
int LuaState::runLuaCode(std::string const &iSource)
{
  // pushing error handler on the stack
  lua_pushcfunction(L, impl::error_handler);
  auto msgh = lua_gettop(L);

  // 1. load the string
  auto res = luaL_loadstring(L, iSource.c_str());

  if(res != LUA_OK)
  {
    std::string errorMsg{lua_tostring(L, -1)};
    lua_pop(L, 1);
    throw re::mock::Exception(errorMsg);
  }

  // 2. execute the code (with error handling)
  res = lua_pcall(L, 0, LUA_MULTRET, msgh);

  if(res != LUA_OK)
  {
    lua_rawgeti(L, -1, 1);
    std::string errorMsg{lua_tostring(L, -1)};
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    auto lineNumber = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    LuaException::throwException({iSource, lineNumber}, errorMsg);
  }

  // 3. remove the error_handler from the stack
  lua_pop(L, 1);

  return res;
}

//------------------------------------------------------------------------
// LuaState::getSourceFile
//------------------------------------------------------------------------
std::string LuaStackInfo::getSourceFile() const
{
  if(!fSource.empty())
  {
    if(fSource[0] == '@')
      return fSource.substr(1);
  }
  return "...";
}

//------------------------------------------------------------------------
// LuaState::computeSnippet
//------------------------------------------------------------------------
std::optional<std::string> LuaStackInfo::computeSnippet(int lineCount) const
{
  if(fSource.empty())
    return std::nullopt;

  try
  {
    if(fSource[0] == '@')
    {
      return computeSnippetFromFile(fSource.substr(1), fLineNumber, lineCount);
    }
    else
    {
      return computeSnippetFromString(fSource, fLineNumber, lineCount);
    }
  }
  catch(...)
  {
    RE_MOCK_LOG_WARNING("Error while computing snippet for %s:%d", fSource, fLineNumber);
  }

  return std::nullopt;
}

//------------------------------------------------------------------------
// LuaState::computeSnippetFromStream
//------------------------------------------------------------------------
std::string LuaStackInfo::computeSnippetFromStream(std::istream &iStream, int lineNumber, int lineCount)
{
  RE_MOCK_ASSERT(lineNumber >= 1);
  RE_MOCK_ASSERT(lineCount >= 0);

  std::stringstream res{};
  int startLine = std::max(lineNumber - lineCount, 1);
  int endLine = lineNumber + lineCount;
  int currentLine = 1;
  std::string line;
  while(std::getline(iStream, line, '\n'))
  {
    if(currentLine >= startLine)
      res << (currentLine == lineNumber ? "->" : "  ") << "[" << currentLine << "]\t" << line << "\n";
    currentLine++;
    if(currentLine > endLine)
      break;
  }
  return res.str();
}

//------------------------------------------------------------------------
// LuaState::computeSnippetFromString
//------------------------------------------------------------------------
std::string LuaStackInfo::computeSnippetFromString(std::string const &iSource, int lineNumber, int lineCount)
{
  std::istringstream stream(iSource);
  return computeSnippetFromStream(stream, lineNumber, lineCount);
}

//------------------------------------------------------------------------
// LuaState::computeSnippetFromFile
//------------------------------------------------------------------------
std::string LuaStackInfo::computeSnippetFromFile(fs::path const &iFilepath, int lineNumber, int lineCount)
{
  std::fstream f{iFilepath};
  return computeSnippetFromStream(f, lineNumber, lineCount);
}

//------------------------------------------------------------------------
// LuaState::extractSnippet
//------------------------------------------------------------------------
void LuaException::throwException(LuaStackInfo const &iStackInfo, char const *iMessage)
{
  auto errorMessage = fmt::printf("%s:%d | %s", iStackInfo.getSourceFile(), iStackInfo.fLineNumber, iMessage);
  auto snippet = iStackInfo.computeSnippet(5);
  if(snippet)
    errorMessage = fmt::printf("%s\nsnippet:\n%s", errorMessage, *snippet);
  throw LuaException(iStackInfo, errorMessage);
}
}