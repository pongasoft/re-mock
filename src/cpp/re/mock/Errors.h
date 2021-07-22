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
#ifndef __Pongasoft_re_mock_errors_h__
#define __Pongasoft_re_mock_errors_h__

#include <stdexcept>
#include <string>
#include <iostream>
#include "fmt.h"

namespace re::mock {

template<typename ... Args>
void log_info(char const *iFile, int iLine, const std::string& format, Args ... args)
{
  std::cout << fmt::printf("%s:%d | %s", iFile, iLine, fmt::printf(format, std::forward<Args>(args)...)) << std::endl;
}

template<typename ... Args>
void log_error(char const *iFile, int iLine, const std::string& format, Args ... args)
{
  std::cerr << fmt::printf("%s:%d | %s", iFile, iLine, fmt::printf(format, std::forward<Args>(args)...)) << std::endl;
}

// Error handling
struct Exception : public std::logic_error {
  explicit Exception(std::string const &s) : std::logic_error(s.c_str()) {}
  explicit Exception(char const *s) : std::logic_error(s) {}

  static void throwException(char const *iMessage, char const *iFile, int iLine)
  {
    throw Exception(fmt::printf("%s | %s:%d", iMessage, iFile, iLine));
  }

  template<typename ... Args>
  static void throwException(char const *iMessage, char const *iFile, int iLine, const std::string& format, Args ... args)
  {
    throw Exception(fmt::printf(" %s:%d | %s | %s", iFile, iLine, iMessage, fmt::printf(format, std::forward<Args>(args)...)));
  }
};

#define CHECK_F(test, ...) (test) == true ? (void)0 : re::mock::Exception::throwException("CHECK FAILED: \"" #test "\"", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_INFO(...) re::mock::log_info(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) re::mock::log_error(__FILE__, __LINE__, __VA_ARGS__)

}

#endif //__Pongasoft_re_mock_errors_h__
