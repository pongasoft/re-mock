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
#ifndef __PongasoftCommon_re_mock_mock_src_h__
#define __PongasoftCommon_re_mock_mock_src_h__

#include "Rack.h"
#include <array>

namespace re::mock {

class MockSrc
{
public:
  constexpr static auto LEFT_SOCKET = "L";
  constexpr static auto RIGHT_SOCKET = "R";

  using buffer_type = std::array<TJBox_AudioSample, 64>;

public:
  explicit MockSrc(int);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);
  void copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef &iToSocket);

  buffer_type fLeftBuffer{};
  buffer_type fRightBuffer{};

  static const Rack::Extension::Configuration Config;

private:
  TJBox_ObjectRef fLeftSocketRef;
  TJBox_ObjectRef fRightSocketRef;
};

}

#endif //__PongasoftCommon_re_mock_mock_src_h__
