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

// Rack.AudioWiring
TEST(Rack, AudioWiring) {
  Rack rack{};

  auto src = rack.newExtension(MAUSrc::Config);
  auto &srcBuffer = src->getInstance<MAUSrc>()->fBuffer;

  ASSERT_TRUE(srcBuffer.check(0, 0));

  auto dst = rack.newExtension(MAUDst::Config);
  auto const &dstBuffer = dst->getInstance<MAUDst>()->fBuffer;

  ASSERT_TRUE(dstBuffer.check(0, 0));

  auto pst = rack.newExtension(MAUPst::Config);
  auto const &pstBuffer = pst->getInstance<MAUDst>()->fBuffer;

  ASSERT_TRUE(pstBuffer.check(0, 0));

  MockAudioDevice::wire(rack, src, pst);
  MockAudioDevice::wire(rack, pst, dst);

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

// Rack.CVWiring
TEST(Rack, CVWiring) {
  Rack rack{};

  auto src = rack.newExtension(MCVSrc::Config);
  auto &srcValue = src->getInstance<MCVSrc>()->fValue;

  ASSERT_FLOAT_EQ(0, srcValue);

  auto dst = rack.newExtension(MCVDst::Config);
  auto const &dstValue = dst->getInstance<MCVDst>()->fValue;

  ASSERT_FLOAT_EQ(0, dstValue);

  auto pst = rack.newExtension(MCVPst::Config);
  auto const &pstValue = pst->getInstance<MCVDst>()->fValue;

  ASSERT_FLOAT_EQ(0, pstValue);

  MockCVDevice::wire(rack, src, pst);
  MockCVDevice::wire(rack, pst, dst);

  ASSERT_FLOAT_EQ(0, srcValue);
  ASSERT_FLOAT_EQ(0, dstValue);
  ASSERT_FLOAT_EQ(0, pstValue);

  rack.nextFrame();

  ASSERT_FLOAT_EQ(0, srcValue);
  ASSERT_FLOAT_EQ(0, dstValue);
  ASSERT_FLOAT_EQ(0, pstValue);

  srcValue = 2.0;

  ASSERT_FLOAT_EQ(2.0, srcValue);
  ASSERT_FLOAT_EQ(0, dstValue);
  ASSERT_FLOAT_EQ(0, pstValue);

  rack.nextFrame();

  srcValue = 4.0;

  ASSERT_FLOAT_EQ(4.0, srcValue);
  ASSERT_FLOAT_EQ(2.0, dstValue);
  ASSERT_FLOAT_EQ(2.0, pstValue);

  rack.nextFrame();

  ASSERT_FLOAT_EQ(4.0, srcValue);
  ASSERT_FLOAT_EQ(4.0, dstValue);
  ASSERT_FLOAT_EQ(4.0, pstValue);
}

}