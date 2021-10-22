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

static int lua_number_no_default(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaNumberNoDefault();
}

static int lua_pitchbend(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaNumberNoDefault(0.5);
}

static int lua_string(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaString();
}

static int lua_audio_input(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSocket(jbox_sockets::Type::AUDIO_INPUT);
}

static int lua_audio_output(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSocket(jbox_sockets::Type::AUDIO_OUTPUT);
}

static int lua_cv_input(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSocket(jbox_sockets::Type::CV_INPUT);
}

static int lua_cv_output(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSocket(jbox_sockets::Type::CV_OUTPUT);
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
    {"add_cv_routing_target",              lua_ignored},
    {"add_mono_audio_routing_target",      lua_ignored},
    {"add_mono_instrument_routing_hint",   lua_ignored},
    {"add_stereo_audio_routing_pair",      lua_ignored},
    {"add_stereo_audio_routing_target",    lua_ignored},
    {"add_stereo_effect_routing_hint",     lua_ignored},
    {"add_stereo_instrument_routing_hint", lua_ignored},
    {"audio_input",                        lua_audio_input},
    {"audio_output",                       lua_audio_output},
    {"blob",                               lua_ignored},
    {"boolean",                            lua_boolean},
    {"cv_input",                           lua_cv_input},
    {"cv_output",                          lua_cv_output},
    {"expand_template_string",             lua_ignored},
    {"format_integer_as_string",           lua_ignored},
    {"format_note_length_value_as_string", lua_ignored},
    {"format_number_as_string",            lua_ignored},
    {"native_object",                      lua_native_object},
    {"number",                             lua_number},
    {"performance_aftertouch",             lua_number_no_default},
    {"performance_breathcontrol",          lua_number_no_default},
    {"performance_expression",             lua_number_no_default},
    {"performance_modwheel",               lua_number_no_default},
    {"performance_pitchbend",              lua_pitchbend},
    {"performance_sustainpedal",           lua_number_no_default},
    {"property_set",                       lua_property_set},
    {"sample",                             lua_ignored},
    {"set_effect_auto_bypass_routing",     lua_ignored},
    {"string",                             lua_string},
    {"trace",                              lua_ignored},
    {"ui_linear",                          lua_ignored},
    {"ui_nonlinear",                       lua_ignored},
    {"ui_percent",                         lua_ignored},
    {"ui_selector",                        lua_ignored},
    {"ui_text",                            lua_ignored},
    {"user_sample",                        lua_ignored},
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
int MotherboardDef::addObjectOnTopOfStack(impl::jbox_object iObject)
{
  auto ud = JBoxObjectUD::New(L);
  ud->fId = fObjects.add(std::move(iObject));
  return 1;
}

//------------------------------------------------------------------------
// MotherboardDef::getObjectOnTopOfStack
//------------------------------------------------------------------------
std::optional<impl::jbox_object> MotherboardDef::getObjectOnTopOfStack()
{
  if(lua_type(L, -1) == LUA_TNIL)
  {
    lua_pop(L, 1);
    return std::nullopt;
  }

  luaL_checktype(L, -1, LUA_TUSERDATA);
  auto ud = reinterpret_cast<JBoxObjectUD *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return fObjects.get(ud->fId);
}

//------------------------------------------------------------------------
// MotherboardDef::luaIgnored
//------------------------------------------------------------------------
int MotherboardDef::luaIgnored()
{
  return addObjectOnTopOfStack(std::make_unique<impl::jbox_ignored>());
}

//------------------------------------------------------------------------
// MotherboardDef::toParam
//------------------------------------------------------------------------
jbox_native_object::param_t MotherboardDef::toParam(int idx)
{
  int t = lua_type(L, idx);
  switch(t)
  {
    case LUA_TBOOLEAN:
      return static_cast<bool>(lua_toboolean(L, idx));

    case LUA_TNUMBER:
      return static_cast<TJBox_Float64>(lua_tonumber(L, idx));

    default:
      RE_MOCK_ASSERT(false, "Invalid type for jbox.native_object param (only boolean or number allowed)");
      return false; // statement never reached (compiler does not know)
  }
}

//------------------------------------------------------------------------
// MotherboardDef::luaNativeObject
//------------------------------------------------------------------------
int MotherboardDef::luaNativeObject()
{
  auto p = std::make_shared<jbox_native_object>();
  populatePropertyTag(p);

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
      p->fDefaultValue.operation = lua_tostring(L, -1);
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
        p->fDefaultValue.params.emplace_back(toParam());
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
  auto p = std::make_shared<jbox_boolean_property>();
  populatePropertyTag(p);
  p->fDefaultValue = L.getTableValueAsBoolean("default", 1);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaNumber
//------------------------------------------------------------------------
int MotherboardDef::luaNumber()
{
  auto p = std::make_shared<jbox_number_property>();
  populatePropertyTag(p);
  p->fDefaultValue = L.getTableValueAsNumber("default", 1);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaNumberNoDefault
//------------------------------------------------------------------------
int MotherboardDef::luaNumberNoDefault(TJBox_Float64 iDefault)
{
  auto p = std::make_shared<jbox_number_property>();
  populatePropertyTag(p);
  p->fDefaultValue = iDefault;
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaString
//------------------------------------------------------------------------
int MotherboardDef::luaString()
{
  auto p = std::make_shared<jbox_string_property>();
  populatePropertyTag(p);
  p->fDefaultValue = L.getTableValueAsString("default", 1);
  p->fMaxSize = L.getTableValueAsNumber("max_size", 1);
  return addObjectOnTopOfStack(std::move(p));
}


//------------------------------------------------------------------------
// MotherboardDef::luaSocket
//------------------------------------------------------------------------
int MotherboardDef::luaSocket(jbox_sockets::Type iSocketType)
{
  auto p = std::make_unique<impl::jbox_socket>();
  p->fType = iSocketType;
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaPropertySet
//------------------------------------------------------------------------
int MotherboardDef::luaPropertySet()
{
  luaL_checktype(L, 1, LUA_TTABLE);
  auto p = std::make_unique<impl::jbox_property_set>(L);
  p->fCustomPropertiesRef = luaL_ref(L, LUA_REGISTRYINDEX);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaPropertySet
//------------------------------------------------------------------------
void MotherboardDef::luaPropertySet(char const *iKey, jbox_object_map_t &oMap)
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
// toJBoxProperty
//------------------------------------------------------------------------
std::optional<jbox_property> toJBoxProperty(std::optional<impl::jbox_object> iObject)
{
  struct visitor
  {
    std::optional<jbox_property> operator()(std::shared_ptr<impl::jbox_ignored>) { return std::nullopt; }
    std::optional<jbox_property> operator()(std::shared_ptr<jbox_native_object> o) { return o; }
    std::optional<jbox_property> operator()(std::shared_ptr<jbox_boolean_property> o) { return o; }
    std::optional<jbox_property> operator()(std::shared_ptr<jbox_number_property> o) { return o; }
    std::optional<jbox_property> operator()(std::shared_ptr<jbox_string_property> o) { return o; }
    std::optional<jbox_property> operator()(std::shared_ptr<impl::jbox_property_set>) { return std::nullopt; }
    std::optional<jbox_property> operator()(std::shared_ptr<impl::jbox_socket>) { return std::nullopt; }
  };

  if(!iObject)
    return std::nullopt;

  return std::visit(visitor{}, iObject.value());
}

//------------------------------------------------------------------------
// MotherboardDef::populateMapFromLuaTable
//------------------------------------------------------------------------
void MotherboardDef::populateMapFromLuaTable(jbox_object_map_t &oMap)
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
      auto object = getObjectOnTopOfStack();
      if(object)
        oMap[name] = object.value();
    }
  }
  lua_pop(L, 1);
}

//------------------------------------------------------------------------
// MotherboardDef::getSockets
//------------------------------------------------------------------------
std::unique_ptr<jbox_sockets> MotherboardDef::getSockets(char const *iSocketName, jbox_sockets::Type iSocketType)
{
  auto sockets = std::make_unique<jbox_sockets>();
  sockets->fType = iSocketType;
  if(lua_getglobal(L, iSocketName) != LUA_TNIL)
  {
    jbox_object_map_t m{};
    populateMapFromLuaTable(m);
    for(auto &iter : m)
    {
      RE_MOCK_ASSERT(std::get<std::shared_ptr<impl::jbox_socket>>(iter.second)->fType == iSocketType, "[%s] wrong socket type", iter.first.c_str());
      sockets->fNames.emplace_back(iter.first);
    }
  }
  else
    lua_pop(L, 1);

  return sockets;
}

//------------------------------------------------------------------------
// MotherboardDef::populatePropertyTag
//------------------------------------------------------------------------
void MotherboardDef::populatePropertyTag(jbox_property iProperty)
{
  RE_MOCK_ASSERT(lua_gettop(L) > 0, "Missing table... Did you use () instead of {}?");
  auto const propertyTag = static_cast<int>(L.getTableValueAsInteger("property_tag", 1));
  std::visit([propertyTag](auto &t) { t->fPropertyTag = propertyTag; }, iProperty);
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
    auto o = getObjectOnTopOfStack();

    if(o)
    {
      auto jps = std::get<std::shared_ptr<impl::jbox_property_set>>(o.value());

      lua_rawgeti(L, LUA_REGISTRYINDEX, jps->fCustomPropertiesRef);

      {
        jbox_object_map_t map{};
        luaPropertySet("document_owner", map);
        filter(map, set->document_owner);
      }

      {
        jbox_object_map_t map{};
        luaPropertySet("rtc_owner", map);
        filter(map, set->rtc_owner);
      }

      {
        jbox_object_map_t map{};
        luaPropertySet("rt_owner", map);
        filter(map, set->rt_owner);
      }
    }
  }

  lua_pop(L, 1);

  return set;
}

//------------------------------------------------------------------------
// jbox_property::filter
//------------------------------------------------------------------------
void MotherboardDef::filter(MotherboardDef::jbox_object_map_t &iMap, MotherboardDef::jbox_property_map_t &oMap)
{
  for(auto iter: iMap)
  {
    auto property = toJBoxProperty(iter.second);
    if(property)
      oMap[iter.first] = property.value();
  }
}

//------------------------------------------------------------------------
// jbox_property_set::jbox_property_set
//------------------------------------------------------------------------
impl::jbox_property_set::jbox_property_set(lua_State *iLuaState) : L{iLuaState} {}

//------------------------------------------------------------------------
// jbox_property_set::jbox_property_set
//------------------------------------------------------------------------
impl::jbox_property_set::~jbox_property_set()
{
  if(fCustomPropertiesRef != LUA_NOREF)
    luaL_unref(L, LUA_REGISTRYINDEX, fCustomPropertiesRef);
}


}