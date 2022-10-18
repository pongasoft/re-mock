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

#include <re/mock/lua/LuaState.h>
#include <gtest/gtest.h>

namespace re::mock::lua::Test {

// LuaState.Basic
TEST(LuaState, Basic)
{
  LuaState lua{};

  ASSERT_EQ(lua.getStackString(), "<empty>");
  ASSERT_EQ(lua.getStackString("test"), "test: <empty>");
}

// LuaStackInfo.Snippet
TEST(LuaStackInfo, Snippet)
{
  auto source = R"(line1
line2
line3
line4
line5
line6
line7)";

  ASSERT_EQ(LuaStackInfo::computeSnippetFromString("", 3), "");
  ASSERT_EQ(LuaStackInfo::computeSnippetFromString(source, 3), "->[3]\tline3\n");
  ASSERT_EQ(LuaStackInfo::computeSnippetFromString(source, 3, 1), "  [2]\tline2\n->[3]\tline3\n  [4]\tline4\n");
  ASSERT_EQ(LuaStackInfo::computeSnippetFromString(source, 2, 2), "  [1]\tline1\n->[2]\tline2\n  [3]\tline3\n  [4]\tline4\n");
  ASSERT_EQ(LuaStackInfo::computeSnippetFromString(source, 7, 1), "  [6]\tline6\n->[7]\tline7\n");
}

}
