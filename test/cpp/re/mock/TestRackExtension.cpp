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
#include <re/mock/stl.h>
#include <gtest/gtest.h>

namespace re::mock::Test {

using namespace mock;

// RackExtension.Motherboard
TEST(RackExtension, Motherboard)
{
  Rack rack{};

  struct Gain
  {
    TJBox_Float64 fVolume{};
  };

  struct Device : public MAUPst, public MCVPst
  {
    Device(int iSampleRate) : MAUPst(iSampleRate), MCVPst(iSampleRate) {}

    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override
    {
      auto fn = [](auto &item) { item = (item + 1.0) * 2.0; };
      copyBuffer(MAUPst::fInSocket, fBuffer);
      stl::for_each(fBuffer.fLeft, fn);
      stl::for_each(fBuffer.fRight, fn);
      copyBuffer(fBuffer, MAUPst::fOutSocket);

      loadValue(MCVPst::fInSocket);
      fn(fValue);
      storeValue(MCVPst::fOutSocket);

      auto customProperties = JBox_GetMotherboardObjectRef("/custom_properties");
      fNumber = JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_number")));
      fFloat = static_cast<float>(JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_float"))));
      fInt = static_cast<int>(JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_int"))));
      fBool = JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_bool")));
    }

    TJBox_Float64 fNumber{};
    float fFloat{};
    int fInt{};
    bool fBool{};
  };

  auto c = Config::byDefault<Device>([](LuaJbox &jbox, MotherboardDef &def, RealtimeController &rtc, Realtime &rt) {
    def.document_owner.properties["prop_number"]  = jbox.number(0.1);
    def.document_owner.properties["prop_float"]   = jbox.number<float>(0.8);
    def.document_owner.properties["prop_int"]     = jbox.number<int>(4);
    def.document_owner.properties["prop_bool"]    = jbox.boolean(true);
    def.document_owner.properties["volume_ro"]    = jbox.number(0.7);
    def.document_owner.properties["volume_rw"]    = jbox.number(0.75);
    def.rt_owner.properties["gain_ro"]            = jbox.native_object();
    def.rt_owner.properties["gain_rw"]            = jbox.native_object();

    def.audio_outputs[MAUPst::LEFT_SOCKET] = jbox.audio_output();
    def.audio_outputs[MAUPst::RIGHT_SOCKET] = jbox.audio_output();
    def.audio_inputs[MAUPst::LEFT_SOCKET] = jbox.audio_input();
    def.audio_inputs[MAUPst::RIGHT_SOCKET] = jbox.audio_input();

    def.cv_inputs[MCVPst::SOCKET] = jbox.cv_input();
    def.cv_outputs[MCVPst::SOCKET] = jbox.cv_output();

    rtc.rtc_bindings["/environment/system_sample_rate"]    = "/global_rtc/init_instance";
    rtc.rtc_bindings["/custom_properties/volume_ro"]       = "/global_rtc/new_gain_ro";
    rtc.rtc_bindings["/custom_properties/volume_rw"]       = "/global_rtc/new_gain_rw";

    rtc.global_rtc["init_instance"] = [&jbox](std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue) {
      auto new_no = jbox.make_native_object_rw("Instance", { iNewValue });
      jbox.store_property("/custom_properties/instance", new_no);
    };

    rtc.global_rtc["new_gain_ro"] = [&jbox](std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue) {
      auto new_no = jbox.make_native_object_ro("Gain", { iNewValue });
      jbox.store_property("/custom_properties/gain_ro", new_no);
    };

    rtc.global_rtc["new_gain_rw"] = [&jbox](std::string const &iSourcePropertyPath, TJBox_Value const &iNewValue) {
      auto new_no = jbox.make_native_object_rw("Gain", { iNewValue });
      jbox.store_property("/custom_properties/gain_rw", new_no);
    };

    rt.create_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount) -> void * {
      LOG_INFO("rt.create_native_object(%s)", iOperation);
      if(std::strcmp(iOperation, "Instance") == 0)
      {
        if(iCount >= 1)
        {
          TJBox_Float64 sampleRate = JBox_GetNumber(iParams[0]);
          return new Device(static_cast<int>(sampleRate));
        }
      }

      if(std::strcmp(iOperation, "Gain") == 0)
        return new Gain{JBox_GetNumber(iParams[0])};

      return nullptr;
    };

    rt.destroy_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount, void *iNativeObject) {
      if(std::strcmp(iOperation, "Instance") == 0)
      {
        delete reinterpret_cast<Device *>(iNativeObject);
      }

      if(std::strcmp(iOperation, "Gain") == 0)
      {
        delete reinterpret_cast<Gain *>(iNativeObject);
      }
    };

  });

  auto re = rack.newDevice<Device>(c);
  auto auSrc = rack.newDevice<MAUSrc>(MAUSrc::Config);
  auto cvSrc = rack.newDevice<MCVSrc>(MCVSrc::Config);
  auto auDst = rack.newDevice<MAUDst>(MAUDst::Config);
  auto cvDst = rack.newDevice<MCVDst>(MCVDst::Config);

  MockAudioDevice::wire(rack, auSrc, re);
  MockAudioDevice::wire(rack, re, auDst);
  MockCVDevice::wire(rack, cvSrc, re);
  MockCVDevice::wire(rack, re, cvDst);

  auSrc->fBuffer.fill(1.0, 2.0);
  cvSrc->fValue = 5.0;

  rack.nextFrame();

  // we make sure that everything was created properly
  ASSERT_FLOAT_EQ(0.1, re->fNumber);
  ASSERT_FLOAT_EQ(0.8, re->fFloat);
  ASSERT_EQ(4, re->fInt);
  ASSERT_TRUE(re->fBool);
  ASSERT_EQ(re->fBuffer, MockAudioDevice::buffer(4.0, 6.0));
  ASSERT_FLOAT_EQ(12.0, re->fValue);

  // check the motherboard apis - get
  ASSERT_FLOAT_EQ(0.1, JBox_GetNumber(re.getValue("/custom_properties/prop_number")));
  ASSERT_FLOAT_EQ(0.8, re.getNum<float>("/custom_properties/prop_float"));
  ASSERT_EQ(4, re.getNum<int>("/custom_properties/prop_int"));
  ASSERT_TRUE(re.getBool("/custom_properties/prop_bool"));
  {
    auto buffer = MockAudioDevice::StereoBuffer{
      .fLeft = re.getDSPBuffer("/audio_inputs/L"),
      .fRight = re.getDSPBuffer(re.getAudioInSocket("R"))
    };
    ASSERT_EQ(buffer, MockAudioDevice::buffer(1.0, 2.0));
  }
  {
    auto buffer = MockAudioDevice::StereoBuffer{
      .fLeft = re.getDSPBuffer("/audio_outputs/L"),
      .fRight = re.getDSPBuffer(re.getAudioOutSocket("R"))
    };
    ASSERT_EQ(buffer, MockAudioDevice::buffer(4.0, 6.0));
  }

  ASSERT_FLOAT_EQ(5.0, re.getCVSocketValue("/cv_inputs/C"));
  ASSERT_FLOAT_EQ(5.0, re.getCVSocketValue(re.getCVInSocket("C")));
  ASSERT_FLOAT_EQ(12.0, re.getCVSocketValue("/cv_outputs/C"));
  ASSERT_FLOAT_EQ(12.0, re.getCVSocketValue(re.getCVOutSocket("C")));

  ASSERT_FLOAT_EQ(0.7, re.getNativeObjectRO<Gain>("/custom_properties/gain_ro")->fVolume);
  ASSERT_THROW(re.getNativeObjectRW<Gain>("/custom_properties/gain_ro"), Exception);
  ASSERT_FLOAT_EQ(0.75, re.getNativeObjectRO<Gain>("/custom_properties/gain_rw")->fVolume);
  ASSERT_FLOAT_EQ(0.75, re.getNativeObjectRW<Gain>("/custom_properties/gain_rw")->fVolume);

  // check the motherboard apis - set
  re.setValue("/custom_properties/prop_number", JBox_MakeNumber(100.1));
  ASSERT_FLOAT_EQ(100.1, JBox_GetNumber(re.getValue("/custom_properties/prop_number")));
  re.setNum<float>("/custom_properties/prop_float", 100.8);
  ASSERT_FLOAT_EQ(100.8, re.getNum<float>("/custom_properties/prop_float"));
  re.setNum<int>("/custom_properties/prop_int", 104);
  ASSERT_EQ(104, re.getNum<int>("/custom_properties/prop_int"));
  re.setBool("/custom_properties/prop_bool", false);
  ASSERT_FALSE(re.getBool("/custom_properties/prop_bool"));

  {
    auto buffer = MockAudioDevice::buffer(101.0, 102.0);
    re.setDSPBuffer("/audio_outputs/L", buffer.fLeft);
    re.setDSPBuffer(re.getAudioOutSocket("R"), buffer.fRight);
    buffer = MockAudioDevice::StereoBuffer{
      .fLeft = re.getDSPBuffer("/audio_outputs/L"),
      .fRight = re.getDSPBuffer(re.getAudioOutSocket("R"))
    };
    ASSERT_EQ(buffer, MockAudioDevice::buffer(101.0, 102.0));
  }

  re.setCVSocketValue("/cv_inputs/C", 105.0);
  ASSERT_FLOAT_EQ(105.0, re.getCVSocketValue("/cv_inputs/C"));

  re.setCVSocketValue(re.getCVOutSocket("C"), 112.0);
  ASSERT_FLOAT_EQ(112.0, re.getCVSocketValue("/cv_outputs/C"));
}

}