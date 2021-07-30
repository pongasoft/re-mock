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

#include <re/mock/Errors.h>
#include <re/mock/Motherboard.h>
#include "RealtimeController.h"

//------------------------------------------------------------------------
// Defining the C-API to invoke from lua as jbox.xxx
//------------------------------------------------------------------------
extern "C" {

using namespace re::mock::lua;

static int lua_load_property(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaLoadProperty();
}

static int lua_store_property(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaStoreProperty();
}

static int lua_make_native_object_rw(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaMakeNativeObject(false);
}

static int lua_make_native_object_ro(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaMakeNativeObject(true);
}

}

namespace re::mock::lua {

//------------------------------------------------------------------------
// RealtimeController::RealtimeController
//------------------------------------------------------------------------
RealtimeController::RealtimeController()
{
  static const struct luaL_Reg jboxLib[] = {
    {"load_property",         lua_load_property},
    {"store_property",        lua_store_property},
    {"make_native_object_rw", lua_make_native_object_rw},
    {"make_native_object_ro", lua_make_native_object_ro},
    {nullptr,                 nullptr}
  };

  luaL_newlib(L, jboxLib);
  lua_setglobal(L, "jbox"); // will be available in realtime_controller.lua as jbox
}

//------------------------------------------------------------------------
// RealtimeController::loadFromRegistry
//------------------------------------------------------------------------
RealtimeController *RealtimeController::loadFromRegistry(lua_State *L)
{
  auto res = dynamic_cast<RealtimeController *>(MockJBox::loadFromRegistry(L));
  RE_MOCK_ASSERT(res != nullptr);
  return res;
}

//------------------------------------------------------------------------
// RealtimeController::fromFile
//------------------------------------------------------------------------
std::unique_ptr<RealtimeController> RealtimeController::fromFile(std::string const &iLuaFilename)
{
  auto res = std::make_unique<RealtimeController>();
  res->loadFile(iLuaFilename);
  return res;
}

//------------------------------------------------------------------------
// RealtimeController::fromString
//------------------------------------------------------------------------
std::unique_ptr<RealtimeController> RealtimeController::fromString(std::string const &iLuaCode)
{
  auto res = std::make_unique<RealtimeController>();
  res->loadString(iLuaCode);
  return res;
}

//------------------------------------------------------------------------
// RealtimeController::luaLoadProperty
//------------------------------------------------------------------------
int RealtimeController::luaLoadProperty()
{
  luaL_checktype(L, 1, LUA_TSTRING);
  // TODO technically only rtc properties are allowed here...
  auto value = getCurrentMotherboard()->getValue(lua_tostring(L, 1));
  pushJBoxValue(value);
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaStoreProperty
//------------------------------------------------------------------------
int RealtimeController::luaStoreProperty()
{
  luaL_checktype(L, 1, LUA_TSTRING);
  // TODO technically only rtc properties are allowed here...
  getCurrentMotherboard()->setValue(lua_tostring(L, 1), toJBoxValue(2));
  return 0;
}

//------------------------------------------------------------------------
// RealtimeController::luaMakeNativeObject
//------------------------------------------------------------------------
int RealtimeController::luaMakeNativeObject(bool iReadOnly)
{
  int numArguments = lua_gettop(L);

  // Operation
  luaL_checktype(L, 1, LUA_TSTRING);
  auto operation = lua_tostring(L, 1);

  std::vector<TJBox_Value> params{};

  // params
  if(numArguments > 1)
  {
    auto paramsTableIdx = 2;

    auto numParams = L.getTableSize(paramsTableIdx);
    for(int i = 1; i <= numParams; i++)
    {
      lua_geti(L, paramsTableIdx, i);
      params.emplace_back(toJBoxValue());
      lua_pop(L, 1);
    }
  }

  pushJBoxValue(iReadOnly ?
                getCurrentMotherboard()->makeNativeObjectRO(operation, params) :
                getCurrentMotherboard()->makeNativeObjectRW(operation, params));

  return 1;

}

//------------------------------------------------------------------------
// RealtimeController::getCurrentMotherboard
//------------------------------------------------------------------------
Motherboard *RealtimeController::getCurrentMotherboard() const
{
  RE_MOCK_ASSERT(fMotherboard != nullptr, "Must be called from within a binding only!");
  return fMotherboard;
}

//------------------------------------------------------------------------
// RealtimeController::invokeBinding
//------------------------------------------------------------------------
void RealtimeController::invokeBinding(Motherboard *iMotherboard,
                                       std::string const &iBindingName,
                                       std::string const &iSourcePropertyPath,
                                       TJBox_Value const &iNewValue)
{
  fMotherboard = iMotherboard;
  putBindingOnTopOfStack(iBindingName);
  lua_pushstring(L, iSourcePropertyPath.c_str());
  pushJBoxValue(iNewValue);
  lua_pcall(L, 2, 0, 0);
  fMotherboard = nullptr;
}

//------------------------------------------------------------------------
// RealtimeController::getBindings
//------------------------------------------------------------------------
std::map<std::string, std::string> RealtimeController::getBindings()
{
  std::map<std::string, std::string> bindings{};
  if(lua_getglobal(L, "rtc_bindings") != LUA_TNIL)
  {
    auto mapIndex = lua_gettop(L);
    luaL_checktype(L, mapIndex, LUA_TTABLE);
    auto n = L.getTableSize(mapIndex);
    for(int i = 1; i <= n; i++)
    {
      lua_geti(L, mapIndex, i);
      auto source = L.getTableValueAsString("source");
      auto dest = L.getTableValueAsString("dest");
      RE_MOCK_ASSERT(dest.find("/global_rtc/") == 0, "invalid rtc_binding [%s]", dest.c_str());
      dest = dest.substr(12);  // skip /global_rtc/
      putBindingOnTopOfStack(dest); // this will check that the binding exists
      lua_pop(L, 1);
      bindings[source] = dest;
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);
  return bindings;
}

//------------------------------------------------------------------------
// RealtimeController::getRTInputSetupNotify
//------------------------------------------------------------------------
std::set<std::string> RealtimeController::getRTInputSetupNotify()
{
  std::set<std::string> res{};

  if(lua_getglobal(L, "rt_input_setup") != LUA_TNIL)
  {
    if(lua_getfield(L, -1, "notify") != LUA_TNIL)
    {
      auto n = L.getTableSize();
      for(int i = 1; i <= n; i++)
      {
        lua_geti(L, -1, i);
        res.emplace(lua_tostring(L, -1));
        lua_pop(L, 1);
      }
    }
    lua_pop(L, 1);
  }

  lua_pop(L, 1);

  return res;
}

//------------------------------------------------------------------------
// RealtimeController::putBindingOnTopOfStack
//------------------------------------------------------------------------
void RealtimeController::putBindingOnTopOfStack(std::string const &iBindingName)
{
  lua_getglobal(L, "global_rtc");
  auto mapIndex = lua_gettop(L);
  luaL_checktype(L, mapIndex, LUA_TTABLE);
  lua_getfield(L, mapIndex, iBindingName.c_str());
  luaL_argexpected(L, lua_type(L, -1) == LUA_TFUNCTION, -1, fmt::printf("/global_rtc/%s", iBindingName).c_str());
  lua_remove(L, -2); // remove global_rtc from stack
}



}