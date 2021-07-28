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

#include "MotherboardDef.h"
#include <Jukebox.h>
#include <re/mock/Errors.h>
#include <re/mock/Motherboard.h>


//------------------------------------------------------------------------
// Defining the C-API to invoke from lua as jbox.xxx
//------------------------------------------------------------------------
extern "C" {

using namespace re::mock::lua;

static int lua_native_object(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaNativeObject();
}

static int lua_boolean(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaBoolean();
}

static int lua_number(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaNumber();
}

static int lua_audio_input(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSocket(jbox_object::Type::AUDIO_INPUT);
}

static int lua_audio_output(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSocket(jbox_object::Type::AUDIO_OUTPUT);
}

static int lua_cv_input(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSocket(jbox_object::Type::CV_INPUT);
}

static int lua_cv_output(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSocket(jbox_object::Type::CV_OUTPUT);
}

static int lua_property_set(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaPropertySet();
}

static int lua_ignored(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaIgnored();
}

}

namespace re::mock::lua {

struct JBoxObjectUD
{
  jbox_object::Type fType;
  int fId;

  static JBoxObjectUD *New(lua_State *L)
  {
    return reinterpret_cast<JBoxObjectUD *>(lua_newuserdata(L, sizeof(JBoxObjectUD)));
  }
};

//------------------------------------------------------------------------
// MotherboardDef::MotherboardDef
//------------------------------------------------------------------------
MotherboardDef::MotherboardDef()
{
  static const struct luaL_Reg jboxLib[] = {
    {"property_set",                       lua_property_set},
    {"native_object",                      lua_native_object},
    {"boolean",                            lua_boolean},
    {"number" ,                            lua_number},
    {"string",                             lua_ignored},
    {"audio_input",                        lua_audio_input},
    {"audio_output",                       lua_audio_output},
    {"cv_input",                           lua_cv_input},
    {"cv_output",                          lua_cv_output},
    {"ui_text",                            lua_ignored},
    {"ui_selector",                        lua_ignored},
    {"ui_linear",                          lua_ignored},
    {"ui_nonlinear",                       lua_ignored},
    {"add_cv_routing_target",              lua_ignored},
    {"add_mono_audio_routing_target",      lua_ignored},
    {"add_stereo_audio_routing_target",    lua_ignored},
    {"add_stereo_audio_routing_pair",      lua_ignored},
    {"add_stereo_effect_routing_hint",     lua_ignored},
    {"add_stereo_instrument_routing_hint", lua_ignored},
    {"add_mono_instrument_routing_hint",   lua_ignored},
    {"set_effect_auto_bypass_routing",     lua_ignored},
    {nullptr,                              nullptr}
  };

  luaL_newlib(L, jboxLib);
  lua_setglobal(L, "jbox"); // will be available in motherboard_def.lua as jbox
}

//------------------------------------------------------------------------
// MotherboardDef::loadFromRegistry
//------------------------------------------------------------------------
MotherboardDef *MotherboardDef::loadFromRegistry(lua_State *L)
{
  auto res = dynamic_cast<MotherboardDef *>(MockJBox::loadFromRegistry(L));
  RE_MOCK_ASSERT(res != nullptr);
  return res;
}

//------------------------------------------------------------------------
// MotherboardDef::addObjectOnTopOfStack
//------------------------------------------------------------------------
int MotherboardDef::addObjectOnTopOfStack(std::unique_ptr<jbox_object> iObject)
{
  auto ud = JBoxObjectUD::New(L);
  ud->fType = iObject->getType();
  ud->fId = fObjects.add(std::move(iObject));
  return 1;
}

//------------------------------------------------------------------------
// MotherboardDef::getObjectOnTopOfStack
//------------------------------------------------------------------------
std::shared_ptr<jbox_object> MotherboardDef::getObjectOnTopOfStack()
{
  if(lua_type(L, -1) == LUA_TNIL)
  {
    lua_pop(L, 1);
    return nullptr;
  }

  luaL_checktype(L, -1, LUA_TUSERDATA);
  auto ud = reinterpret_cast<JBoxObjectUD *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  auto o = fObjects.get(ud->fId);
  RE_MOCK_ASSERT(o->getType() == ud->fType, "type mismatch");
  return o;
}

//------------------------------------------------------------------------
// MotherboardDef::luaIgnored
//------------------------------------------------------------------------
int MotherboardDef::luaIgnored()
{
  return addObjectOnTopOfStack(std::make_unique<jbox_ignored>());
}

//------------------------------------------------------------------------
// MotherboardDef::luaNativeObject
//------------------------------------------------------------------------
int MotherboardDef::luaNativeObject()
{
  auto p = std::make_unique<jbox_native_object>();
  populatePropertyTag(p.get());

  // handling default = { "Operation", { param1, param2, ... } }
  if(lua_getfield(L, 1, "default") != LUA_TNIL)
  {
    auto defaultTableIdx = lua_gettop(L);

    auto size = L.getTableSize(defaultTableIdx); // actually an array

    // Operation
    if(size > 0)
    {
      lua_geti(L, defaultTableIdx, 1);
      luaL_checktype(L, -1, LUA_TSTRING);
      p->default_value.operation = lua_tostring(L, -1);
      lua_pop(L, 1);
    }

    // params
    if(size > 1)
    {
      lua_geti(L, defaultTableIdx, 2);

      auto paramsTableIdx = lua_gettop(L);

      auto numParams = L.getTableSize(paramsTableIdx);
      for(int i = 1; i <= numParams; i++)
      {
        lua_geti(L, paramsTableIdx, i);
        p->default_value.params.emplace_back(toJBoxValue());
        lua_pop(L, 1);
      }
      lua_pop(L, 1);
    }

  }
  lua_pop(L, 1);

  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaBoolean
//------------------------------------------------------------------------
int MotherboardDef::luaBoolean()
{
  auto p = std::make_unique<jbox_boolean_property>();
  populatePropertyTag(p.get());
  p->default_value = L.getTableValueAsBoolean("default", 1);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaNumber
//------------------------------------------------------------------------
int MotherboardDef::luaNumber()
{
  auto p = std::make_unique<jbox_number_property>();
  populatePropertyTag(p.get());
  p->default_value = L.getTableValueAsNumber("default", 1);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaSocket
//------------------------------------------------------------------------
int MotherboardDef::luaSocket(jbox_object::Type iSocketType)
{
  auto p = std::make_unique<jbox_socket>();
  p->type = iSocketType;
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaPropertySet
//------------------------------------------------------------------------
int MotherboardDef::luaPropertySet()
{
  luaL_checktype(L, 1, LUA_TTABLE);
  auto p = std::make_unique<jbox_property_set>(L);
  p->custom_properties_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaPropertySet
//------------------------------------------------------------------------
void MotherboardDef::luaPropertySet(char const *iKey, jbox_object::map_t &oMap)
{
  luaL_checktype(L, 1, LUA_TTABLE);

  if(lua_getfield(L, 1, iKey) != LUA_TNIL)
  {
    luaL_checktype(L, 2, LUA_TTABLE);
    if(lua_getfield(L, 2, "properties") != LUA_TNIL)
      populateMapFromLuaTable(oMap);
  }

  lua_pop(L, 1);
}

//------------------------------------------------------------------------
// MotherboardDef::populateMapFromLuaTable
//------------------------------------------------------------------------
void MotherboardDef::populateMapFromLuaTable(jbox_object::map_t &oMap)
{
  int mapStackIndex = lua_gettop(L);
  // check for NIL
  if(lua_type(L, mapStackIndex) != LUA_TNIL)
  {
    luaL_checktype(L, mapStackIndex, LUA_TTABLE);

    lua_pushnil(L);  /* first key */
    while(lua_next(L, mapStackIndex) != 0)
    {
      luaL_checktype(L, -2, LUA_TSTRING);
      auto name = lua_tostring(L, -2);
      oMap[name] = getObjectOnTopOfStack();
    }

  }
  lua_pop(L, 1);
}

//------------------------------------------------------------------------
// MotherboardDef::getSockets
//------------------------------------------------------------------------
std::unique_ptr<jbox_sockets> MotherboardDef::getSockets(char const *iSocketName,
                                                         jbox_object::Type iSocketsType,
                                                         jbox_object::Type iSocketType)
{
  auto sockets = std::make_unique<jbox_sockets>();
  sockets->type = iSocketsType;
  if(lua_getglobal(L, iSocketName) != LUA_TNIL)
  {
    jbox_object::map_t m{};
    populateMapFromLuaTable(m);
    for(auto &iter : m)
    {
      RE_MOCK_ASSERT(iter.second->getType() == iSocketType, "[%s] wrong socket type", iter.first.c_str());
      sockets->names.emplace_back(iter.first);
    }
  }
  else
    lua_pop(L, 1);

  return sockets;
}

//------------------------------------------------------------------------
// MotherboardDef::populatePropertyTag
//------------------------------------------------------------------------
void MotherboardDef::populatePropertyTag(jbox_property *iProperty)
{
  RE_MOCK_ASSERT(lua_gettop(L) > 0, "Missing table... Did you use () instead of {}?");
  iProperty->property_tag = static_cast<int>(L.getTableValueAsInteger("property_tag", 1));
}

//------------------------------------------------------------------------
// MotherboardDef::fromFile
//------------------------------------------------------------------------
std::unique_ptr<MotherboardDef> MotherboardDef::fromFile(std::string const &iLuaFilename)
{
  auto res = std::make_unique<MotherboardDef>();
  res->loadFile(iLuaFilename);
  return res;
}

//------------------------------------------------------------------------
// MotherboardDef::fromString
//------------------------------------------------------------------------
std::unique_ptr<MotherboardDef> MotherboardDef::fromString(std::string const &iLuaCode)
{
  auto res = std::make_unique<MotherboardDef>();
  res->loadString(iLuaCode);
  return res;
}

//------------------------------------------------------------------------
// MotherboardDef::getCustomProperties
//------------------------------------------------------------------------
std::unique_ptr<JboxPropertySet> MotherboardDef::getCustomProperties()
{
  auto set = std::make_unique<JboxPropertySet>();

  if(lua_getglobal(L, "custom_properties") != LUA_TNIL)
  {
    auto o = getObjectOnTopOfStack()->withType<jbox_property_set>();

    if(o)
    {
      lua_rawgeti(L, LUA_REGISTRYINDEX, o->custom_properties_ref);

      luaPropertySet("document_owner", set->document_owner);
      luaPropertySet("rtc_owner", set->rtc_owner);
      luaPropertySet("rt_owner", set->rt_owner);
    }
  }

  lua_pop(L, 1);

  return set;
}

//------------------------------------------------------------------------
// jbox_property::getDefaultValue
//------------------------------------------------------------------------
TJBox_Value jbox_property::getDefaultValue() const
{
  return JBox_MakeNil();
}

//------------------------------------------------------------------------
// jbox_boolean_property::getDefaultValue
//------------------------------------------------------------------------
TJBox_Value jbox_boolean_property::getDefaultValue() const
{
  return JBox_MakeBoolean(default_value);
}

//------------------------------------------------------------------------
// jbox_number_property::getDefaultValue
//------------------------------------------------------------------------
TJBox_Value jbox_number_property::getDefaultValue() const
{
  return JBox_MakeNumber(default_value);
}

//------------------------------------------------------------------------
// jbox_native_object::computeDefaultValue
//------------------------------------------------------------------------
TJBox_Value jbox_native_object::computeDefaultValue(Motherboard *iMotherboard) const
{
  if(!default_value.operation.empty())
    return iMotherboard->makeNativeObjectRW(default_value.operation, default_value.params);
  else
    return JBox_MakeNil();
}

//------------------------------------------------------------------------
// jbox_property_set::jbox_property_set
//------------------------------------------------------------------------
jbox_property_set::jbox_property_set(lua_State *iLuaState) : L{iLuaState} {}

//------------------------------------------------------------------------
// jbox_property_set::jbox_property_set
//------------------------------------------------------------------------
jbox_property_set::~jbox_property_set()
{
  if(custom_properties_ref != LUA_NOREF)
    luaL_unref(L, LUA_REGISTRYINDEX, custom_properties_ref);
}


}