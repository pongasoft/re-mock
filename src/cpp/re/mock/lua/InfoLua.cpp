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

#include "InfoLua.h"

namespace re::mock::lua {

/*
 * -- Max 40 chars
long_name = "Simple Instrument"

-- Max 20 chars
medium_name = "Simple Instrument"

-- Max 10 chars
short_name = "SimpInstr"

product_id = "se.propellerheads.SimpleInstrument"
manufacturer = "Propellerhead Software"
version_number = "1.0.0d1"
device_type = "instrument"
device_categories = { "Misc" }
supports_patches = true
default_patch = "/Public/Plain Sinus.repatch"
accepts_notes = true
auto_create_track = true
auto_create_note_lane = true
supports_performance_automation = true
device_height_ru = 2
automation_highlight_color = {r = 60, g = 255, b = 2}

 */

//------------------------------------------------------------------------
// InfoLua::fromFile
//------------------------------------------------------------------------
std::unique_ptr<InfoLua> InfoLua::fromFile(fs::path const &iLuaFilename)
{
  auto res = std::make_unique<InfoLua>();
  res->loadFile(iLuaFilename);
  return res;
}

//------------------------------------------------------------------------
// InfoLua::fromFile
//------------------------------------------------------------------------
std::unique_ptr<InfoLua> InfoLua::fromString(std::string const &iLuaCode)
{
  auto res = std::make_unique<InfoLua>();
  res->loadString(iLuaCode);
  return res;
}

//------------------------------------------------------------------------
// InfoLua::long_name
//------------------------------------------------------------------------
std::string InfoLua::long_name()
{
  return L.getGlobalAsString("long_name");
}

//------------------------------------------------------------------------
// InfoLua::medium_name
//------------------------------------------------------------------------
std::string InfoLua::medium_name()
{
  return L.getGlobalAsString("medium_name");
}

//------------------------------------------------------------------------
// InfoLua::short_name
//------------------------------------------------------------------------
std::string InfoLua::short_name()
{
  return L.getGlobalAsString("short_name");
}

//------------------------------------------------------------------------
// InfoLua::product_id
//------------------------------------------------------------------------
std::string InfoLua::product_id()
{
  return L.getGlobalAsString("product_id");
}

//------------------------------------------------------------------------
// InfoLua::manufacturer
//------------------------------------------------------------------------
std::string InfoLua::manufacturer()
{
  return L.getGlobalAsString("manufacturer");
}

//------------------------------------------------------------------------
// InfoLua::version_number
//------------------------------------------------------------------------
std::string InfoLua::version_number()
{
  return L.getGlobalAsString("version_number");
}

//------------------------------------------------------------------------
// InfoLua::supports_patches
//------------------------------------------------------------------------
bool InfoLua::supports_patches()
{
  return L.getGlobalAsBoolean("supports_patches");
}

//------------------------------------------------------------------------
// InfoLua::default_patch
//------------------------------------------------------------------------
std::string InfoLua::default_patch()
{
  return L.getGlobalAsString("default_patch");
}

//------------------------------------------------------------------------
// InfoLua::device_type
//------------------------------------------------------------------------
std::string InfoLua::device_type()
{
  return L.getGlobalAsString("device_type");
}

//------------------------------------------------------------------------
// InfoLua::device_categories
//------------------------------------------------------------------------
std::vector<std::string> InfoLua::device_categories()
{
  std::vector<std::string> res{};
  if(lua_getglobal(L, "device_categories") != LUA_TNIL)
  {
    auto count = L.getTableSize();
    for(auto i = 1; i <= count; i++)
    {
      lua_geti(L, -1, i);
      auto s = lua_tostring(L, -1);
      if(s != nullptr)
        res.emplace_back(s);
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);
  return res;
}

//------------------------------------------------------------------------
// InfoLua::accepts_notes
//------------------------------------------------------------------------
bool InfoLua::accepts_notes()
{
  return L.getGlobalAsBoolean("accepts_notes");
}

//------------------------------------------------------------------------
// InfoLua::auto_create_track
//------------------------------------------------------------------------
bool InfoLua::auto_create_track()
{
  return L.getGlobalAsBoolean("auto_create_track");
}

//------------------------------------------------------------------------
// InfoLua::auto_create_note_lane
//------------------------------------------------------------------------
bool InfoLua::auto_create_note_lane()
{
  return L.getGlobalAsBoolean("auto_create_note_lane");
}

//------------------------------------------------------------------------
// InfoLua::supports_performance_automation
//------------------------------------------------------------------------
bool InfoLua::supports_performance_automation()
{
  return L.getGlobalAsBoolean("supports_performance_automation");
}

//------------------------------------------------------------------------
// InfoLua::device_height_ru
//------------------------------------------------------------------------
int InfoLua::device_height_ru()
{
  return static_cast<int>(L.getGlobalAsInteger("device_height_ru"));
}

}