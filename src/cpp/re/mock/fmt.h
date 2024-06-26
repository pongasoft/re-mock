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
#include "fs.h"
#include "stl.h"
#if RE_MOCK_USE_STB_SPRINTF
#include <stb_sprintf.h>
#else
#include <stdio.h>
#endif

namespace re::mock::fmt {

namespace impl {

template<typename T>
constexpr auto printf_arg(T const &t) { return t; }

// Handles std::string without having to call c_str all the time
template<>
inline auto printf_arg<std::string>(std::string const &s) { return s.c_str(); }

// Handles std::string without having to call c_str all the time
template<>
[[deprecated("Since 1.6.0: Due to utf8, the right thing to do is s.u8string().str(), but cannot have a function return a pointer to a temporary string. It's ok to use s.u8string() as an argument.")]]
inline auto printf_arg<fs::path>(fs::path const &s) { return s.c_str(); }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif
/*
 * Copied from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf */
template<typename ... Args>
std::string printf(const std::string& format, Args ... args )
{
#if RE_MOCK_USE_STB_SPRINTF
  int size_s = stbsp_snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
#else
  int size_s = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
#endif
  if(size_s <= 0) { throw std::runtime_error("Error during formatting."); }
  auto size = static_cast<size_t>( size_s );
  auto buf = std::make_unique<char[]>(size);
#if RE_MOCK_USE_STB_SPRINTF
  stbsp_snprintf(buf.get(), size_s, format.c_str(), args ...);
#else
  snprintf(buf.get(), size_s, format.c_str(), args ...);
#endif
  return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

/**
 * Appends the content of the c-style, null terminated string, to `out`
 * @return the last value of `out` (for continue processing) */
template<typename OutputIterator>
constexpr OutputIterator to_chars(char const *iValue, OutputIterator out)
{
  auto input = iValue;
  char c{};

  while((c = *input++) != 0)
    *out++ = c;

  return out;
}

//------------------------------------------------------------------------
// printf using Jukebox format (^0, ... )
//------------------------------------------------------------------------
template<typename OutputIterator>
OutputIterator printf(char const *iFormat, std::vector<std::string> const &iParams, OutputIterator out)
{
  auto input = iFormat;
  char c;

  while((c = *input++) != 0)
  {
    if(c == '^')
    {
      auto n = *input++;

      // we reached the end
      if(n == 0)
      {
        *out++ = '^';
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
          out = to_chars(iParams[n - '0'].c_str(), out);
          break;

        default:
          *out++ = c;
          break;
      }
    }
    else
    {
      *out++ = c;
    }
  }
  
  return out;
}

//------------------------------------------------------------------------
// ASCIIHexToInt
//------------------------------------------------------------------------
static int ASCIIHexToInt[] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

//! utility to convert ascii char to hex
constexpr int ascii_hex_to_int(char c)
{
  if(c < 0 || c > 127)
    return -1;
  return ASCIIHexToInt[static_cast<unsigned char>(c)];
}

constexpr char INVALID_SYNTAX_CHAR = 0xff;

//------------------------------------------------------------------------
// url_decode
//------------------------------------------------------------------------
template<typename OutputIterator>
constexpr OutputIterator url_decode(char const *u, OutputIterator out)
{
  if(u == nullptr)
    return out;

  auto input = u;
  char c{};

  while((c = *input++) != 0)
  {
    if(c == '+')
    {
      *out++ = ' ';
    } else if(c == '%')
    {
      auto n1 = *input++;

      // we reached the end
      if(n1 == 0)
      {
        *out++ = '%';
        break; // out of while
      }

      auto n2 = *input++;
      // we reached the end
      if(n2 == 0)
      {
        *out++ = '%';
        *out++ = n1;
        break; // out of while
      }

      auto h1 = ascii_hex_to_int(n1);
      auto h2 = ascii_hex_to_int(n2);

      if(h1 == -1 || h2 == -1)
        *out++ = INVALID_SYNTAX_CHAR;
      else
        *out++ = h1 * 16 + h2;
    }
    else
    {
      *out++ = c;
    }
  }

  return out;
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
// Trim a string (removes whitespace from front and back)
//------------------------------------------------------------------------
std::string trim(std::string const &s);

//------------------------------------------------------------------------
// Split a string
//------------------------------------------------------------------------
std::vector<std::string> split(const std::string &s, char delimiter, bool includeEmptyTokens = false);

//------------------------------------------------------------------------
// decodes the url provided (%xx => value)
//------------------------------------------------------------------------
std::string url_decode(const std::string &u);
}

#endif //__Pongasoft_re_mock_fmt_h__
