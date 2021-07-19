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

  ASSERT_THROW(JBox_GetMotherboardObjectRef("/custom_properties"), Exception);

  re.use([]() {
    // now this works
    JBox_GetMotherboardObjectRef("/custom_properties");
  });

}

// Rack.AudioWiring
TEST(Rack, AudioWiring) {
  Rack rack{};

  auto src = rack.newDevice<MAUSrc>(MAUSrc::Config);

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(0, 0));

  auto dst = rack.newDevice<MAUDst>(MAUDst::Config);

  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(0, 0));

  auto pst = rack.newDevice<MAUPst>(MAUPst::Config);

  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(0, 0));

  MockAudioDevice::wire(rack, src, pst);
  MockAudioDevice::wire(rack, pst, dst);

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(0, 0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(0, 0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(0, 0));

  rack.nextFrame();

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(0, 0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(0, 0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(0, 0));

  src->fBuffer.fill(2.0, 3.0);

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(2.0, 3.0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(0, 0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(0, 0));

  rack.nextFrame();

  src->fBuffer.fill(4.0, 5.0);

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(4.0, 5.0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(2.0, 3.0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(2.0, 3.0));

 rack.nextFrame();

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(4.0, 5.0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(4.0, 5.0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(4.0, 5.0));
}

// Rack.AudioWiring2 (different wiring api)
TEST(Rack, AudioWiring2) {
  Rack rack{};

  auto src = rack.newDevice<MAUSrc>(MAUSrc::Config);
  auto dst = rack.newDevice<MAUDst>(MAUDst::Config);
  auto pst = rack.newDevice<MAUPst>(MAUPst::Config);

  MockAudioDevice::wire(rack, src, pst.getStereoAudioInSocket(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET));
  MockAudioDevice::wire(rack, pst.getStereoAudioOutSocket(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET), dst);

  src->fBuffer.fill(2.0, 3.0);

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(2.0, 3.0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(0, 0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(0, 0));

  rack.nextFrame();

  src->fBuffer.fill(4.0, 5.0);

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(4.0, 5.0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(2.0, 3.0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(2.0, 3.0));
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
  ASSERT_EQ(ex2->fBuffer, MockAudioDevice::buffer(0, 0));
  ASSERT_FLOAT_EQ(3.0, ex1->fValue);

  rack.nextFrame();

  // now the audio buffer has caught up
  ASSERT_EQ(ex2->fBuffer, MockAudioDevice::buffer(2.0, 0));
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

// Rack.toString
TEST(Rack, toString)
{
  Rack rack{};

  struct Gain
  {
    TJBox_Float64 fVolume{};
  };

  Config c([](auto &def, auto &rtc, auto &rt) {
    def.audio_outputs["output_1"] = jbox.audio_output();

    def.document_owner.properties["prop_volume_ro"]       = jbox.number<float>(0.8);
    def.document_owner.properties["prop_volume_rw"]       = jbox.number<float>(0.9);
    def.rt_owner.properties["prop_gain_ro"]               = jbox.native_object();
    def.rt_owner.properties["prop_gain_rw"]               = jbox.native_object();

    rtc.rtc_bindings["/custom_properties/prop_volume_ro"]   = "/global_rtc/new_gain_ro";
    rtc.rtc_bindings["/custom_properties/prop_volume_rw"]   = "/global_rtc/new_gain_rw";

    rtc.rtc_bindings["/custom_properties/prop_volume_ro"]   = "/global_rtc/new_gain_ro";
    rtc.rtc_bindings["/custom_properties/prop_volume_rw"]   = "/global_rtc/new_gain_rw";

    rtc.global_rtc["new_gain_ro"] = [](std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue) {
      auto new_no = jbox.make_native_object_ro("Gain", { iNewValue });
      jbox.store_property("/custom_properties/prop_gain_ro", new_no);
    };

    rtc.global_rtc["new_gain_rw"] = [](std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue) {
      auto new_no = jbox.make_native_object_rw("Gain", { iNewValue });
      jbox.store_property("/custom_properties/prop_gain_rw", new_no);
    };

    rt.create_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount) -> void * {
      if(std::strcmp(iOperation, "Gain") == 0)
        return new Gain{JBox_GetNumber(iParams[0])};

      return nullptr;
    };

    rt.destroy_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount, void *iNativeObject) {
      if(std::strcmp(iOperation, "Gain") == 0)
      {
        delete reinterpret_cast<Gain *>(iNativeObject);
      }
    };
  });

  auto re = rack.newExtension(c);
  ASSERT_STREQ("Nil", re.toString(JBox_MakeNil()).c_str());
  ASSERT_STREQ("true", re.toString(JBox_MakeBoolean(true)).c_str());
  ASSERT_STREQ("false", re.toString(JBox_MakeBoolean(false)).c_str());
  ASSERT_STREQ("false", re.toString("/audio_outputs/output_1/connected").c_str());
  ASSERT_STREQ("0.800000", re.toString("/custom_properties/prop_volume_ro").c_str());
  ASSERT_STREQ("DSPBuffer[1]", re.toString("/audio_outputs/output_1/buffer").c_str());
  ASSERT_STREQ("RONativeObject[1]", re.toString("/custom_properties/prop_gain_ro").c_str());
  ASSERT_STREQ("RWNativeObject[2]", re.toString("/custom_properties/prop_gain_rw").c_str());

  re.use([&re]{
    auto o1 = JBox_GetMotherboardObjectRef("/audio_outputs/output_1");
    ASSERT_STREQ("/audio_outputs/output_1", re.getObjectPath(o1).c_str());
    ASSERT_STREQ("/audio_outputs/output_1/buffer", re.toString(JBox_MakePropertyRef(o1, "buffer")).c_str());

    TJBox_Value values[] = { JBox_MakeNil(),
                             re.getValue("/audio_outputs/output_1/connected"),
                             re.getValue("/audio_outputs/output_1/buffer"),
                             re.getValue("/custom_properties/prop_volume_ro"),
                             re.getValue("/custom_properties/prop_gain_ro"),
                             re.getValue("/custom_properties/prop_gain_rw"),
                             };

    JBOX_TRACEVALUES("Nil=^0, connected=^1, buffer=^2, prop_volume_ro=^3, prop_gain_ro=^4, prop_gain_rw=^5, Nil=^0", values, 6);
  });
}

// Rack.Diff
TEST(Rack, Diff)
{
  Rack rack{};

  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      fDiffs.clear();

      for(int i = 0; i < iDiffCount; i++)
        fDiffs.emplace_back(iPropertyDiffs[i]);
    }

    std::vector<TJBox_PropertyDiff> fDiffs{};
  };

  auto c = Config::byDefault<Device>([](auto &def, auto &rtc, auto &rt) {
    def.audio_inputs["input"] = jbox.audio_input();
    def.cv_inputs["cv"] = jbox.cv_input();

    def.document_owner.properties["prop_float"]       = jbox.number<float>(0.8);
    def.document_owner.properties["prop_bool"]        = jbox.boolean();
    def.document_owner.properties["prop_untracked"]   = jbox.boolean();

    // realtime_controller.lua
    rtc.rt_input_setup.notify = {
      "/cv_inputs/cv/connected",
      "/cv_inputs/cv/value",

      "/audio_inputs/input/connected",

      "/custom_properties/prop_float",
      "/custom_properties/prop_bool",
    };
  });

  auto src = rack.newDevice<MAUSrc>(MAUSrc::Config);
  auto re = rack.newDevice<Device>(c);

  rack.wire(src.getAudioOutSocket(MAUSrc::LEFT_SOCKET), re.getAudioInSocket("input"));

  rack.nextFrame();

  // there are 6 diffs because /audio_inputs/input/connected gets "initialized" with false
  // then get updated with true when wiring happens => 2 updates
  ASSERT_EQ(6, re->fDiffs.size());

  std::map<std::string, std::string> diffMap{};

  for(auto &diff: re->fDiffs)
    diffMap[re.toString(diff.fPropertyRef)] = re.toString(diff.fCurrentValue);

  std::map<std::string, std::string> expected{};
  expected["/audio_inputs/input/connected"] = "true";
  expected["/custom_properties/prop_bool"] = "false";
  expected["/custom_properties/prop_float"] = "0.800000";
  expected["/cv_inputs/cv/connected"] = "false";
  expected["/cv_inputs/cv/value"] = "0.000000";

  ASSERT_EQ(expected, diffMap);
}

// Rack.InstanceID
TEST(Rack, InstanceID)
{
  Rack rack{};

  struct Device
  {
    Device(int instanceID) : fInstanceID{instanceID} {}

    void renderBatch(TJBox_PropertyDiff const *, TJBox_UInt32)
    {
    }

    int fInstanceID;

    std::vector<TJBox_PropertyDiff> fDiffs{};
  };

  auto c = Config([](auto &def, auto &rtc, auto &rt) {

    rtc.rtc_bindings["/environment/instance_id"] = "/global_rtc/init_instance";

    rtc.global_rtc["init_instance"] = [](std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue) {
      auto new_no = jbox.make_native_object_rw("Instance", { iNewValue });
      jbox.store_property("/custom_properties/instance", new_no);
    };

    rt = Realtime::byDefault<Device>();
  });

  auto re = rack.newDevice<Device>(c);

  ASSERT_TRUE(re.getInstance<Device>() != nullptr);
  ASSERT_EQ(1, re->fInstanceID);

}
}