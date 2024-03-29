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
  std::cout << fmt::printf("INFO | %s:%d | %s", iFile, iLine, fmt::printf(format, std::forward<Args>(args)...)) << std::endl;
}

template<typename ... Args>
void log_warning(char const *iFile, int iLine, const std::string& format, Args ... args)
{
  std::cerr << fmt::printf("WARN | %s:%d | %s", iFile, iLine, fmt::printf(format, std::forward<Args>(args)...)) << std::endl;
}

template<typename ... Args>
void log_jukebox_info(const std::string& format, Args ... args)
{
  std::cout << fmt::printf(format, std::forward<Args>(args)...) << std::endl;
}

template<typename ... Args>
void log_error(char const *iFile, int iLine, const std::string& format, Args ... args)
{
  std::cerr << fmt::printf("ERR  | %s:%d | %s", iFile, iLine, fmt::printf(format, std::forward<Args>(args)...)) << std::endl;
}

template<typename ... Args>
void log_jukebox_error(const std::string& format, Args ... args)
{
  std::cerr << fmt::printf(format, std::forward<Args>(args)...) << std::endl;
}

/**
 * When `re-mock` detects an error, it usually throws an exception of this type via the `RE_MOCK_ASSERT` define */
struct Exception : public std::logic_error {
  explicit Exception(std::string const &s) : std::logic_error(s.c_str()) {}
  explicit Exception(char const *s) : std::logic_error(s) {}

  [[ noreturn ]] static void throwException(char const *iMessage, char const *iFile, int iLine)
  {
    throw Exception(fmt::printf("%s:%d | %s", iFile, iLine, iMessage));
  }

  template<typename ... Args>
  [[ noreturn ]] static void throwException(char const *iMessage, char const *iFile, int iLine, const std::string& format, Args ... args)
  {
    throw Exception(fmt::printf(" %s:%d | %s | %s", iFile, iLine, iMessage, fmt::printf(format, std::forward<Args>(args)...)));
  }
};

#define RE_MOCK_ASSERT(test, ...) (test) == true ? (void)0 : re::mock::Exception::throwException("CHECK FAILED: [" #test "]", __FILE__, __LINE__, ##__VA_ARGS__)
#define RE_MOCK_FAIL(...) re::mock::Exception::throwException("FAIL", __FILE__, __LINE__, ##__VA_ARGS__)
#define RE_MOCK_TBD RE_MOCK_FAIL("Not implemented yet")
#if ENABLE_RE_MOCK_INTERNAL_ASSERT
#define RE_MOCK_INTERNAL_ASSERT(test, ...) (test) == true ? (void)0 : re::mock::Exception::throwException("INTERNAL CHECK FAILED: [" #test "]", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define RE_MOCK_INTERNAL_ASSERT(test, ...)
#endif

#define RE_MOCK_LOG_INFO(...) re::mock::log_info(__FILE__, __LINE__, __VA_ARGS__)
#define RE_MOCK_LOG_WARNING(...) re::mock::log_warning(__FILE__, __LINE__, __VA_ARGS__)
#define RE_MOCK_LOG_ERROR(...) re::mock::log_error(__FILE__, __LINE__, __VA_ARGS__)
#define RE_MOCK_LOG_JUKEBOX_INFO(...) re::mock::log_jukebox_info(__VA_ARGS__)
#define RE_MOCK_LOG_JUKEBOX_ERROR(...) re::mock::log_jukebox_error(__VA_ARGS__)

}

#endif //__Pongasoft_re_mock_errors_h__
