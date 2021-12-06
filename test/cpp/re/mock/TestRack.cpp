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
#include <re/mock/MockJukebox.h>

namespace re::mock::Test {

using namespace mock;

// Rack.Basic
TEST(Rack, Basic)
{
  Rack rack{};

  auto re = rack.newExtension(Config{DeviceType::kHelper});

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

  // we bypass the device
  pst.use([&pst]{
    pst->setBypassState(kJBox_EnabledOff);
  });

  rack.nextFrame();

  // dst should be 0
  ASSERT_EQ(src->fBuffer, MockAudioDevice::buffer(4.0, 5.0));
  ASSERT_EQ(dst->fBuffer, MockAudioDevice::buffer(0.0, 0.0));
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
document_owner_properties["prop_string"]       = jbox.string { default = 'abc' }
document_owner_properties["prop_volume_ro"]       = jbox.number { default = 0.8 }
document_owner_properties["prop_volume_rw"]       = jbox.number { default = 0.9 }
rtc_owner_properties["prop_gain_ro"]               = jbox.native_object{ }
rtc_owner_properties["prop_gain_rw"]               = jbox.native_object{ }
)")
    .mdef(Config::rtc_owner_property("prop_blob", lua::jbox_blob_property{}))
    .mdef(Config::rtc_owner_property("prop_sample", lua::jbox_sample_property{}))
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
  ASSERT_STREQ("DSPBuffer{", re.toString("/audio_outputs/output_1/buffer").substr(0, 10).c_str());
  ASSERT_STREQ("abc", re.toString("/custom_properties/prop_string").c_str());
  ASSERT_STREQ("RONativeObject{", re.toString("/custom_properties/prop_gain_ro").substr(0, 15).c_str());
  ASSERT_STREQ("RWNativeObject{", re.toString("/custom_properties/prop_gain_rw").substr(0, 15).c_str());
  ASSERT_STREQ("Blob{.fSize=0,.fResidentSize=0,.fLoadStatus=nil,.fBlobPath=[]}", re.toString("/custom_properties/prop_blob").c_str());
  ASSERT_STREQ("Sample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}", re.toString("/custom_properties/prop_sample").c_str());

  re.use([&re]{
    auto o1 = JBox_GetMotherboardObjectRef("/audio_outputs/output_1");
    ASSERT_STREQ("/audio_outputs/output_1", re.getObjectPath(o1).c_str());
    ASSERT_STREQ("/audio_outputs/output_1/buffer", re.toString(JBox_MakePropertyRef(o1, "buffer")).c_str());

    TJBox_Value values[] = { JBox_MakeNil(),
                             re.getValue("/audio_outputs/output_1/connected"),
                             re.getValue("/audio_outputs/output_1/buffer"),
                             re.getValue("/custom_properties/prop_string"),
                             re.getValue("/custom_properties/prop_volume_ro"),
                             re.getValue("/custom_properties/prop_gain_ro"),
                             re.getValue("/custom_properties/prop_gain_rw"),
                             re.getValue("/custom_properties/prop_blob"),
                             re.getValue("/custom_properties/prop_sample"),
                             };

    JBOX_TRACEVALUES("Nil=^0, connected=^1, buffer=^2, prop_string=^3, prop_volume_ro=^4, prop_gain_ro=^5, prop_gain_rw=^6, prop_blob=^7, prop_sample=^8, Nil=^0", values, 9);
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
      fDiffMap.clear();

      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        fDiffs.emplace_back(diff);
        fDiffMap[JBox_toString(diff.fPropertyRef)] =
          JBox_toString(diff.fPreviousValue) + "->" + JBox_toString(diff.fCurrentValue) + "@" + std::to_string(diff.fAtFrameIndex);
      }
    }

    std::vector<TJBox_PropertyDiff> fDiffs{};
    std::map<std::string, std::string> fDiffMap{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .accept_notes(true)
    .mdef(Config::audio_in("input"))
    .mdef(Config::cv_in("cv"))
    .mdef(Config::document_owner_property("prop_float", lua::jbox_number_property{}.default_value(0.8)))
    .mdef(Config::document_owner_property("prop_bool", lua::jbox_boolean_property{}))
    .mdef(Config::document_owner_property("prop_untracked", lua::jbox_boolean_property{}))
    .mdef(Config::document_owner_property("prop_string", lua::jbox_string_property{}.default_value("abcd")))
    .rtc(Config::rt_input_setup_notify("/cv_inputs/cv/*"))
    .rtc(Config::rt_input_setup_notify("/audio_inputs/input/connected"))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_float"))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_bool"))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_string"))
    .rtc(Config::rt_input_setup_notify("/note_states/69"));

  auto src = rack.newDevice(MAUSrc::CONFIG);
  auto re = rack.newDevice(c);

  rack.wire(src.getAudioOutSocket(MAUSrc::LEFT_SOCKET), re.getAudioInSocket("input"));
  re.setNoteInEvent(69, 100, 25);

  rack.nextFrame();

  // there are 9 diffs because
  // 1. /audio_inputs/input/connected gets "initialized" with false then get updated with true when wiring happens => 2 updates
  // 2. /note_states/69 gets "initialized" with 0 then get updated with 100 => 2 updates
  ASSERT_EQ(9, re->fDiffs.size());

  {
    std::map<std::string, std::string> expected{};
    expected["/audio_inputs/input/connected"] = "false->true@0";
    expected["/custom_properties/prop_bool"] = "false->false@0";
    expected["/custom_properties/prop_float"] = "0.800000->0.800000@0";
    expected["/custom_properties/prop_string"] = "abcd->abcd@0";
    expected["/cv_inputs/cv/connected"] = "false->false@0";
    expected["/cv_inputs/cv/value"] = "0.000000->0.000000@0";
    expected["/note_states/69"] = "0.000000->100.000000@25";

    ASSERT_EQ(expected, re->fDiffMap);
  }

  auto noteStateDiff = re->fDiffs[re->fDiffs.size() - 1];
  re.use([&noteStateDiff] {
    auto noteEvent = JBox_AsNoteEvent(noteStateDiff);
    ASSERT_EQ(69, noteEvent.fNoteNumber);
    ASSERT_EQ(100, noteEvent.fVelocity);
    ASSERT_EQ(25, noteEvent.fAtFrameIndex);
  });

  // write in any order but make sure it gets ordered by fAtFrameIndex
  re.setNoteInEvent(69, 100, 0);
  re.setNoteInEvent(69, 50, 10);
  re.setNoteInEvent(69, 120, 5);
  re.setNoteInEvent(69, 90, 3);

  rack.nextFrame();

  ASSERT_EQ(4, re->fDiffs.size());
  re.use([&re] {
    auto noteEvent = JBox_AsNoteEvent(re->fDiffs[0]);
    ASSERT_EQ(69, noteEvent.fNoteNumber); ASSERT_EQ(100, noteEvent.fVelocity); ASSERT_EQ(0, noteEvent.fAtFrameIndex);
    noteEvent = JBox_AsNoteEvent(re->fDiffs[1]);
    ASSERT_EQ(69, noteEvent.fNoteNumber); ASSERT_EQ(90, noteEvent.fVelocity); ASSERT_EQ(3, noteEvent.fAtFrameIndex);
    noteEvent = JBox_AsNoteEvent(re->fDiffs[2]);
    ASSERT_EQ(69, noteEvent.fNoteNumber); ASSERT_EQ(120, noteEvent.fVelocity); ASSERT_EQ(5, noteEvent.fAtFrameIndex);
    noteEvent = JBox_AsNoteEvent(re->fDiffs[3]);
    ASSERT_EQ(69, noteEvent.fNoteNumber); ASSERT_EQ(50, noteEvent.fVelocity); ASSERT_EQ(10, noteEvent.fAtFrameIndex);
  });

  re.setBool("/custom_properties/prop_bool", true);
  re.setNum("/custom_properties/prop_float", 0.9);
  re.setString("/custom_properties/prop_string", "efg");

  rack.nextFrame();

  ASSERT_EQ(3, re->fDiffs.size());

  {
    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_bool"] = "false->true@0";
    expected["/custom_properties/prop_float"] = "0.800000->0.900000@0";
    expected["/custom_properties/prop_string"] = "abcd->efg@0";

    ASSERT_EQ(expected, re->fDiffMap);
  }

  auto patchString = R"(
<?xml version="1.0"?>
<JukeboxPatch version="2.0"  deviceProductID="com.acme.Kooza"  deviceVersion="1.0.0d1" >
    <DeviceNameInEnglish>
        Kooza
    </DeviceNameInEnglish>
    <Properties>
        <Object name="custom_properties" >
            <Value property="prop_float"  type="number" >
                0.5
            </Value>
            <Value property="prop_bool"  type="boolean" >
                false
            </Value>
            <Value property="prop_string"  type="string" >
                414243
            </Value>
        </Object>
        <Object name="transport" />
    </Properties>
</JukeboxPatch>
)";

  re.loadPatch(ConfigString{patchString});

  rack.nextFrame();

  ASSERT_EQ(3, re->fDiffs.size());

  {
    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_bool"] = "true->false@0";
    expected["/custom_properties/prop_float"] = "0.900000->0.500000@0";
    expected["/custom_properties/prop_string"] = "efg->ABC@0";

    ASSERT_EQ(expected, re->fDiffMap);
  }
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