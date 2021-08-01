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
#ifndef __Pongasoft_re_mock_fmt_h__
#define __Pongasoft_re_mock_fmt_h__

#include <string>
#include <vector>
#include <stdexcept>

namespace re::mock::fmt {

namespace impl {

template<typename T>
constexpr auto printf_arg(T const &t) { return t; }

// Handles std::string without having to call c_str all the time
template<>
inline auto printf_arg<std::string>(std::string const &s) { return s.c_str(); }

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

/**
 * Appends the content of the c-style, null terminated string, to `first` (but not beyond `last`).
 * @return the last value of `first` (for continue processing) */
template<typename Iter>
Iter to_chars(char const *iValue, Iter first, Iter last)
{
  auto input = iValue;
  char c;

  while(first != last && (c = *input++) != 0)
    *first++ = c;

  return first;
}

//------------------------------------------------------------------------
// printf using Jukebox format (^0, ... )
//------------------------------------------------------------------------
template<typename Iter>
Iter printf(Iter first, Iter last, char const *iFormat, std::vector<std::string> const &iParams)
{
  auto input = iFormat;
  char c;

  while(first != last && (c = *input++) != 0)
  {
    if(c == '^')
    {
      auto n = *input++;

      // we reached the end
      if(n == 0)
      {
        *first++ = '^';
        break; // out of while
      }

      switch(n)
      {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          first = to_chars(iParams[n - '0'].c_str(), first, last);
          break;

        default:
          *first++ = c;
          break;
      }
    }
    else
    {
      *first++ = c;
    }
  }
  
  // must be null terminated
  if(first == last)
    first--;
  *first++ = '\0';

  return first;
}

}

//------------------------------------------------------------------------
// printf -> std::string
//------------------------------------------------------------------------
template<typename ... Args>
inline std::string printf(const std::string& format, Args ... args)
{
  return impl::printf(format, impl::printf_arg(args)...);
}

//------------------------------------------------------------------------
// path
//------------------------------------------------------------------------
inline std::string path(std::string const &path)
{
  return path;
}

//------------------------------------------------------------------------
// path
//------------------------------------------------------------------------
template<typename ... Args>
inline std::string path(std::string const &dir, std::string const &child, Args ... children)
{
#ifdef _WIN32
  constexpr char pathSeparator = '\\';
#else
  constexpr char pathSeparator = '/';
#endif
  return path(dir + pathSeparator + child, std::forward<Args>(children)...);
}

}

#endif //__Pongasoft_re_mock_fmt_h__
