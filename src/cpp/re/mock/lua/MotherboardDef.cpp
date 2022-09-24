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

static int lua_blob(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaBlob();
}

static int lua_boolean(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaBoolean();
}

static int lua_number(lua_State *L) { return MotherboardDef::loadFromRegistry(L)->luaNumber(); }

static int lua_performance(lua_State *L, jbox_performance_property::Type iType)
{
  return MotherboardDef::loadFromRegistry(L)->luaPerformance(iType);
}

static int lua_performance_aftertouch(lua_State *L) { return lua_performance(L, jbox_performance_property::Type::AFTERTOUCH); }
static int lua_performance_breath_control(lua_State *L) { return lua_performance(L, jbox_performance_property::Type::BREATH_CONTROL); }
static int lua_performance_expression(lua_State *L) { return lua_performance(L, jbox_performance_property::Type::EXPRESSION); }
static int lua_performance_mod_wheel(lua_State *L) { return lua_performance(L, jbox_performance_property::Type::MOD_WHEEL); }
static int lua_performance_pitch_bend(lua_State *L) { return lua_performance(L, jbox_performance_property::Type::PITCH_BEND); }
static int lua_performance_sustain_pedal(lua_State *L) { return lua_performance(L, jbox_performance_property::Type::SUSTAIN_PEDAL); }

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

static int lua_sample(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaSample();
}

static int lua_user_sample(lua_State *L)
{
  return MotherboardDef::loadFromRegistry(L)->luaUserSample();
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
    {"blob",                               lua_blob},
    {"boolean",                            lua_boolean},
    {"cv_input",                           lua_cv_input},
    {"cv_output",                          lua_cv_output},
    {"expand_template_string",             lua_ignored},
    {"format_integer_as_string",           lua_ignored},
    {"format_note_length_value_as_string", lua_ignored},
    {"format_number_as_string",            lua_ignored},
    {"native_object",                      lua_native_object},
    {"number",                             lua_number},
    {"performance_aftertouch",             lua_performance_aftertouch},
    {"performance_breathcontrol",          lua_performance_breath_control},
    {"performance_expression",             lua_performance_expression},
    {"performance_modwheel",               lua_performance_mod_wheel},
    {"performance_pitchbend",              lua_performance_pitch_bend},
    {"performance_sustainpedal",           lua_performance_sustain_pedal},
    {"property_set",                       lua_property_set},
    {"sample",                             lua_sample},
    {"set_effect_auto_bypass_routing",     lua_ignored},
    {"string",                             lua_string},
    {"trace",                              lua_ignored},
    {"ui_linear",                          lua_ignored},
    {"ui_nonlinear",                       lua_ignored},
    {"ui_percent",                         lua_ignored},
    {"ui_selector",                        lua_ignored},
    {"ui_text",                            lua_ignored},
    {"user_sample",                        lua_user_sample},
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

    case LUA_TSTRING:
      return std::string(lua_tostring(L, idx));

    default:
      RE_MOCK_ASSERT(false, "Invalid type for jbox.native_object param (only boolean, number or string allowed)");
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
// MotherboardDef::luaBlob
//------------------------------------------------------------------------
int MotherboardDef::luaBlob()
{
  auto p = std::make_shared<jbox_blob_property>();
  populatePropertyTag(p);
  p->fDefaultValue = L.getTableValueAsOptionalString("default", 1);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaBoolean
//------------------------------------------------------------------------
int MotherboardDef::luaBoolean()
{
  auto p = std::make_shared<jbox_boolean_property>();
  populatePropertyTag(p);
  p->fPersistence = getPersistence();
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
  p->fPersistence = getPersistence();
  p->fDefaultValue = L.getTableValueAsNumber("default", 1);
  p->fSteps = L.getTableValueAsOptionalInteger("steps", 1);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// jbox_property::luaPerformance
//------------------------------------------------------------------------
int MotherboardDef::luaPerformance(jbox_performance_property::Type iType)
{
  auto p = std::make_shared<jbox_performance_property>();
  p->fType = iType;
  populatePropertyTag(p);
  p->fPersistence = getPersistence();
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaString
//------------------------------------------------------------------------
int MotherboardDef::luaString()
{
  auto p = std::make_shared<jbox_string_property>();
  populatePropertyTag(p);
  p->fPersistence = getPersistence();
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
// MotherboardDef::luaSample
//------------------------------------------------------------------------
int MotherboardDef::luaSample()
{
  auto p = std::make_shared<jbox_sample_property>();
  populatePropertyTag(p);
  p->fDefaultValue = L.getTableValueAsOptionalString("default", 1);
  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// MotherboardDef::luaUserSample
//------------------------------------------------------------------------
int MotherboardDef::luaUserSample()
{
  auto p = std::make_shared<jbox_user_sample_property>();
  p->fPersistence = getPersistence();

  lua_getfield(L, 1, "sample_parameters");
  iterateLuaTable([this, &p](lua_table_key_t key) {
    RE_MOCK_ASSERT(std::holds_alternative<int>(key), "malformed sample_parameters list");
    luaL_checktype(L, -1, LUA_TSTRING);
    p->sample_parameter(lua_tostring(L, -1));
    lua_pop(L, 1);
  });

  return addObjectOnTopOfStack(std::move(p));
}

//------------------------------------------------------------------------
// setDefaultPersistence
//------------------------------------------------------------------------
template<typename T>
void setDefaultPersistence(T o, EPersistence p) { if(!o->fPersistence) o->fPersistence = p; }

//------------------------------------------------------------------------
// setNoPersistence
//------------------------------------------------------------------------
template<typename T>
void setNoPersistence(std::string name, T o) {
  RE_MOCK_ASSERT(o->fPersistence == std::nullopt, "[%s] property cannot be persisted", name);
  o->fPersistence = EPersistence::kNone;
}

//------------------------------------------------------------------------
// to_gui_jbox_property
// From the doc: Properties in the gui_owner scope are not stored in
// documents and patches by default.
//------------------------------------------------------------------------
std::optional<gui_jbox_property> to_gui_jbox_property(std::string iKey, std::optional<impl::jbox_object> iObject)
{
  struct visitor
  {
    std::optional<gui_jbox_property> operator()(std::shared_ptr<impl::jbox_ignored>) { return std::nullopt; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<jbox_native_object>) { return std::nullopt; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<jbox_blob_property>) { return std::nullopt; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<jbox_sample_property>) { return std::nullopt; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<jbox_user_sample_property>) { return std::nullopt; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<jbox_performance_property>) { return std::nullopt; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<impl::jbox_property_set>) { return std::nullopt; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<impl::jbox_socket>) { return std::nullopt; }

    std::optional<gui_jbox_property> operator()(std::shared_ptr<jbox_boolean_property> o) { return o; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<jbox_number_property> o) { return o; }
    std::optional<gui_jbox_property> operator()(std::shared_ptr<jbox_string_property> o) { return o; }
  };

  if(!iObject)
    return std::nullopt;

  auto res = std::visit(visitor{}, iObject.value());
  if(res)
    std::visit([iKey](auto p) {
                 RE_MOCK_ASSERT(p->fPropertyTag == 0, "[%s] gui_owner property cannot have a property_tag", iKey);
                 setDefaultPersistence(p, EPersistence::kNone);
               },
               *res);
  return res;
}

//------------------------------------------------------------------------
// to_document_jbox_property
// From the doc: Properties in the document_owner scope are stored in
// song documents and patches by default (except for performance properties)
//------------------------------------------------------------------------
std::optional<document_jbox_property> to_document_jbox_property(std::string iKey, std::optional<impl::jbox_object> iObject)
{
  struct visitor
  {
    std::string fKey{};

    std::optional<document_jbox_property> operator()(std::shared_ptr<impl::jbox_ignored>) { return std::nullopt; }
    std::optional<document_jbox_property> operator()(std::shared_ptr<jbox_native_object>) { return std::nullopt; }
    std::optional<document_jbox_property> operator()(std::shared_ptr<jbox_blob_property>) { return std::nullopt; }
    std::optional<document_jbox_property> operator()(std::shared_ptr<jbox_sample_property>) { return std::nullopt; }
    std::optional<document_jbox_property> operator()(std::shared_ptr<jbox_user_sample_property>) { return std::nullopt; }
    std::optional<document_jbox_property> operator()(std::shared_ptr<impl::jbox_property_set>) { return std::nullopt; }
    std::optional<document_jbox_property> operator()(std::shared_ptr<impl::jbox_socket>) { return std::nullopt; }

    std::optional<document_jbox_property> operator()(std::shared_ptr<jbox_boolean_property> o) {
      setDefaultPersistence(o, EPersistence::kPatch);
      return o;
    }
    std::optional<document_jbox_property> operator()(std::shared_ptr<jbox_number_property> o) {
      setDefaultPersistence(o, EPersistence::kPatch);
      return o;
    }
    std::optional<document_jbox_property> operator()(std::shared_ptr<jbox_string_property> o) {
      setDefaultPersistence(o, EPersistence::kPatch);
      return o;
    }
    std::optional<document_jbox_property> operator()(std::shared_ptr<jbox_performance_property> o) {
      RE_MOCK_ASSERT(o->fType != jbox_performance_property::Type::UNKNOWN);
      switch(o->fType)
      {
        case jbox_performance_property::Type::MOD_WHEEL:
        case jbox_performance_property::Type::EXPRESSION:
          break;

        default:
          // should not be reached
          RE_MOCK_ASSERT(o->fPersistence == std::nullopt,
                         "[%s] only modwhell and expression properties can be persisted",
                         fKey);
          break;
      }
      setDefaultPersistence(o, EPersistence::kNone);
      return o;
    }
  };

  if(!iObject)
    return std::nullopt;

  return std::visit(visitor{iKey}, iObject.value());
}

//------------------------------------------------------------------------
// to_rtc_jbox_property
// Cannot be persisted
//------------------------------------------------------------------------
std::optional<rtc_jbox_property> to_rtc_jbox_property(std::string iKey, std::optional<impl::jbox_object> iObject)
{
  struct visitor
  {
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<impl::jbox_ignored>) { return std::nullopt; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<jbox_performance_property>) { return std::nullopt; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<impl::jbox_property_set>) { return std::nullopt; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<impl::jbox_socket>) { return std::nullopt; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<jbox_user_sample_property>) { return std::nullopt; }

    std::optional<rtc_jbox_property> operator()(std::shared_ptr<jbox_native_object> o) { return o; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<jbox_blob_property> o) { return o; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<jbox_sample_property> o) { return o; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<jbox_boolean_property> o) { return o; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<jbox_number_property> o) { return o; }
    std::optional<rtc_jbox_property> operator()(std::shared_ptr<jbox_string_property> o) { return o; }
  };

  if(!iObject)
    return std::nullopt;

  auto res = std::visit(visitor{}, iObject.value());
  if(res)
    std::visit([iKey](auto p) { setNoPersistence(iKey, p); }, *res);
  return res;
}

//------------------------------------------------------------------------
// to_rt_jbox_property
// Cannot be persisted
//------------------------------------------------------------------------
std::optional<rt_jbox_property> to_rt_jbox_property(std::string iKey, std::optional<impl::jbox_object> iObject)
{
  struct visitor
  {
    std::optional<rt_jbox_property> operator()(std::shared_ptr<impl::jbox_ignored>) { return std::nullopt; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<jbox_native_object>) { return std::nullopt; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<jbox_blob_property>) { return std::nullopt; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<jbox_performance_property>) { return std::nullopt; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<impl::jbox_property_set>) { return std::nullopt; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<impl::jbox_socket>) { return std::nullopt; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<jbox_user_sample_property>) { return std::nullopt; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<jbox_sample_property>) { return std::nullopt; }

    std::optional<rt_jbox_property> operator()(std::shared_ptr<jbox_boolean_property> o) { return o; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<jbox_number_property> o) { return o; }
    std::optional<rt_jbox_property> operator()(std::shared_ptr<jbox_string_property> o) { return o; }
  };

  if(!iObject)
    return std::nullopt;

  auto res = std::visit(visitor{}, iObject.value());
  if(res)
    std::visit([iKey](auto p) { setNoPersistence(iKey, p); }, *res);
  return res;
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
std::unique_ptr<jbox_sockets> MotherboardDef::getSockets(char const *iSocketName, jbox_sockets::Type iSocketType) const
{
  // implementation detail: the method is not changing the state of the object
  // but all lua APIs technically modify the state (L)
  auto def = const_cast<MotherboardDef *>(this);
  return def->doGetSockets(iSocketName, iSocketType);
}

//------------------------------------------------------------------------
// MotherboardDef::doGetSockets
//------------------------------------------------------------------------
std::unique_ptr<jbox_sockets> MotherboardDef::doGetSockets(char const *iSocketName, jbox_sockets::Type iSocketType)
{
  auto sockets = std::make_unique<jbox_sockets>();
  sockets->fType = iSocketType;
  if(lua_getglobal(L, iSocketName) != LUA_TNIL)
  {
    jbox_object_map_t m{};
    populateMapFromLuaTable(m);
    for(auto &iter : m)
    {
      RE_MOCK_ASSERT(std::get<std::shared_ptr<impl::jbox_socket>>(iter.second)->fType == iSocketType, "[%s] wrong socket type", iter.first);
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
// MotherboardDef::getPersistence
//------------------------------------------------------------------------
std::optional<EPersistence> MotherboardDef::getPersistence()
{
  RE_MOCK_ASSERT(lua_gettop(L) > 0, "Missing table... Did you use () instead of {}?");

  std::optional<EPersistence> persistence = std::nullopt;

  auto persistenceString = L.getTableValueAsOptionalString("persistence", 1);
  if(persistenceString)
  {
    if(*persistenceString == "patch")
      persistence = EPersistence::kPatch;
    else if(*persistenceString == "song")
      persistence = EPersistence::kSong;
    else
      RE_MOCK_ASSERT(*persistenceString == "none", "persistence [%s] should be patch/song/none", persistenceString->c_str());
  }

  return persistence;
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
std::shared_ptr<JboxPropertySet> MotherboardDef::getCustomProperties() const
{
  // implementation detail: the method is not changing the state of the object
  // but all lua APIs technically modify the state (L)
  auto def = const_cast<MotherboardDef *>(this);
  return def->doGetCustomProperties();
}

//------------------------------------------------------------------------
// MotherboardDef::getCustomProperties
//------------------------------------------------------------------------
std::shared_ptr<JboxPropertySet> MotherboardDef::doGetCustomProperties()
{
  if(fCustomProperties)
    return fCustomProperties;

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
        luaPropertySet("gui_owner", map);
        filter<gui_jbox_property>(map, set->gui_owner, to_gui_jbox_property);
      }

      {
        jbox_object_map_t map{};
        luaPropertySet("document_owner", map);
        filter<document_jbox_property>(map, set->document_owner, to_document_jbox_property);
      }

      {
        jbox_object_map_t map{};
        luaPropertySet("rtc_owner", map);
        filter<rtc_jbox_property>(map, set->rtc_owner, to_rtc_jbox_property);
      }

      {
        jbox_object_map_t map{};
        luaPropertySet("rt_owner", map);
        filter<rt_jbox_property>(map, set->rt_owner, to_rt_jbox_property);
      }
    }
  }

  lua_pop(L, 1);

  lua_getglobal(L, "user_samples");
  int stringKeyCount = 0;
  int indexKeyCount = 0;
  iterateLuaTable([this, &set, &stringKeyCount, &indexKeyCount](lua_table_key_t key) {
    auto userSample = std::get<std::shared_ptr<jbox_user_sample_property>>(*getObjectOnTopOfStack());
    setDefaultPersistence(userSample, EPersistence::kPatch);
    if(std::holds_alternative<std::string>(key))
    {
      RE_MOCK_ASSERT(indexKeyCount == 0, "Cannot mix and match named and indexed sample in user_samples");
      userSample->name(std::get<std::string>(key));
      stringKeyCount++;
    }
    else
    {
      RE_MOCK_ASSERT(stringKeyCount == 0, "Cannot mix and match named and indexed sample in user_samples");
      indexKeyCount++;
    }
    set->user_samples.emplace_back(userSample);
  });

  fCustomProperties = std::move(set);

  return fCustomProperties;
}

//------------------------------------------------------------------------
// MotherboardDef::filter
//------------------------------------------------------------------------
template<typename jbox_property_type>
void MotherboardDef::filter(jbox_object_map_t &iMap, std::map<std::string, jbox_property_type> &oMap,
                            std::function<std::optional<jbox_property_type>(std::string, std::optional<impl::jbox_object>)> iFilter)
{
  for(auto iter: iMap)
  {
    auto property = iFilter(iter.first, iter.second);
    if(property)
      oMap[iter.first] = property.value();
  }
}

//------------------------------------------------------------------------
// MotherboardDef::getNumPatterns
//------------------------------------------------------------------------
int MotherboardDef::getNumPatterns()
{
  int res = 0;
  if(lua_getglobal(L, "patterns") != LUA_TNIL)
  {
    res = L.getTableValueAsInteger("num_patterns");
    RE_MOCK_ASSERT(res >= 1 && res <= 32, "num_patterns [%d] must be between 1 and 32", res);
  }

  lua_pop(L, 1);

  return res;
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