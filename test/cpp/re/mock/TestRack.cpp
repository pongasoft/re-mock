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

// Rack.CircularWiring
TEST(Rack, CircularWiring)
{
  // Setup: we create a circular dependency
  // MAUSrcCVDst emits audio and receives CV
  // MAUDstCVSrc receives audio and emits CV
  // MAUSrcCVDst -> MAUDstCVSrc for audio
  // MAUDstCVSrc -> MAUSrcCVDst for CV
  struct MAUSrcCVDst : public MAUSrc, MCVDst
  {
    MAUSrcCVDst(int iSampleRate) : MAUSrc(iSampleRate), MCVDst(iSampleRate){}
    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount)
    {
      MAUSrc::renderBatch(iPropertyDiffs, iDiffCount);
      MCVDst::renderBatch(iPropertyDiffs, iDiffCount);
    }
  };

  const Rack::Extension::Configuration MAUSrcCVDstConfig = [](auto &def, auto &rtc, auto &rt) {
    // MAUSrcCVDst emits audio
    def.audio_outputs[MAUSrcCVDst::LEFT_SOCKET] = jbox.audio_output();
    def.audio_outputs[MAUSrcCVDst::RIGHT_SOCKET] = jbox.audio_output();

    // MAUSrcCVDst receives CV
    def.cv_inputs[MAUSrcCVDst::SOCKET] = jbox.cv_input();

    // use default bindings
    rtc = RealtimeController::byDefault();

    // rt
    rt = Realtime::byDefault<MAUSrcCVDst>();
  };

  struct MAUDstCVSrc : public MAUDst, MCVSrc
  {
    MAUDstCVSrc(int iSampleRate) : MAUDst(iSampleRate), MCVSrc(iSampleRate){}
    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount)
    {
      MAUDst::renderBatch(iPropertyDiffs, iDiffCount);
      MCVSrc::renderBatch(iPropertyDiffs, iDiffCount);
    }
  };

  static const Rack::Extension::Configuration MAUDstCVSrcConfig = [](auto &def, auto &rtc, auto &rt) {
    // MAUDstCVSrc receives audio
    def.audio_inputs[MAUDstCVSrc::LEFT_SOCKET] = jbox.audio_input();
    def.audio_inputs[MAUDstCVSrc::RIGHT_SOCKET] = jbox.audio_input();

    // MAUDstCVSrc emits CV
    def.cv_outputs[MAUDstCVSrc::SOCKET] = jbox.cv_output();

    // use default bindings
    rtc = RealtimeController::byDefault();

    // rt
    rt = Realtime::byDefault<MAUDstCVSrc>();
  };


  Rack rack{};

  auto ex1 = rack.newExtension(MAUSrcCVDstConfig);
  auto ex2 = rack.newExtension(MAUDstCVSrcConfig);

  // we do not connect the right socket on purpose (no need to create the circular dependency)
  rack.wire(ex1->getAudioOutSocket(MAUSrcCVDst::LEFT_SOCKET), ex2->getAudioInSocket(MAUDstCVSrc::LEFT_SOCKET));
  rack.wire(ex2->getCVOutSocket(MAUDstCVSrc::SOCKET), ex1->getCVInSocket(MAUSrcCVDst::SOCKET));

  // we make sure that this does not introduce an infinite loop
  rack.nextFrame();

  auto &ex1Buffer = ex1->getInstance<MAUSrcCVDst>()->fBuffer;
  auto &ex1Value = ex1->getInstance<MAUSrcCVDst>()->fValue;

  auto &ex2Buffer = ex2->getInstance<MAUDstCVSrc>()->fBuffer;
  auto &ex2Value = ex2->getInstance<MAUDstCVSrc>()->fValue;

  ex1Buffer.fill(2.0, 0);
  ex2Value = 3.0;

  rack.nextFrame();

  // due to circular dependency, and the fact that devices are processed in order of id, ex2 out will be processed
  // first so the CV value will make in this frame while the audio buffer will make it in the next
  ASSERT_TRUE(ex2Buffer.check(0.0, 0));
  ASSERT_FLOAT_EQ(3.0, ex1Value);

  rack.nextFrame();

  // now the audio buffer has caught up
  ASSERT_TRUE(ex2Buffer.check(2.0, 0));
  ASSERT_FLOAT_EQ(3.0, ex1Value);
}

// Rack.SelfConnection
TEST(Rack, SelfConnection)
{
  struct SelfConnectedDevice
  {
    SelfConnectedDevice(int /* iSampleRate */) :
      fInput{JBox_GetMotherboardObjectRef("/cv_inputs/I")},
      fOutput{JBox_GetMotherboardObjectRef("/cv_outputs/O")}
    {}

    void renderBatch(const TJBox_PropertyDiff [], TJBox_UInt32)
    {
      TJBox_Float64 inputValue{};
      MockCVDevice::loadValue(fInput, inputValue);
      fValue = inputValue + 1.0;
      MockCVDevice::storeValue(fValue, fOutput);
    }

    TJBox_Float64 fValue{};
    TJBox_ObjectRef fInput{};
    TJBox_ObjectRef fOutput{};
  };

  const Rack::Extension::Configuration SelfConnectedDeviceConfig = [](auto &def, auto &rtc, auto &rt) {
    def.cv_inputs["I"] = jbox.cv_input();
    def.cv_outputs["O"] = jbox.cv_output();

    // use default bindings
    rtc = RealtimeController::byDefault();

    // rt
    rt = Realtime::byDefault<SelfConnectedDevice>();
  };

  Rack rack{};

  auto dev = rack.newExtension(SelfConnectedDeviceConfig);
  auto &value = dev->getInstance<SelfConnectedDevice>()->fValue;

  rack.wire(dev->getCVOutSocket("O"), dev->getCVInSocket("I"));

  rack.nextFrame();

  ASSERT_FLOAT_EQ(1.0, value);

  rack.nextFrame();

  ASSERT_FLOAT_EQ(2.0, value);

  rack.nextFrame();

  ASSERT_FLOAT_EQ(3.0, value);
}

}