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

static int lua_is_native_object(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaIsNativeObject();
}

static int lua_is_blob(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaIsBlob();
}

static int lua_make_empty_native_object(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaMakeNil();
}

static int lua_make_empty_blob(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaMakeEmptyBlob();
}

static int lua_load_blob_async(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaLoadBlobAsync();
}

static int lua_get_blob_info(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaGetBlobInfo();
}

static int lua_is_sample(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaIsSample();
}

static int lua_make_empty_sample(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaMakeEmptySample();
}

static int lua_get_sample_meta_data(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaGetSampleMetaData();
}

static int lua_get_sample_info(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaGetSampleInfo();
}

static int lua_load_sample_async(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaLoadSampleAsync();
}

static int lua_trace(lua_State *L)
{
  return RealtimeController::loadFromRegistry(L)->luaTrace();
}

}

namespace re::mock::lua {

//------------------------------------------------------------------------
// RealtimeController::RealtimeController
//------------------------------------------------------------------------
RealtimeController::RealtimeController()
{
  static const struct luaL_Reg jboxLib[] = {
    {"get_blob_info",            lua_get_blob_info},
    {"get_sample_info",          lua_get_sample_info},
    {"get_sample_meta_data",     lua_get_sample_meta_data},
    {"is_blob",                  lua_is_blob},
    {"is_native_object",         lua_is_native_object},
    {"is_sample",                lua_is_sample},
    {"load_blob_async",          lua_load_blob_async},
    {"load_property",            lua_load_property},
    {"load_sample_async",        lua_load_sample_async},
    {"make_empty_blob",          lua_make_empty_blob},
    {"make_empty_native_object", lua_make_empty_native_object},
    {"make_empty_sample",        lua_make_empty_sample},
    {"make_native_object_rw",    lua_make_native_object_rw},
    {"make_native_object_ro",    lua_make_native_object_ro},
    {"store_property",           lua_store_property},
    {"trace",                    lua_trace},
    {nullptr,                    nullptr}
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
std::unique_ptr<RealtimeController> RealtimeController::fromFile(fs::path const &iLuaFilename)
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
// RealtimeController::luaTrace
//------------------------------------------------------------------------
int RealtimeController::luaTrace()
{
  RE_MOCK_ASSERT(lua_gettop(L) == 1, "jbox.trace() is expecting 1 argument");
  int t = lua_type(L, 1);
  luaL_argexpected(L, t == LUA_TSTRING, 1, "jbox.trace() is expecting a string argument");
  lua_Debug ar;
  lua_getstack(L, 1, &ar);
  lua_getinfo(L, "nSl", &ar);
  auto s = lua_tostring(L, -1);
  if(s != nullptr)
    getCurrentMotherboard()->trace("realtime_controller.lua", ar.currentline, s);
  return 0;
}

//------------------------------------------------------------------------
// RealtimeController::luaLoadProperty
//------------------------------------------------------------------------
int RealtimeController::luaLoadProperty()
{
  luaL_checktype(L, 1, LUA_TSTRING);
  auto const propertyPath = lua_tostring(L, 1);

  RE_MOCK_ASSERT(stl::contains_key(fReverseBindings.at(fCurrentBindingName), propertyPath),
                 "Load property failed while executing binding [/global_rtc/%s]. Can only read property [%s] in rtc_bindings function with this property as source.",
                 fCurrentBindingName, propertyPath);

  auto value = getCurrentMotherboard()->getJboxValue(propertyPath);
  pushJBoxValue(value);
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaStoreProperty
//------------------------------------------------------------------------
int RealtimeController::luaStoreProperty()
{
  luaL_checktype(L, 1, LUA_TSTRING);
  auto const propertyPath = lua_tostring(L, 1);
  RE_MOCK_ASSERT(getCurrentMotherboard()->getPropertyOwner(propertyPath) == PropertyOwner::kRTCOwner);
  getCurrentMotherboard()->setValue(propertyPath, toJBoxValue(2));
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

  std::vector<std::shared_ptr<const JboxValue>> params{};

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
// RealtimeController::luaMakeNil
//------------------------------------------------------------------------
int RealtimeController::luaMakeNil()
{
  pushJBoxValue(getCurrentMotherboard()->makeNil());
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaIsNativeObject
//------------------------------------------------------------------------
int RealtimeController::luaIsNativeObject()
{
  RE_MOCK_ASSERT(lua_gettop(L) == 1, "jbox.is_native_object() expects 1 argument");
  auto type = toJBoxValue()->getValueType();
  lua_pushboolean(L, type == kJBox_NativeObject || type == kJBox_Nil);
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaMakeEmptyBlob
//------------------------------------------------------------------------
int RealtimeController::luaMakeEmptyBlob()
{
  pushJBoxValue(getCurrentMotherboard()->makeEmptyBlob());
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaIsBlob
//------------------------------------------------------------------------
int RealtimeController::luaIsBlob()
{
  RE_MOCK_ASSERT(lua_gettop(L) == 1, "jbox.is_blob() expects 1 argument");
  auto type = toJBoxValue()->getValueType();
  lua_pushboolean(L, type == kJBox_BLOB);
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaLoadBlobAsync
//------------------------------------------------------------------------
int RealtimeController::luaLoadBlobAsync()
{
  luaL_checktype(L, 1, LUA_TSTRING);
  pushJBoxValue(getCurrentMotherboard()->loadBlobAsync(lua_tostring(L, 1)));
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaGetBlobInfo
//------------------------------------------------------------------------
int RealtimeController::luaGetBlobInfo()
{
  RE_MOCK_ASSERT(lua_gettop(L) == 1, "jbox.get_blob_info() expects 1 argument");
  auto blobValue = toJBoxValue();
  lua_newtable(L);
  if(blobValue->getValueType() == kJBox_Nil)
    L.setTableValue("state", 0);
  else
  {
    auto info = getCurrentMotherboard()->getBLOBInfo(*blobValue);
    L.setTableValue("size", info.getSize());
    L.setTableValue("resident_size", info.getResidentSize());
    L.setTableValue("state", info.fLoadingContext.getStatusAsInt());
  }
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaIsSample
//------------------------------------------------------------------------
int RealtimeController::luaIsSample()
{
  RE_MOCK_ASSERT(lua_gettop(L) == 1, "jbox.is_sample() expects 1 argument");
  auto type = toJBoxValue()->getValueType();
  lua_pushboolean(L, type == kJBox_Sample);
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaMakeEmptySample
//------------------------------------------------------------------------
int RealtimeController::luaMakeEmptySample()
{
  pushJBoxValue(getCurrentMotherboard()->makeEmptySample());
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaLoadSampleAsync
//------------------------------------------------------------------------
int RealtimeController::luaLoadSampleAsync()
{
  luaL_checktype(L, 1, LUA_TSTRING);
  pushJBoxValue(getCurrentMotherboard()->loadSampleAsync(lua_tostring(L, 1)));
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaGetSampleMetaData
//------------------------------------------------------------------------
int RealtimeController::luaGetSampleInfo()
{
  RE_MOCK_ASSERT(lua_gettop(L) == 1, "jbox.get_sample_info() expects 1 argument");
  auto sampleValue = toJBoxValue();
  lua_newtable(L);
  if(sampleValue->getValueType() == kJBox_Nil)
    L.setTableValue("state", 0);
  else
  {
    auto metadata = getCurrentMotherboard()->getSampleMetadata(*sampleValue);
    auto &md = metadata.fMain;
    L.setTableValue("state", metadata.fLoadingContext.getStatusAsInt());
    L.setTableValue("frame_count", md.fSpec.fFrameCount);
    L.setTableValue("resident_count", md.fResidentFrameCount);
    L.setTableValue("channels", md.fSpec.fChannels);
    L.setTableValue("sample_rate", md.fSpec.fSampleRate);
  }
  return 1;
}

//------------------------------------------------------------------------
// RealtimeController::luaGetSampleMetaData
//------------------------------------------------------------------------
int RealtimeController::luaGetSampleMetaData()
{
  RE_MOCK_ASSERT(lua_gettop(L) == 1, "jbox.get_sample_metadata() expects 1 argument");
  auto sampleValue = toJBoxValue();
  lua_newtable(L);
  if(sampleValue->getValueType() == kJBox_Nil)
    L.setTableValue("load_status", "nil");
  else
  {
    auto metadata = getCurrentMotherboard()->getSampleMetadata(*sampleValue);
    auto &md = metadata.fMain;
    L.setTableValue("load_status", metadata.fLoadingContext.getStatusAsString());
    L.setTableValue("frame_count", md.fSpec.fFrameCount);
    L.setTableValue("resident_frame_count", md.fResidentFrameCount);
    L.setTableValue("channels", md.fSpec.fChannels);
    L.setTableValue("sample_rate", md.fSpec.fSampleRate);
    L.setTableValue("sample_name", metadata.getSampleName());

    auto &params = metadata.fMain.fParameters;
    L.setTableValue("root_key", params.fRootNote);
    L.setTableValue("tune_cents", params.fTuneCents);
    L.setTableValue("play_range_start", params.fPlayRangeStart);
    L.setTableValue("play_range_end", params.fPlayRangeEnd);
    L.setTableValue("loop_range_start", params.fLoopRangeStart);
    L.setTableValue("loop_range_end", params.fLoopRangeEnd);
    L.setTableValue("preview_volume_level", params.fVolumeLevel);
    L.setTableValue("loop_mode", metadata.getLoopModeAsString());
  }
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

namespace impl {
extern int error_handler(lua_State *L);
}

//------------------------------------------------------------------------
// RealtimeController::invokeBinding
//------------------------------------------------------------------------
void RealtimeController::invokeBinding(Motherboard *iMotherboard,
                                       std::string const &iBindingName,
                                       std::string const &iSourcePropertyPath,
                                       std::shared_ptr<const JboxValue> const &iNewValue)
{
  RE_MOCK_ASSERT(fMotherboard == nullptr, "calling binding from a binding"); // sanity check

  RE_MOCK_ASSERT(stl::contains_key(getBindings(), iSourcePropertyPath), "No rtc binding found for property [%s]", iSourcePropertyPath);
  RE_MOCK_ASSERT(stl::contains_key(fReverseBindings, iBindingName), "No rtc binding named [%s] found", iBindingName);
  RE_MOCK_ASSERT(stl::contains_key(fReverseBindings.at(iBindingName), iSourcePropertyPath), "Property [%s] is not a source for rtc binding [%s]", iSourcePropertyPath, iBindingName);

  lua_pushcfunction(L, impl::error_handler);
  auto msgh = lua_gettop(L);

  fMotherboard = iMotherboard;
  fCurrentBindingName = iBindingName;
  putBindingOnTopOfStack(iBindingName);
  lua_pushstring(L, iSourcePropertyPath.c_str());
  pushJBoxValue(iNewValue);
  auto const res = lua_pcall(L, 2, 0, msgh);
  if(res != LUA_OK)
  {
    lua_rawgeti(L, -1, 1);
    std::string errorMsg{lua_tostring(L, -1)};
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    auto lineNumber = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 3);
    auto source = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_pop(L, 1);

    LuaException::throwException({source, lineNumber}, "Error executing binding %s(%s, %s) | %s",
                                 iBindingName.c_str(),
                                 iSourcePropertyPath.c_str(),
                                 fMotherboard->toString(*iNewValue).c_str(),
                                 errorMsg);
  }
  fCurrentBindingName = "";
  fMotherboard = nullptr;
  fJboxValues.reset();

  // remove the error_handler from the stack
  lua_pop(L, 1);
}

//------------------------------------------------------------------------
// RealtimeController::getBindings
//------------------------------------------------------------------------
std::map<std::string, std::set<std::string>> const &RealtimeController::getBindings()
{
  if(!fBindings)
  {
    std::map<std::string, std::set<std::string>> bindings{};
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
        RE_MOCK_ASSERT(dest.find("/global_rtc/") == 0, "invalid rtc_binding [%s]", dest);
        dest = dest.substr(12);  // skip /global_rtc/
        putBindingOnTopOfStack(dest); // this will check that the binding exists
        lua_pop(L, 1);
        if(!stl::contains_key(bindings, source))
          bindings[source] = {};
        bindings[source].emplace(dest);

        // also add it to reverse bindings
        if(!stl::contains_key(fReverseBindings, dest))
          fReverseBindings[dest] = {};
        fReverseBindings[dest].emplace(source);

        lua_pop(L, 1);
      }
    }
    lua_pop(L, 1);
    fBindings = std::move(bindings);
  }
  return *fBindings;
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

//------------------------------------------------------------------------
// RealtimeController::toJBoxValue
//------------------------------------------------------------------------
std::shared_ptr<const JboxValue> RealtimeController::toJBoxValue(int idx)
{
  int t = lua_type(L, idx);
  switch(t)
  {
    case LUA_TBOOLEAN:
      return getCurrentMotherboard()->makeBoolean(lua_toboolean(L, idx));

    case LUA_TNUMBER:
      return getCurrentMotherboard()->makeNumber(lua_tonumber(L, idx));

    case LUA_TSTRING:
      return getCurrentMotherboard()->makeString(lua_tostring(L, idx));

    case LUA_TUSERDATA:
      return fJboxValues.get(*reinterpret_cast<int *>(lua_touserdata(L, idx)));

    default:  /* other values */
      return getCurrentMotherboard()->makeNil();
  }
}

//------------------------------------------------------------------------
// RealtimeController::pushJBoxValue
//------------------------------------------------------------------------
void RealtimeController::pushJBoxValue(std::shared_ptr<const JboxValue> iJBoxValue)
{
  switch(iJBoxValue->getValueType())
  {
    case kJBox_Nil:
      lua_pushnil(L);
      break;

    case kJBox_Boolean:
      lua_pushboolean(L, iJBoxValue->getBoolean());
      break;

    case kJBox_Number:
      lua_pushnumber(L, iJBoxValue->getNumber());
      break;

    case kJBox_String:
    {
      lua_pushstring(L, iJBoxValue->getString().fValue.c_str());
      break;
    }

    default:
      auto jboxValueUserData = reinterpret_cast<int *>(lua_newuserdata(L, sizeof(int)));
      *jboxValueUserData = fJboxValues.add(std::move(iJBoxValue));
      break;
  }
}

}