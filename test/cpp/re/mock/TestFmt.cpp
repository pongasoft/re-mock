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

#include <re/mock/fmt.h>
#include <gtest/gtest.h>

namespace re::mock::Test {

using namespace mock;

// Fmt.path
TEST(Fmt, path)
{
  std::vector<std::string> v{"a1", "a2"};

  auto sep = std::string(1, fmt::impl::pathSeparator);

  ASSERT_EQ("root" + sep + "c1" + sep + "a1" + sep + "a2" + sep + "c2", fmt::path("root", "c1", v, "c2"));

}

}