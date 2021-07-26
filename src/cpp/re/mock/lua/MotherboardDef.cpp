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
#include <re/mock/Errors.h>


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

//------------------------------------------------------------------------
// MotherboardDef::MotherboardDef
//------------------------------------------------------------------------
MotherboardDef::MotherboardDef()
{
  static const struct luaL_Reg jboxLib[] = {
    {"property_set",                  lua_property_set},
    {"native_object",                 lua_native_object},
    {"boolean",                       lua_boolean},
    {"string",                        lua_ignored},
    {"audio_input",                   lua_audio_input},
    {"audio_output",                  lua_audio_output},
    {"cv_input",                      lua_cv_input},
    {"cv_output",                     lua_cv_output},
    {"ui_text",                       lua_ignored},
    {"ui_selector",                   lua_ignored},
    {"ui_linear",                     lua_ignored},
    {"add_stereo_audio_routing_pair", lua_ignored},
    {nullptr,                         nullptr}
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
// MotherboardDef::addObject
//------------------------------------------------------------------------
int MotherboardDef::addObject(std::unique_ptr<jbox_object> iObject)
{
  auto ud = JBoxObjectUD::New(L);
  ud->fType = iObject->getType();
  ud->fId = fObjects.add(std::move(iObject));
  return 1;
}

//------------------------------------------------------------------------
// MotherboardDef::getObject
//------------------------------------------------------------------------
jbox_object *MotherboardDef::getObjectOnTopOfStack()
{
  if(lua_type(L, -1) == LUA_TNIL)
  {
    lua_pop(L, 1);
    return nullptr;
  }

  luaL_checktype(L, -1, LUA_TUSERDATA);
  auto ud = reinterpret_cast<JBoxObjectUD *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  auto o = fObjects.get(ud->fId).get();
  RE_MOCK_ASSERT(o->getType() == ud->fType, "type mismatch");
  return o;
}

//------------------------------------------------------------------------
// MotherboardDef::lua_ignored
//------------------------------------------------------------------------
int MotherboardDef::luaIgnored()
{
  return addObject(std::make_unique<jbox_ignored>());
}

//------------------------------------------------------------------------
// MotherboardDef::lua_native_object
//------------------------------------------------------------------------
int MotherboardDef::luaNativeObject()
{
  auto p = std::make_unique<jbox_native_object>();
  populatePropertyTag(p.get());
  return addObject(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::lua_boolean
//------------------------------------------------------------------------
int MotherboardDef::luaBoolean()
{
  auto p = std::make_unique<jbox_boolean_property>();
  populatePropertyTag(p.get());
  p->default_value = L.getTableValueAsBoolean("default_value");
  return addObject(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::lua_socket
//------------------------------------------------------------------------
int MotherboardDef::luaSocket(jbox_object::Type iSocketType)
{
  auto p = std::make_unique<jbox_socket>();
  p->type = iSocketType;
  return addObject(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::lua_property_set
//------------------------------------------------------------------------
int MotherboardDef::luaPropertySet()
{
  auto set = std::make_unique<jbox_property_set>();

  luaPropertySet("document_owner", set->document_owner);
  luaPropertySet("rtc_owner", set->rtc_owner);
  luaPropertySet("rt_owner", set->rt_owner);

  return addObject(std::move(set));
}

//------------------------------------------------------------------------
// MotherboardDef::lua_property_set
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
  lua_pop(L, 1);

  return sockets;
}

//------------------------------------------------------------------------
// MotherboardDef::populatePropertyTag
//------------------------------------------------------------------------
void MotherboardDef::populatePropertyTag(jbox_property *iProperty)
{
  iProperty->property_tag = static_cast<int>(L.getTableValueAsInteger("property_tag"));
}

//------------------------------------------------------------------------
// MotherboardDef::fromFile
//------------------------------------------------------------------------
std::unique_ptr<MotherboardDef> MotherboardDef::fromFile(std::string const &iLuaFilename)
{
  auto res = std::unique_ptr<MotherboardDef>(new MotherboardDef());
  res->L.runLuaFile(iLuaFilename);
  return res;
}

//------------------------------------------------------------------------
// MotherboardDef::fromString
//------------------------------------------------------------------------
std::unique_ptr<MotherboardDef> MotherboardDef::fromString(std::string const &iLuaCode)
{
  auto res = std::unique_ptr<MotherboardDef>(new MotherboardDef());
  res->L.runLuaCode(iLuaCode);
  return res;
}


}