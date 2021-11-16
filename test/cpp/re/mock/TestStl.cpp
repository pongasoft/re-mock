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

#include <re/mock/stl.h>
#include <gtest/gtest.h>

namespace re::mock::Test {

using namespace mock;

// Stl.join
TEST(Stl, join)
{
  {
    std::vector<std::string> v{};
    ASSERT_EQ("", stl::join_to_string(v, "/"));
    ASSERT_EQ("", stl::join_to_string(v));
  }

  {
    std::vector<std::string> v{"a1"};
    ASSERT_EQ("a1", stl::join_to_string(v, "/"));
    ASSERT_EQ("a1", stl::join_to_string(v));
  }

  {
    std::vector<std::string> v{"a1", "a2"};
    ASSERT_EQ("a1/a2", stl::join_to_string(v, "/"));
    ASSERT_EQ("a1, a2", stl::join_to_string(v));
  }

  {
    std::vector<std::string> v{"a1", "a2", "a3"};
    ASSERT_EQ("a1/a2/a3", stl::join_to_string(v, "/"));
    ASSERT_EQ("a1, a2, a3", stl::join_to_string(v));
  }
}

}