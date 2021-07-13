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

#pragma once
#ifndef __PongasoftCommon_re_mock_stl_h__
#define __PongasoftCommon_re_mock_stl_h__

#include <algorithm>

namespace re::mock::stl {

template<typename Container, typename Predicate>
inline bool all_of(Container const &iContainer, Predicate iPredicate)
{
  return std::all_of(std::begin(iContainer), std::end(iContainer), iPredicate);
}

template<typename Container, typename T>
inline bool all_item(Container const &iContainer, T const &iItem)
{
  return all_of(iContainer, [&iItem](auto &item) {return item == iItem; });
}

template<typename Container, typename T>
inline bool contains(Container const &iContainer, T const &iItem)
{
  return std::find(std::begin(iContainer), std::end(iContainer), iItem) != std::end(iContainer);
}

template<typename Container, typename Predicate>
inline bool find_if(Container const &iContainer, Predicate iPredicate)
{
  return std::find_if(std::begin(iContainer), std::end(iContainer), iPredicate) != std::end(iContainer);
}

}

#endif //__PongasoftCommon_re_mock_stl_h__
