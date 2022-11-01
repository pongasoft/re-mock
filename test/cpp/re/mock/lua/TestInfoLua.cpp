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

#include <re/mock/lua/InfoLua.h>
#include <re/mock/Errors.h>
#include <gtest/gtest.h>

namespace re::mock::lua::Test {

using namespace testing;

fs::path getResourceFile(std::string iFilename);

// InfoLua.Empty
TEST(InfoLua, Empty)
{
  auto def = InfoLua::fromString("print('hello from lua')");

  ASSERT_EQ(def->getStackString(), "<empty>");

  ASSERT_EQ(def->device_type(), "");
  ASSERT_FALSE(def->supports_patches());
  ASSERT_EQ(def->default_patch(), "");


  ASSERT_EQ(def->getStackString(), "<empty>");
}

// InfoLua.Basic
TEST(InfoLua, Basic)
{
  // file from SDK Simple Instrument
  auto def = InfoLua::fromFile(getResourceFile("simple_instrument-info.lua"));

  ASSERT_EQ(def->getStackString(), "<empty>");

  ASSERT_EQ(def->long_name(), "Simple Instrument (long)");
  ASSERT_EQ(def->medium_name(), "Simple Instrument");
  ASSERT_EQ(def->short_name(), "SimpInstr");
  ASSERT_EQ(def->product_id(), "se.propellerheads.SimpleInstrument");
  ASSERT_EQ(def->manufacturer(), "Propellerhead Software");
  ASSERT_EQ(def->version_number(), "1.0.0d1");
  ASSERT_EQ(def->device_type(), "instrument");
  ASSERT_TRUE(def->supports_patches());
  ASSERT_EQ(def->default_patch(), "/Public/Plain Sinus.repatch");
  ASSERT_TRUE(def->accepts_notes());
  ASSERT_TRUE(def->auto_create_track());
  ASSERT_TRUE(def->auto_create_note_lane());
  ASSERT_TRUE(def->supports_performance_automation());
  ASSERT_EQ(def->device_height_ru(), 2);

  ASSERT_EQ(def->getStackString(), "<empty>");
}

// InfoLua.Invalid
TEST(InfoLua, Invalid)
{
  try
  {
    InfoLua::fromFile(getResourceFile("invalid1.lua"));
    FAIL(); // should not be reached
  }
  catch(Exception &e)
  {
    RE_MOCK_LOG_INFO("invalid1.lua\n%s", e.what());
  }

  try
  {
    InfoLua::fromFile(getResourceFile("invalid2.lua"));
    FAIL(); // should not be reached
  }
  catch(Exception &e)
  {
    RE_MOCK_LOG_INFO("invalid.lua\n%s", e.what());
  }

  try
  {
    InfoLua::fromFile(getResourceFile("not_exists.lua"));
    FAIL(); // should not be reached
  }
  catch(Exception &e)
  {
    RE_MOCK_LOG_INFO("not_exists.lua\n%s", e.what());
  }

}

}
