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

#include <re/mock/Rack.h>
#include <re/mock/MockSrc.h>
#include <re/mock/MockDst.h>
#include <gtest/gtest.h>

namespace re::mock::Test {

using namespace mock;

// Rack.Basic
TEST(Rack, Basic)
{
  Rack rack{};

  auto re = rack.newExtension([](auto &mdef, auto &rtc, auto &rt) {
  });

  ASSERT_THROW(JBox_GetMotherboardObjectRef("/custom_properties"), Error);

  re->use([]() {
    // now this works
    JBox_GetMotherboardObjectRef("/custom_properties");
  });

}

TEST(Rack, Wiring) {
  Rack rack{};

  auto checkBuffer = [](TJBox_AudioSample iValue, MockSrc::buffer_type const &iBuffer) {
    for(auto v: iBuffer)
      ASSERT_FLOAT_EQ(v, iValue);
  };

  auto src = rack.newExtension(MockSrc::Config);
  auto &srcLeftBuffer = src->getInstance<MockSrc>()->fLeftBuffer;
  auto &srcRightBuffer = src->getInstance<MockSrc>()->fRightBuffer;
  
  checkBuffer(0, srcLeftBuffer);
  checkBuffer(0, srcRightBuffer);

  auto dst = rack.newExtension(MockDst::Config);
  auto const &dstLeftBuffer = dst->getInstance<MockDst>()->fLeftBuffer;
  auto const &dstRightBuffer = dst->getInstance<MockDst>()->fRightBuffer;

  checkBuffer(0, dstLeftBuffer);
  checkBuffer(0, dstRightBuffer);

  rack.wire(src->getAudioOutSocket(MockSrc::LEFT_SOCKET), dst->getAudioInSocket(MockDst::LEFT_SOCKET));
  rack.wire(src->getAudioOutSocket(MockSrc::RIGHT_SOCKET), dst->getAudioInSocket(MockSrc::RIGHT_SOCKET));

  checkBuffer(0, srcLeftBuffer);
  checkBuffer(0, srcRightBuffer);
  checkBuffer(0, dstLeftBuffer);
  checkBuffer(0, dstRightBuffer);

  rack.nextFrame();

  checkBuffer(0, srcLeftBuffer);
  checkBuffer(0, srcRightBuffer);
  checkBuffer(0, dstLeftBuffer);
  checkBuffer(0, dstRightBuffer);

  srcLeftBuffer.fill(2.0);
  srcRightBuffer.fill(3.0);

  checkBuffer(2.0, srcLeftBuffer);
  checkBuffer(3.0, srcRightBuffer);
  checkBuffer(0, dstLeftBuffer);
  checkBuffer(0, dstRightBuffer);

  rack.nextFrame();

  srcLeftBuffer.fill(4.0);
  srcRightBuffer.fill(5.0);

  checkBuffer(4.0, srcLeftBuffer);
  checkBuffer(5.0, srcRightBuffer);
  checkBuffer(0, dstLeftBuffer);
  checkBuffer(0, dstRightBuffer);

  rack.nextFrame();

  checkBuffer(4.0, srcLeftBuffer);
  checkBuffer(5.0, srcRightBuffer);
  checkBuffer(2.0, dstLeftBuffer);
  checkBuffer(3.0, dstRightBuffer);

  rack.nextFrame();

  checkBuffer(4.0, srcLeftBuffer);
  checkBuffer(5.0, srcRightBuffer);
  checkBuffer(4.0, dstLeftBuffer);
  checkBuffer(5.0, dstRightBuffer);
}

}