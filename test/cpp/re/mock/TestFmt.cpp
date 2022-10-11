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
//TEST(Fmt, path)
//{
//  std::vector<std::string> v{"a1", "a2"};
//
//  auto sep = std::string(1, fmt::impl::pathSeparator);
//
//  ASSERT_EQ("root" + sep + "c1" + sep + "a1" + sep + "a2" + sep + "c2", fmt::path("root", "c1", v, "c2"));
//}

// Fmt.printf
TEST(Fmt, printf)
{
  ASSERT_EQ("12,[abc],[def],99.10", fmt::printf("%d,[%s],[%s],%.2f", 12, "abc", std::string("def"), 99.1));
}

// Fmt.url_decode
TEST(Fmt, url_decode)
{
  ASSERT_EQ("file:///tmp/Music/Samples/Sample 1.aif", fmt::url_decode("file:///tmp/%4Dusic/Sa%6dples/Sa%6Dple%201.aif"));
  ASSERT_EQ("file:///tmp/abcΩdef/klmΨn/samplemΨo.wav", fmt::url_decode("file://%2Ftmp%2Fabc%CE%A9def%2Fklm%CE%A8n%2Fsamplem%CE%A8o.wav"));
  ASSERT_EQ("file:///tmp/S\xFF" "a", fmt::url_decode("file:///tmp/S%4Za"));
}

}