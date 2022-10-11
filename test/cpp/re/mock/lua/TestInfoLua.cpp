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

  ASSERT_EQ(def->device_type(), "instrument");
  ASSERT_TRUE(def->supports_patches());
  ASSERT_EQ(def->default_patch(), "/Public/Plain Sinus.repatch");

  ASSERT_EQ(def->getStackString(), "<empty>");
}

}
