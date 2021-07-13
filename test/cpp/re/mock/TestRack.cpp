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
#include <re/mock/MockDevices.h>
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

  auto src = rack.newExtension(MAuSrc::Config);
  auto &srcBuffer = src->getInstance<MAuSrc>()->fBuffer;

  ASSERT_TRUE(srcBuffer.check(0, 0));

  auto dst = rack.newExtension(MAuDst::Config);
  auto const &dstBuffer = dst->getInstance<MAuDst>()->fBuffer;

  ASSERT_TRUE(dstBuffer.check(0, 0));

  auto pst = rack.newExtension(MAuPst::Config);
  auto const &pstBuffer = pst->getInstance<MAuDst>()->fBuffer;

  ASSERT_TRUE(pstBuffer.check(0, 0));

  rack.wire(src->getAudioOutSocket(MAuSrc::LEFT_SOCKET), pst->getAudioInSocket(MAuPst::LEFT_SOCKET));
  rack.wire(pst->getAudioOutSocket(MAuPst::LEFT_SOCKET), dst->getAudioInSocket(MAuDst::LEFT_SOCKET));

  rack.wire(src->getAudioOutSocket(MAuSrc::RIGHT_SOCKET), pst->getAudioInSocket(MAuPst::RIGHT_SOCKET));
  rack.wire(pst->getAudioOutSocket(MAuPst::RIGHT_SOCKET), dst->getAudioInSocket(MAuDst::RIGHT_SOCKET));

  ASSERT_TRUE(srcBuffer.check(0, 0));
  ASSERT_TRUE(dstBuffer.check(0, 0));
  ASSERT_TRUE(pstBuffer.check(0, 0));

  rack.nextFrame();

  ASSERT_TRUE(srcBuffer.check(0, 0));
  ASSERT_TRUE(dstBuffer.check(0, 0));
  ASSERT_TRUE(pstBuffer.check(0, 0));

  srcBuffer.fill(2.0, 3.0);

  ASSERT_TRUE(srcBuffer.check(2.0, 3.0));
  ASSERT_TRUE(dstBuffer.check(0, 0));
  ASSERT_TRUE(pstBuffer.check(0, 0));

  rack.nextFrame();

  srcBuffer.fill(4.0, 5.0);

  ASSERT_TRUE(srcBuffer.check(4.0, 5.0));
  ASSERT_TRUE(dstBuffer.check(2.0, 3.0));
  ASSERT_TRUE(pstBuffer.check(2.0, 3.0));

  rack.nextFrame();

  ASSERT_TRUE(srcBuffer.check(4.0, 5.0));
  ASSERT_TRUE(dstBuffer.check(4.0, 5.0));
  ASSERT_TRUE(pstBuffer.check(4.0, 5.0));
}

}