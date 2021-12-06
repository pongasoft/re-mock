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

#include "fmt.h"
#include <locale>
#include <sstream>

namespace re::mock::fmt {

//------------------------------------------------------------------------
// fmt::trim
//------------------------------------------------------------------------
std::string trim(std::string const &s)
{
  auto str = s;
  auto it1 = std::find_if(str.rbegin(), str.rend(),
                          [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
  str.erase(it1.base(), str.end());

  auto it2 = std::find_if(str.begin(), str.end(),
                          [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
  str.erase(str.begin(), it2);
  return str;
}

//------------------------------------------------------------------------
// fmt::split
//------------------------------------------------------------------------
std::vector<std::string> split(std::string const &s, char delimiter, bool includeEmptyTokens)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while(std::getline(tokenStream, token, delimiter))
  {
    if(!token.empty() || includeEmptyTokens)
      tokens.push_back(token);
  }
  return tokens;
}

//------------------------------------------------------------------------
// fmt::url_decode
//------------------------------------------------------------------------
std::string url_decode(std::string const &u)
{
  std::string o{};
  impl::url_decode(u.c_str(), std::back_inserter(o));
  return o;
}

}