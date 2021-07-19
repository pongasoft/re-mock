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
#ifndef __PongasoftCommon_re_mock_fmt_h__
#define __PongasoftCommon_re_mock_fmt_h__

#include <string>

namespace re::mock::fmt {

namespace impl {

template<typename T>
constexpr auto printf_arg(T const &t) { return t; }

// Handles std::string without having to call c_str all the time
template<>
constexpr auto printf_arg<std::string>(std::string const &s) { return s.c_str(); }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
/*
 * Copied from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf */
template<typename ... Args>
std::string printf(const std::string& format, Args ... args )
{
  int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
  if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
  auto size = static_cast<size_t>( size_s );
  auto buf = std::make_unique<char[]>( size );
  std::snprintf( buf.get(), size, format.c_str(), args ... );
  return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}
#pragma clang diagnostic pop

}

template<typename ... Args>
std::string printf(const std::string& format, Args ... args )
{
  return impl::printf(format, impl::printf_arg(args)...);
}

}

#endif //__PongasoftCommon_re_mock_fmt_h__
