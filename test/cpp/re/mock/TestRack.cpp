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

  auto re = rack.newExtension(Config{});

  ASSERT_THROW(JBox_GetMotherboardObjectRef("/custom_properties"), Error);

  re.use([]() {
    // now this works
    JBox_GetMotherboardObjectRef("/custom_properties");
  });

}

// Rack.AudioWiring
TEST(Rack, AudioWiring) {
  Rack rack{};

  auto src = rack.newDevice<MAUSrc>(MAUSrc::Config);

  ASSERT_TRUE(src->fBuffer.check(0, 0));

  auto dst = rack.newDevice<MAUDst>(MAUDst::Config);

  ASSERT_TRUE(dst->fBuffer.check(0, 0));

  auto pst = rack.newDevice<MAUPst>(MAUPst::Config);

  ASSERT_TRUE(pst->fBuffer.check(0, 0));

  MockAudioDevice::wire(rack, src, pst);
  MockAudioDevice::wire(rack, pst, dst);

  ASSERT_TRUE(src->fBuffer.check(0, 0));
  ASSERT_TRUE(dst->fBuffer.check(0, 0));
  ASSERT_TRUE(pst->fBuffer.check(0, 0));

  rack.nextFrame();

  ASSERT_TRUE(src->fBuffer.check(0, 0));
  ASSERT_TRUE(dst->fBuffer.check(0, 0));
  ASSERT_TRUE(pst->fBuffer.check(0, 0));

  src->fBuffer.fill(2.0, 3.0);

  ASSERT_TRUE(src->fBuffer.check(2.0, 3.0));
  ASSERT_TRUE(dst->fBuffer.check(0, 0));
  ASSERT_TRUE(pst->fBuffer.check(0, 0));

  rack.nextFrame();

  src->fBuffer.fill(4.0, 5.0);

  ASSERT_TRUE(src->fBuffer.check(4.0, 5.0));
  ASSERT_TRUE(dst->fBuffer.check(2.0, 3.0));
  ASSERT_TRUE(pst->fBuffer.check(2.0, 3.0));

  rack.nextFrame();

  ASSERT_TRUE(src->fBuffer.check(4.0, 5.0));
  ASSERT_TRUE(dst->fBuffer.check(4.0, 5.0));
  ASSERT_TRUE(pst->fBuffer.check(4.0, 5.0));
}

// Rack.CVWiring
TEST(Rack, CVWiring) {
  Rack rack{};

  auto src = rack.newDevice<MCVSrc>(MCVSrc::Config);

  ASSERT_FLOAT_EQ(0, src->fValue);

  auto dst = rack.newDevice<MCVDst>(MCVDst::Config);

  ASSERT_FLOAT_EQ(0, dst->fValue);

  auto pst = rack.newDevice<MCVPst>(MCVPst::Config);

  ASSERT_FLOAT_EQ(0, pst->fValue);

  MockCVDevice::wire(rack, src, pst);
  MockCVDevice::wire(rack, pst, dst);

  ASSERT_FLOAT_EQ(0, src->fValue);
  ASSERT_FLOAT_EQ(0, dst->fValue);
  ASSERT_FLOAT_EQ(0, pst->fValue);

  rack.nextFrame();

  ASSERT_FLOAT_EQ(0, src->fValue);
  ASSERT_FLOAT_EQ(0, dst->fValue);
  ASSERT_FLOAT_EQ(0, pst->fValue);

  src->fValue = 2.0;

  ASSERT_FLOAT_EQ(2.0, src->fValue);
  ASSERT_FLOAT_EQ(0, dst->fValue);
  ASSERT_FLOAT_EQ(0, pst->fValue);

  rack.nextFrame();

  src->fValue = 4.0;

  ASSERT_FLOAT_EQ(4.0, src->fValue);
  ASSERT_FLOAT_EQ(2.0, dst->fValue);
  ASSERT_FLOAT_EQ(2.0, pst->fValue);

  rack.nextFrame();

  ASSERT_FLOAT_EQ(4.0, src->fValue);
  ASSERT_FLOAT_EQ(4.0, dst->fValue);
  ASSERT_FLOAT_EQ(4.0, pst->fValue);
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

    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override
    {
      MAUSrc::renderBatch(iPropertyDiffs, iDiffCount);
      MCVDst::renderBatch(iPropertyDiffs, iDiffCount);
    }
  };

  const auto MAUSrcCVDstConfig = Config::byDefault<MAUSrcCVDst>([](auto &def, auto &rtc, auto &rt) {
    // MAUSrcCVDst emits audio
    def.audio_outputs[MAUSrcCVDst::LEFT_SOCKET] = jbox.audio_output();
    def.audio_outputs[MAUSrcCVDst::RIGHT_SOCKET] = jbox.audio_output();

    // MAUSrcCVDst receives CV
    def.cv_inputs[MAUSrcCVDst::SOCKET] = jbox.cv_input();
  });

  struct MAUDstCVSrc : public MAUDst, MCVSrc
  {
    MAUDstCVSrc(int iSampleRate) : MAUDst(iSampleRate), MCVSrc(iSampleRate){}
    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override
    {
      MAUDst::renderBatch(iPropertyDiffs, iDiffCount);
      MCVSrc::renderBatch(iPropertyDiffs, iDiffCount);
    }
  };

  const auto MAUDstCVSrcConfig = Config::byDefault<MAUDstCVSrc>([](auto &def, auto &rtc, auto &rt) {
    // MAUDstCVSrc receives audio
    def.audio_inputs[MAUDstCVSrc::LEFT_SOCKET] = jbox.audio_input();
    def.audio_inputs[MAUDstCVSrc::RIGHT_SOCKET] = jbox.audio_input();

    // MAUDstCVSrc emits CV
    def.cv_outputs[MAUDstCVSrc::SOCKET] = jbox.cv_output();
  });


  Rack rack{};

  auto ex1 = rack.newDevice<MAUSrcCVDst>(MAUSrcCVDstConfig);
  auto ex2 = rack.newDevice<MAUDstCVSrc>(MAUDstCVSrcConfig);

  // we do not connect the right socket on purpose (no need to create the circular dependency)
  rack.wire(ex1.getAudioOutSocket(MAUSrcCVDst::LEFT_SOCKET), ex2.getAudioInSocket(MAUDstCVSrc::LEFT_SOCKET));
  rack.wire(ex2.getCVOutSocket(MAUDstCVSrc::SOCKET), ex1.getCVInSocket(MAUSrcCVDst::SOCKET));

  // we make sure that this does not introduce an infinite loop
  rack.nextFrame();

  ex1->fBuffer.fill(2.0, 0);
  ex2->fValue = 3.0;

  rack.nextFrame();

  // due to circular dependency, and the fact that devices are processed in order of id, ex2 out will be processed
  // first so the CV dev->fValue will make in this frame while the audio buffer will make it in the next
  ASSERT_TRUE(ex2->fBuffer.check(0.0, 0));
  ASSERT_FLOAT_EQ(3.0, ex1->fValue);

  rack.nextFrame();

  // now the audio buffer has caught up
  ASSERT_TRUE(ex2->fBuffer.check(2.0, 0));
  ASSERT_FLOAT_EQ(3.0, ex1->fValue);
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

  Rack rack{};

  auto dev = rack.newDeviceByDefault<SelfConnectedDevice>([](auto &def, auto &rtc, auto &rt) {
    def.cv_inputs["I"] = jbox.cv_input();
    def.cv_outputs["O"] = jbox.cv_output();
  });

  rack.wire(dev.getCVOutSocket("O"), dev.getCVInSocket("I"));

  rack.nextFrame();

  ASSERT_FLOAT_EQ(1.0, dev->fValue);

  rack.nextFrame();

  ASSERT_FLOAT_EQ(2.0, dev->fValue);

  rack.nextFrame();

  ASSERT_FLOAT_EQ(3.0, dev->fValue);
}

}