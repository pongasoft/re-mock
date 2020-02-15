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

#ifndef __PongasoftCommon_fmt_h__
#define __PongasoftCommon_fmt_h__

#include <algorithm>
#include <array>

namespace fmt {

//------------------------------------------------------------------------
// implementation detail (necessary here due to templates)
//------------------------------------------------------------------------
namespace impl {

/**
 * If there is something wrong in `printf`, this character will be output instead of the one(s) intended */
constexpr char INVALID_SYNTAX_CHAR = 0xff;

/**
 * Appends the content of the c-style, null terminated string, to `first` (but not beyond `last`).
 *
 * @return the last value of `first` (for continue processing) */
template<typename Iter>
constexpr Iter to_chars(char iDynamicCheck, char const *iValue, Iter first, Iter last) noexcept
{
  // check for %s
  if(iDynamicCheck != 's')
  {
    if(first != last)
      *first++ = INVALID_SYNTAX_CHAR;
    return first;
  }

  auto input = iValue;
  char c{};

  while(first != last && (c = *input++) != 0)
  {
    *first++ = c;
  }

  return first;
}

/**
 * Appends the int (converted to a string) to `first` (but not beyond `last`).
 *
 * @return the last value of `first` (for continue processing) */
template<typename Iter>
constexpr Iter to_chars(char iDynamicCheck, int iValue, Iter first, Iter last) noexcept
{
  // check for %d
  if(iDynamicCheck != 'd')
  {
    if(first != last)
      *first++ = INVALID_SYNTAX_CHAR;
    return first;
  }

  char buf[32]{};

  bool negative = iValue < 0;
  if(negative)
    iValue = -iValue;

  int i = 0;
  // generate digits in reverse order
  do
  {
    buf[i++] = static_cast<char>(iValue % 10) + '0';
  } while((iValue /= 10) > 0);

  if(negative)
    buf[i++] = '-';

  // array is initialized with 0 so no need to add one at the end...

  std::reverse(buf, buf + i);

  return to_chars('s', buf, first, last);
}

/**
 * This call represents the "leaf" call when there is no more arguments. Note that this implementation
 * cannot simply append `iFormat` to `first` because it needs to take into consideration the fact that
 * there is the replacement sequence (ex: with an original format of "abc %s def %%", then the leaf
 * call will be " def %%" => we need to replace `%%` with `%`)
 */
template<typename Iter>
constexpr Iter printf(Iter first, Iter last, char const *iFormat) noexcept
{
  auto input = iFormat;
  char c{};

  while(first != last && (c = *input++) != 0)
  {
    if(c == '%')
    {
      auto n = *input++;

      // we reached the end
      if(n == 0)
      {
        *first++ = '%';
        break; // out of while
      }

      *first++ = (n == '%' ? '%' : INVALID_SYNTAX_CHAR);
    }
    else
    {
      *first++ = c;
    }
  }

  return first;
}

/**
 * This is the recursive call which "extracts" the first element from `Args&&...` and processes it. Once
 * processed, it recursively calls itself. It ends when there is no more arguments at which point
 * the leaf call (defined above) is invoked instead */
template<typename Iter, typename T, typename... Args>
constexpr Iter printf(Iter first, Iter last, char const *iFormat, T iValue, Args&&... args) noexcept
{
  auto input = iFormat;
  char c{};

  while(first != last && (c = *input++) != 0)
  {
    if(c == '%')
    {
      auto n = *input++;

      // we reached the end
      if(n == 0)
      {
        *first++ = '%';
        break; // out of while
      }

      switch(n)
      {
        case '%':
          *first++ = '%';
          break;

        case 's':
        case 'd':
          return printf(to_chars(n, iValue, first, last), last, input, std::forward<Args>(args)...);

        default:
          *first++ = INVALID_SYNTAX_CHAR;
          break;
      }
    }
    else
    {
      *first++ = c;
    }
  }

  return first;
}

}

//------------------------------------------------------------------------
// End implementation detail
//------------------------------------------------------------------------

/**
 * This is the main API for formatting. It is a simplified version of `snprintf` using modern C++
 * techniques (variable template arguments).
 *
 * The syntax for `iFormat` is a (c-style null terminated) string which can contain the replacement tokens
 *
 * - `%d` for an integer
 * - `%s` for a string (`char const *`)
 * - `%%` for the `%` character
 *
 * `Iter` can be any iterator supporting `++`, `--` and `*`
 *
 * Example:
 * ```
 *  char buf[100];
 *  fmt::printf(std::begin(buf), std::end(buf), "this is %s (%d) test", "one", 1);
 *  // produces => "this is one (1) test"
 * ```
 *
 * @note Any invalid token or mismatch type will be replaced by a single invalid character
 *
 * At this time, `Args` can be `int`, `char const *` (aka c-style null terminated string), or `StaticString`.
 *
 * @return one past the last position where `first` was written to */
template<typename Iter, typename... Args>
constexpr Iter printf(Iter first, Iter last, char const *iFormat, Args&&... args) noexcept
{
  first = impl::printf(first, last, iFormat, std::forward<Args>(args)...);

  // must be null terminated...
  if(first == last)
    first--;
  *first++ = '\0';

  return first;
}

}

#endif //__PongasoftCommon_fmt_h__
