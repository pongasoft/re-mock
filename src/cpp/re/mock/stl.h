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
#ifndef __Pongasoft_re_mock_stl_h__
#define __Pongasoft_re_mock_stl_h__

#include <algorithm>
#include <cmath>
#include <sstream>
#include <ostream>
#include <string>
#include <variant>

namespace re::mock::stl {

template<typename Container, typename Predicate>
inline constexpr bool all_of(Container const &iContainer, Predicate iPredicate)
{
  return std::all_of(std::begin(iContainer), std::end(iContainer), iPredicate);
}

template<typename Container, typename T>
inline constexpr bool all_item(Container const &iContainer, T const &iItem)
{
  return all_of(iContainer, [&iItem](auto &item) { return item == iItem; });
}

template<typename Container, typename T>
inline constexpr bool contains(Container const &iContainer, T const &iItem)
{
  return std::find(std::begin(iContainer), std::end(iContainer), iItem) != std::end(iContainer);
}

template<typename Container, typename Predicate>
inline constexpr bool contains_if(Container const &iContainer, Predicate iPredicate)
{
  return std::find_if(std::begin(iContainer), std::end(iContainer), iPredicate) != std::end(iContainer);
}

template<typename Container, typename Function>
inline constexpr void for_each(Container &iContainer, Function &&iFunction)
{
  std::for_each(std::begin(iContainer), std::end(iContainer), std::forward<Function>(iFunction));
}

template<typename Container, typename Function>
inline constexpr void for_each(Container const &iContainer, Function &&iFunction)
{
  std::for_each(std::begin(iContainer), std::end(iContainer), std::forward<Function>(iFunction));
}

template<typename T>
inline constexpr typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
almost_equal(T f1, T f2)
{
  if(std::isnan(f1) || std::isnan(f2))
    return false;

  return (std::fabs(f1 - f2) <= std::numeric_limits<T>::epsilon() * std::fmax(std::fabs(f1), std::fabs(f2)));
}

template<typename InputIterator, typename Function>
inline constexpr Function for_each_indexed(InputIterator iFirst, InputIterator iLast, Function iFunction)
{
  for(auto i = 0; iFirst != iLast; ++iFirst, ++i)
    iFunction(i, *iFirst);
  return iFunction;
}

template<typename Container, typename Function>
inline constexpr Function for_each_indexed(Container &iContainer, Function &&iFunction)
{
  return for_each_indexed(std::begin(iContainer), std::end(iContainer), std::forward<Function>(iFunction));
}

template<typename Container, typename Function>
inline constexpr Function for_each_indexed(Container const &iContainer, Function &&iFunction)
{
  return for_each_indexed(std::begin(iContainer), std::end(iContainer), std::forward<Function>(iFunction));
}

template<typename Container, typename Key>
inline constexpr bool contains_key(Container const &iContainer, Key const &iKey)
{
  return iContainer.find(iKey) != iContainer.end();
}

template<typename InputIterator, typename Stream, typename Separator = std::string>
inline Stream &join(InputIterator iFirst, InputIterator iLast, Stream &os, Separator iSeparator = {})
{
  for_each_indexed(iFirst, iLast, [&os, &iSeparator](auto i, auto &item) {
    if(i > 0)
      os << iSeparator;
    os << item;
  });
  return os;
}

template<typename Container, typename Stream, typename Separator = std::string>
inline Stream &join(Container const &iContainer, Stream &os, Separator iSeparator = {})
{
  return join(std::begin(iContainer), std::end(iContainer), os, iSeparator);
}

template<typename Container, typename Separator>
class Join
{
public:
  Join(Container const &iContainer, Separator iSeparator) : fContainer{iContainer}, fSeparator{iSeparator} {}
  friend std::ostream &operator<<(std::ostream &os, Join const &iJoin) { return join(iJoin.fContainer, os, iJoin.fSeparator); }

private:
  Container const &fContainer;
  Separator fSeparator;
};

template<typename Container>
inline std::string join_to_string(Container const &iContainer, std::string iSeparator = ", ")
{
  std::ostringstream os{};
  os << Join(iContainer, iSeparator);
  return os.str();
}

inline bool starts_with(std::string const &s, std::string const &iPrefix)
{
  if(s.size() < iPrefix.size())
    return false;
  return s.substr(0, iPrefix.size()) == iPrefix;
}

namespace impl {
template <class... Args>
struct variant_cast_proxy
{
  std::variant<Args...> v;

  template <class... ToArgs>
  constexpr operator std::variant<ToArgs...>() const
  {
    return std::visit([](auto&& arg) -> std::variant<ToArgs...> { return arg ; }, v);
  }
};
}

/**
 * Cast a variant into another one that is a a superset */
template <class... Args>
inline constexpr auto variant_cast(const std::variant<Args...>& v) -> stl::impl::variant_cast_proxy<Args...>
{
  return {v};
}

}

#endif //__Pongasoft_re_mock_stl_h__
