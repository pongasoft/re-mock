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

  auto src = rack.newDevice(MAUSrc::CONFIG);

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(0, 0));

  auto dst = rack.newDevice(MAUDst::CONFIG);

  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(0, 0));

  auto pst = rack.newDevice(MAUPst::CONFIG);

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

// Rack.AudioUnwiring
TEST(Rack, AudioUnwiring) {
  Rack rack{};

  auto pst = rack.newDevice(MAUPst::CONFIG);
  auto src = rack.newDevice(MAUSrc::CONFIG);
  auto dst = rack.newDevice(MAUDst::CONFIG);

  MockAudioDevice::wire(rack, src, pst);
  MockAudioDevice::wire(rack, pst, dst);

  src->fBuffer.fill(2.0, 3.0);

  rack.nextFrame();

  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(2.0, 3.0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(2.0, 3.0));

  // disconnecting LEFT socket
  rack.unwire(src.getAudioOutSocket(MAUSrc::LEFT_SOCKET));

  src->fBuffer.fill(4.0, 5.0);

  rack.nextFrame();

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(4.0, 5.0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(2.0, 5.0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(2.0, 5.0));

  // disconnecting LEFT & RIGHT socket (LEFT was already disconnected so it's ok)
  rack.unwire(src.getStereoAudioOutSocket(MAUSrc::LEFT_SOCKET, MAUSrc::RIGHT_SOCKET));

  src->fBuffer.fill(6.0, 7.0);

  rack.nextFrame();

  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(6.0, 7.0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(2.0, 5.0));
  ASSERT_EQ(pst->fBuffer, MockAudioDevice::buffer(2.0, 5.0));
}

// Rack.AudioWiring2 (different wiring api)
TEST(Rack, AudioWiring2) {
  Rack rack{};

  auto src = rack.newDevice(MAUSrc::CONFIG);
  auto dst = rack.newDevice(MAUDst::CONFIG);
  auto pst = rack.newDevice(MAUPst::CONFIG);

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

  auto src = rack.newDevice(MCVSrc::CONFIG);

  ASSERT_FLOAT_EQ(0, src->fValue);

  auto dst = rack.newDevice(MCVDst::CONFIG);

  ASSERT_FLOAT_EQ(0, dst->fValue);

  auto pst = rack.newDevice(MCVPst::CONFIG);

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

// Rack.CVUnWiring
TEST(Rack, CVUnWiring) {
  Rack rack{};

  auto src = rack.newDevice(MCVSrc::CONFIG);
  auto dst = rack.newDevice(MCVDst::CONFIG);
  auto pst = rack.newDevice(MCVPst::CONFIG);
  MockCVDevice::wire(rack, src, pst);
  MockCVDevice::wire(rack, pst, dst);

  src->fValue = 2.0;

  rack.nextFrame();

  ASSERT_FLOAT_EQ(2.0, src->fValue);
  ASSERT_FLOAT_EQ(2.0, pst->fValue);
  ASSERT_FLOAT_EQ(2.0, dst->fValue);

  rack.unwire(pst.getCVOutSocket(MCVDst::SOCKET));
  src->fValue = 4.0;

  rack.nextFrame();

  ASSERT_FLOAT_EQ(4.0, src->fValue);
  ASSERT_FLOAT_EQ(4.0, pst->fValue);
  ASSERT_FLOAT_EQ(2.0, dst->fValue);
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

  const auto MAUSrcCVDstConfig = DeviceConfig<MAUSrcCVDst>::fromSkeleton()
    .mdef(Config::stereo_audio_out())
    .mdef(Config::cv_in());

  struct MAUDstCVSrc : public MAUDst, MCVSrc
  {
    MAUDstCVSrc(int iSampleRate) : MAUDst(iSampleRate), MCVSrc(iSampleRate){}
    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override
    {
      MAUDst::renderBatch(iPropertyDiffs, iDiffCount);
      MCVSrc::renderBatch(iPropertyDiffs, iDiffCount);
    }
  };

  const auto MAUDstCVSrcConfig = DeviceConfig<MAUDstCVSrc>::fromSkeleton()
    .mdef(Config::stereo_audio_in())
    .mdef(Config::cv_out());

  Rack rack{};

  auto ex1 = rack.newDevice(MAUSrcCVDstConfig);
  auto ex2 = rack.newDevice(MAUDstCVSrcConfig);

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

  auto dev = rack.newDevice(DeviceConfig<SelfConnectedDevice>::fromSkeleton()
                              .mdef(Config::cv_in("I"))
                              .mdef(Config::cv_out("O")));

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

  auto c = Config::fromSkeleton()
    .mdef(Config::audio_out("output_1"))
    .mdef_string(R"(
document_owner_properties["prop_volume_ro"]       = jbox.number { default = 0.8 }
document_owner_properties["prop_volume_rw"]       = jbox.number { default = 0.9 }
rtc_owner_properties["prop_gain_ro"]               = jbox.native_object{ }
rtc_owner_properties["prop_gain_rw"]               = jbox.native_object{ }
)")
    .rtc(Config::rtc_binding("/custom_properties/prop_volume_ro", "/global_rtc/new_gain_ro"))
    .rtc(Config::rtc_binding("/custom_properties/prop_volume_rw", "/global_rtc/new_gain_rw"))
    .rtc_string(R"(
global_rtc["new_gain_ro"] = function(source_property_path, new_value)
  local new_no = jbox.make_native_object_ro("Gain", { new_value })
  jbox.store_property("/custom_properties/prop_gain_ro", new_no)
end

global_rtc["new_gain_rw"] = function(source_property_path, new_value)
  local new_no = jbox.make_native_object_rw("Gain", { new_value })
  jbox.store_property("/custom_properties/prop_gain_rw", new_no)
end
)")
    .rt([](Realtime &rt) {
      rt.create_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount) -> void * {
        RE_MOCK_LOG_INFO("rt.create_native_object(%s)", iOperation);
        if(std::strcmp(iOperation, "Gain") == 0)
          return new Gain{JBox_GetNumber(iParams[0])};

        return nullptr;
      };

      rt.destroy_native_object = Realtime::destroyer<Gain>("Gain");
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

  auto c = DeviceConfig<Device>::fromSkeleton()
    .mdef(Config::audio_in("input"))
    .mdef(Config::cv_in("cv"))
    .mdef(Config::document_owner_property("prop_float", lua::jbox_number_property{}.default_value(0.8)))
    .mdef(Config::document_owner_property("prop_bool", lua::jbox_boolean_property{}))
    .mdef(Config::document_owner_property("prop_untracked", lua::jbox_boolean_property{}))
    .rtc_string(R"(
rt_input_setup = {
  notify = {
    "/cv_inputs/cv/connected",
    "/cv_inputs/cv/value",

    "/audio_inputs/input/connected",

    "/custom_properties/prop_float",
    "/custom_properties/prop_bool",
  }
}
)");

  auto src = rack.newDevice(MAUSrc::CONFIG);
  auto re = rack.newDevice(c);

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

  auto c = DeviceConfig<Device>::fromSkeleton()
    .rtc_string(R"(
rtc_bindings = {
  { source = "/environment/instance_id", dest = "/global_rtc/init_instance" },
}
)");

  auto re = rack.newDevice(c);

  ASSERT_TRUE(re.getInstance<Device>() != nullptr);
  ASSERT_EQ(re.getInstanceId(), re->fInstanceID);
}
}