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
#include <re/mock/MockJukebox.h>
#include <re/mock/stl.h>
#include <re_mock_build.h>
#include <gtest/gtest.h>
#include <atomic>

namespace re::mock::Test {

using namespace mock;

struct Gain {
  Gain(TJBox_Float64 v) : fVolume{v} {
    fCount++;
  }
  ~Gain() {
    fCount--;
  }

  TJBox_Float64 fVolume{};

  static std::atomic<int> fCount;
};

std::atomic<int> Gain::fCount{};

// RackExtension.Motherboard
TEST(RackExtension, Motherboard)
{
  ASSERT_EQ(0, Gain::fCount);

  Rack rack{};

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

      std::array<char, 10> ar{};
      JBox_GetSubstring(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_string")), 0, 4, ar.data());
      fString = std::string(ar.data());

      std::array<TJBox_UInt8 , 3> ar2{'e', 'f', 'g'};
      JBox_SetRTStringData(JBox_MakePropertyRef(customProperties, "prop_rt_string"), ar2.size(), ar2.data());
    }

    TJBox_Float64 fNumber{};
    float fFloat{};
    int fInt{};
    bool fBool{};
    std::string fString{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .mdef(Config::stereo_audio_in())
    .mdef(Config::stereo_audio_out())
    .mdef(Config::cv_in())
    .mdef(Config::cv_out())
    .mdef(Config::document_owner_property("prop_number", lua::jbox_number_property{}.default_value(0.1)))
    .mdef(Config::document_owner_property("prop_float", lua::jbox_number_property{}.default_value(0.8)))
    .mdef(Config::document_owner_property("prop_int", lua::jbox_number_property{}.default_value(4).steps(5)))
    .mdef(Config::document_owner_property("prop_bool", lua::jbox_boolean_property{}.default_value(true)))
    .mdef(Config::document_owner_property("prop_string", lua::jbox_string_property{}.default_value("abcd")))
    .mdef(Config::document_owner_property("volume_ro", lua::jbox_number_property{}.default_value(0.7)))
    .mdef(Config::document_owner_property("volume_rw", lua::jbox_number_property{}.default_value(0.75)))

    .mdef(Config::rtc_owner_property("gain_ro", lua::jbox_native_object{ }))
    .mdef(Config::rtc_owner_property("gain_rw", lua::jbox_native_object{ }))

    .mdef(Config::rt_owner_property("prop_rt_string", lua::jbox_string_property{}.max_size(100)))

    .rtc(Config::rtc_binding("/custom_properties/volume_ro", "/global_rtc/new_gain_ro"))
    .rtc(Config::rtc_binding("/custom_properties/volume_rw", "/global_rtc/new_gain_rw"))
    .rtc_string(R"(
global_rtc["new_gain_ro"] = function(source_property_path, new_value)
  local new_no = jbox.make_native_object_ro("Gain", { new_value })
  jbox.store_property("/custom_properties/gain_ro", new_no)
end

global_rtc["new_gain_rw"] = function(source_property_path, new_value)
  local new_no = jbox.make_native_object_rw("Gain", { new_value })
  jbox.store_property("/custom_properties/gain_rw", new_no)
end
)")
    .rt([](Realtime &rt) {
      rt.create_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount) -> void * {
        RE_MOCK_LOG_INFO("rt.create_native_object(%s)", iOperation);
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

      rt.destroy_native_object = [](const char iOperation[], void *iNativeObject) {
        RE_MOCK_LOG_INFO("rt.destroy_native_object(%s)", iOperation);
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

  auto re = rack.newDevice(c);
  auto auSrc = rack.newDevice(MAUSrc::CONFIG);
  auto cvSrc = rack.newDevice(MCVSrc::CONFIG);
  auto auDst = rack.newDevice(MAUDst::CONFIG);
  auto cvDst = rack.newDevice(MCVDst::CONFIG);

  MockAudioDevice::wire(rack, auSrc, re);
  MockAudioDevice::wire(rack, re, auDst);
  MockCVDevice::wire(rack, cvSrc, re);
  MockCVDevice::wire(rack, re, cvDst);

  auSrc->fBuffer.fill(1.0, 2.0);
  cvSrc->fValue = 5.0;

  rack.nextBatch();

  // we make sure that everything was created properly
  ASSERT_FLOAT_EQ(0.1, re->fNumber);
  ASSERT_FLOAT_EQ(0.8, re->fFloat);
  ASSERT_EQ(4, re->fInt);
  ASSERT_TRUE(re->fBool);
  ASSERT_EQ(re->fBuffer, MockAudioDevice::buffer(4.0, 6.0));
  ASSERT_FLOAT_EQ(12.0, re->fValue);
  ASSERT_EQ("abcd", re->fString);

  // check the motherboard apis - get
  ASSERT_FLOAT_EQ(0.1, JBox_GetNumber(re.getValue("/custom_properties/prop_number")));
  ASSERT_FLOAT_EQ(0.8, re.getNum<float>("/custom_properties/prop_float"));
  ASSERT_EQ(4, re.getNum<int>("/custom_properties/prop_int"));
  ASSERT_TRUE(re.getBool("/custom_properties/prop_bool"));
  ASSERT_EQ("abcd", re.getString("/custom_properties/prop_string"));
  ASSERT_EQ("efg", re.getRTString("/custom_properties/prop_rt_string"));
  {
    auto buffer = MockAudioDevice::StereoBuffer{
      /* .fLeft = */  re.getDSPBuffer("/audio_inputs/L"),
      /* .fRight = */ re.getDSPBuffer(re.getAudioInSocket("R"))
    };
    // after nextBatch input buffers are cleared (they have been consumed)
    ASSERT_EQ(buffer, MockAudioDevice::buffer(0, 0));
  }
  {
    auto buffer = MockAudioDevice::StereoBuffer{
      /* .fLeft = */  re.getDSPBuffer("/audio_outputs/L"),
      /* .fRight = */ re.getDSPBuffer(re.getAudioOutSocket("R"))
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
  ASSERT_THROW(re.setNum<int>("/custom_properties/prop_int", 104), Exception); // must be [0-4]
  re.setNum<int>("/custom_properties/prop_int", 2);
  ASSERT_EQ(2, re.getNum<int>("/custom_properties/prop_int"));
  re.setBool("/custom_properties/prop_bool", false);
  ASSERT_FALSE(re.getBool("/custom_properties/prop_bool"));
  re.setString("/custom_properties/prop_string", "jkl");
  ASSERT_EQ("jkl", re.getString("/custom_properties/prop_string"));
  re.setRTString("/custom_properties/prop_rt_string", "mno");
  ASSERT_EQ("mno", re.getRTString("/custom_properties/prop_rt_string"));

  ASSERT_THROW(re.getRTString("/custom_properties/prop_string"), Exception);
  ASSERT_THROW(re.getString("/custom_properties/prop_rt_string"), Exception);

  {
    auto buffer = MockAudioDevice::buffer(101.0, 102.0);
    re.setDSPBuffer("/audio_outputs/L", buffer.fLeft);
    re.setDSPBuffer(re.getAudioOutSocket("R"), buffer.fRight);
    buffer = MockAudioDevice::StereoBuffer{
      /* .fLeft = */  re.getDSPBuffer("/audio_outputs/L"),
      /* .fRight = */ re.getDSPBuffer(re.getAudioOutSocket("R"))
    };
    ASSERT_EQ(buffer, MockAudioDevice::buffer(101.0, 102.0));
  }

  re.setCVSocketValue("/cv_inputs/C", 105.0);
  ASSERT_FLOAT_EQ(105.0, re.getCVSocketValue("/cv_inputs/C"));

  re.setCVSocketValue(re.getCVOutSocket("C"), 112.0);
  ASSERT_FLOAT_EQ(112.0, re.getCVSocketValue("/cv_outputs/C"));

  auto infos = re.getPropertyInfos();
  ASSERT_EQ(45, infos.size());
  auto expectedInfos =
    "/audio_inputs/L/buffer,\n"
    "/audio_inputs/L/connected,\n"
    "/audio_inputs/R/buffer,\n"
    "/audio_inputs/R/connected,\n"
    "/audio_outputs/L/buffer,\n"
    "/audio_outputs/L/connected,\n"
    "/audio_outputs/L/dsp_latency,\n"
    "/audio_outputs/R/buffer,\n"
    "/audio_outputs/R/connected,\n"
    "/audio_outputs/R/dsp_latency,\n"
    "/custom_properties/gain_ro,\n"
    "/custom_properties/gain_rw,\n"
    "/custom_properties/instance,\n"
    "/custom_properties/prop_bool,\n"
    "/custom_properties/prop_float,\n"
    "/custom_properties/prop_int,\n"
    "/custom_properties/prop_number,\n"
    "/custom_properties/prop_rt_string,\n"
    "/custom_properties/prop_string,\n"
    "/custom_properties/volume_ro,\n"
    "/custom_properties/volume_rw,\n"
    "/cv_inputs/C/connected,\n"
    "/cv_inputs/C/value,\n"
    "/cv_outputs/C/connected,\n"
    "/cv_outputs/C/dsp_latency,\n"
    "/cv_outputs/C/value,\n"
    "/environment/device_visible,\n"
    "/environment/instance_id,\n"
    "/environment/master_tune,\n"
    "/environment/system_sample_rate,\n"
    "/transport/bar_start_pos,\n"
    "/transport/filtered_tempo,\n"
    "/transport/loop_enabled,\n"
    "/transport/loop_end_pos,\n"
    "/transport/loop_start_pos,\n"
    "/transport/muted,\n"
    "/transport/play_pos,\n"
    "/transport/playing,\n"
    "/transport/request_reset_audio,\n"
    "/transport/request_run,\n"
    "/transport/request_stop,\n"
    "/transport/tempo,\n"
    "/transport/tempo_automation,\n"
    "/transport/time_signature_denominator,\n"
    "/transport/time_signature_numerator";
  std::set<std::string> computedInfos{};
  for(auto &info: infos)
  {
    computedInfos.emplace(info.fPropertyPath);
  }
  ASSERT_STREQ(expectedInfos, stl::join_to_string(computedInfos, ",\n").c_str());

  ASSERT_EQ(0, re.getPropertyInfo("/custom_properties/prop_float").fStepCount);
  ASSERT_EQ(0, re.getPropertyInfo("/custom_properties/prop_number").fStepCount);
  ASSERT_EQ(5, re.getPropertyInfo("/custom_properties/prop_int").fStepCount);
}

std::vector<char> to_chars(std::string s)
{
  std::vector<char> res{};
  std::copy(s.begin(), s.end(), std::back_inserter(res));
  return res;
}

std::vector<TJBox_UInt8> to_TJBox_UInt8_vector(std::string s)
{
  std::vector<TJBox_UInt8> res{};
  std::copy(s.begin(), s.end(), std::back_inserter(res));
  return res;
}

// RackExtension.Motherboard_NativeObject
TEST(RackExtension, Motherboard_NativeObject)
{
  Rack rack{};

  struct NativeObject
  {
    NativeObject(TJBox_Float64 iNumberValue, bool iBoolValue, std::string iStringValue) :
      fNumberValue{iNumberValue},
      fBoolValue{iBoolValue},
      fStringValue{std::move(iStringValue)}
    {}

    TJBox_Float64 fNumberValue;
    bool fBoolValue;
    std::string fStringValue;
  };


  auto c = DeviceConfig<MockDevice>::fromSkeleton()
    .mdef(Config::rtc_owner_property("native_object",
                                     lua::jbox_native_object{}.default_value("NativeObject", {12.98f, true, std::string("abc")})))
    .rt([](Realtime &rt) {
      rt.create_native_object = [](const char iOperation[], const TJBox_Value iParams[],
                                   TJBox_UInt32 iCount) -> void * {
        RE_MOCK_LOG_INFO("rt.create_native_object(%s)", iOperation);
        if(std::strcmp(iOperation, "Instance") == 0)
        {
          if(iCount >= 1)
          {
            TJBox_Float64 sampleRate = JBox_GetNumber(iParams[0]);
            return new MockDevice(static_cast<int>(sampleRate));
          }
        }

        if(std::strcmp(iOperation, "NativeObject") == 0)
        {
          RE_MOCK_ASSERT(iCount == 3);
          auto size = JBox_GetStringLength(iParams[2]);
          char buf[1024];
          JBox_GetSubstring(iParams[2], 0, size, buf);
          return new NativeObject(JBox_GetNumber(iParams[0]), JBox_GetBoolean(iParams[1]), buf);
        }

        return nullptr;
      };

      rt.destroy_native_object = [](const char iOperation[], void *iNativeObject) {
        RE_MOCK_LOG_INFO("rt.destroy_native_object(%s)", iOperation);
        if(std::strcmp(iOperation, "Instance") == 0)
        {
          delete reinterpret_cast<MockDevice *>(iNativeObject);
        }
        if(std::strcmp(iOperation, "NativeObject") == 0)
        {
          delete reinterpret_cast<NativeObject *>(iNativeObject);
        }
      };
    });

  auto re = rack.newDevice(c);

  auto o = re.getNativeObjectRO<NativeObject>("/custom_properties/native_object");
  ASSERT_FLOAT_EQ(12.98f, o->fNumberValue);
  ASSERT_TRUE(o->fBoolValue);
  ASSERT_EQ("abc", o->fStringValue);
}

// RackExtension.RealtimeController_NativeObject
TEST(RackExtension, RealtimeController_NativeObject)
{
  ASSERT_EQ(0, Gain::fCount);

  {
    Rack rack{};

    struct Device : public MockDevice
    {
      Device(int iSampleRate) : MockDevice(iSampleRate)
      {}

      void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override
      {
        for(int i = 0; i < iDiffCount; i++)
        {
          auto diff = iPropertyDiffs[i];
          switch(diff.fPropertyTag)
          {
            case 2000:
            {
              auto len = JBox_GetStringLength(diff.fCurrentValue);
              char tmp[100];
              RE_MOCK_ASSERT(len < 100);
              JBox_GetSubstring(diff.fCurrentValue, 0, len, tmp);
              fPropFunction = tmp;
              break;
            }
            case 3000:
            {
              auto gain = reinterpret_cast<Gain const *>(JBox_GetNativeObjectRO(diff.fCurrentValue));
              if(gain)
                fVolume = gain->fVolume;
              else
                fVolume = std::nullopt;
              break;
            }

            default:
              break;
          }
        }
      }

      std::string fPropFunction{};
      std::optional<TJBox_Float64> fVolume{};
    };

    auto c = DeviceConfig<Device>::fromSkeleton()
      .mdef(Config::document_owner_property("prop_function", lua::jbox_string_property{}.default_value("noop").property_tag(2000)))
      .mdef(Config::rtc_owner_property("prop_function_return", lua::jbox_string_property{}))
      .mdef(Config::rtc_owner_property("prop_gain", lua::jbox_native_object{}.property_tag(3000)))

      .rtc(resource::File{fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "lua",
                                "RackExtension_RealtimeController_NativeObject.lua")})
      .rt([](Realtime &rt) {
        rt.create_native_object = [](const char iOperation[], const TJBox_Value iParams[],
                                     TJBox_UInt32 iCount) -> void * {
          RE_MOCK_LOG_INFO("rt.create_native_object(%s)", iOperation);
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

        rt.destroy_native_object = [](const char iOperation[], void *iNativeObject) {
          RE_MOCK_LOG_INFO("rt.destroy_native_object(%s)", iOperation);
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

    auto re = rack.newDevice(c);

    // first batch (default to noop)
    ASSERT_EQ(0, Gain::fCount);
    ASSERT_EQ("noop -> void", re.getString("/custom_properties/prop_function_return"));
    rack.nextBatch();

    // trace
    re.setString("/custom_properties/prop_function", "trace");
    rack.nextBatch();
    ASSERT_EQ("trace -> void", re.getString("/custom_properties/prop_function_return"));

    // new_gain
    ASSERT_EQ(std::nullopt, re->fVolume);
    re.setString("/custom_properties/prop_function", "new_gain");
    rack.nextBatch();
    ASSERT_EQ(1, Gain::fCount);
    ASSERT_EQ("new_gain -> true", re.getString("/custom_properties/prop_function_return"));
    ASSERT_FLOAT_EQ(0.7, *re->fVolume);

    // nil_gain
    re.setString("/custom_properties/prop_function", "nil_gain");
    rack.nextBatch();
    ASSERT_EQ("nil_gain -> true", re.getString("/custom_properties/prop_function_return"));
    ASSERT_EQ(0, Gain::fCount);
    ASSERT_EQ(std::nullopt, re->fVolume);
  }

  ASSERT_EQ(0, Gain::fCount);
}

// RackExtension.RealtimeController_Blob
TEST(RackExtension, RealtimeController_Blob)
{
  Rack rack{};

  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate)
    {}

    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override
    {
      // clear the data first
      std::fill(std::begin(fBlobData), std::end(fBlobData), 0);
      fBlobInfo = std::nullopt;
      fBlobString = "";

      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        switch(diff.fPropertyTag)
        {
          case 2000:
          {
            auto len = JBox_GetStringLength(diff.fCurrentValue);
            char tmp[100];
            RE_MOCK_ASSERT(len < 100);
            JBox_GetSubstring(diff.fCurrentValue, 0, len, tmp);
            fPropFunction = tmp;
            break;
          }
          case 5000:
          {
            fBlobInfo = JBox_GetBLOBInfo(diff.fCurrentValue);
            fBlobString = JBox_toString(diff.fCurrentValue);
            if(fBlobInfo->fResidentSize > 0)
              JBox_GetBLOBData(diff.fCurrentValue, fBlobRange.first,
                               std::min(fBlobInfo->fResidentSize, fBlobRange.second), fBlobData);
          }

          default:
            break;
        }
      }

      // prop_blob_default is loaded with default value
      fDefaultBlobInfo = JBox_GetBLOBInfo(
        JBox_LoadMOMPropertyByTag(JBox_GetMotherboardObjectRef("/custom_properties"), 4000));
    }

    std::string fPropFunction{};
    TJBox_BLOBInfo fDefaultBlobInfo{};
    std::optional<TJBox_BLOBInfo> fBlobInfo{};
    std::string fBlobString{};
    std::pair<TJBox_SizeT, TJBox_SizeT> fBlobRange{0, 10};
    TJBox_UInt8 fBlobData[10];
  };

  std::vector<char> blobData{'A', 'L', '\0', 'z'};

  auto const blobFilePath = fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "audio", "sine.wav");

  auto c = DeviceConfig<Device>::fromSkeleton()
    .mdef(Config::document_owner_property("prop_function",
                                          lua::jbox_string_property{}.default_value("noop").property_tag(2000)))
    .mdef(Config::rtc_owner_property("prop_function_return", lua::jbox_string_property{}))
    .mdef(Config::rtc_owner_property("on_prop_blob_return", lua::jbox_string_property{}))
    .mdef(Config::rtc_owner_property("prop_blob_default",
                                     lua::jbox_blob_property{}.default_value("/Private/blob.default").property_tag(
                                       4000)))
    .mdef(Config::rtc_owner_property("prop_blob", lua::jbox_blob_property{}.property_tag(5000)))

    .blob_data("/Private/blob.default", to_chars("default"))
    .blob_data("/Private/blob.data", blobData)
    .blob_file("/Private/blob.file", blobFilePath)

    .resource_loading_context("/Private/blob.file", resource::LoadingContext{}.status(resource::LoadStatus::kPartiallyResident).resident_size(100))

    .rtc(resource::File{fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "lua",
                              "RackExtension_RealtimeController_Blob.lua")});

  auto re = rack.newDevice(c);

  // first batch (default to noop)
  ASSERT_EQ("noop -> void", re.getString("/custom_properties/prop_function_return"));
  rack.nextBatch();

  ASSERT_EQ(7, re->fDefaultBlobInfo.fResidentSize);
  ASSERT_EQ(7, re->fDefaultBlobInfo.fSize);

  ASSERT_EQ(0, re->fBlobInfo->fResidentSize);
  ASSERT_EQ(0, re->fBlobInfo->fSize);
  ASSERT_EQ("is_blob=true;size=0.0;resident_size=0.0;state=0.0", re.getString("/custom_properties/on_prop_blob_return"));
  ASSERT_EQ("Blob{.fSize=0,.fResidentSize=0,.fLoadStatus=nil,.fBlobPath=[]}", re->fBlobString);

  // loading all prop_blob_default (should do nothing...)
  re.loadMoreBlob("/custom_properties/prop_blob_default");
  rack.nextBatch();
  ASSERT_EQ(7, re->fDefaultBlobInfo.fResidentSize);
  ASSERT_EQ(7, re->fDefaultBlobInfo.fSize);
  ASSERT_EQ(std::nullopt, re->fBlobInfo);

  // load_blob_data
  re.setString("/custom_properties/prop_function", "load_blob_data");
  rack.nextBatch();
  ASSERT_EQ("load_blob_data -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(blobData.size(), re->fBlobInfo->fResidentSize);
  ASSERT_EQ(blobData.size(), re->fBlobInfo->fSize);
  ASSERT_EQ(std::vector<TJBox_UInt8>({'A', 'L', 0, 'z', 0, 0, 0, 0, 0, 0}),
            std::vector<TJBox_UInt8>(std::begin(re->fBlobData), std::end(re->fBlobData)));
  ASSERT_EQ("Blob{.fSize=4,.fResidentSize=4,.fLoadStatus=resident,.fBlobPath=[/Private/blob.data]}", re->fBlobString);
  rack.nextBatch();
  ASSERT_EQ("is_blob=true;size=4.0;resident_size=4.0;state=2.0", re.getString("/custom_properties/on_prop_blob_return"));

  // load_blob_file (100 bytes first)
  re.setString("/custom_properties/prop_function", "load_blob_file");
  rack.nextBatch();
  ASSERT_EQ("load_blob_file -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(100, re->fBlobInfo->fResidentSize);
  ASSERT_EQ(244, re->fBlobInfo->fSize);
  ASSERT_EQ(std::vector<TJBox_UInt8>({0x52, 0x49, 0x46, 0x46, 0xec, 0x00, 0x00, 0x00, 0x57, 0x41}),
            std::vector<TJBox_UInt8>(std::begin(re->fBlobData), std::end(re->fBlobData)));
  ASSERT_EQ("Blob{.fSize=244,.fResidentSize=100,.fLoadStatus=partially_resident,.fBlobPath=[/Private/blob.file]}", re->fBlobString);
  rack.nextBatch();
  ASSERT_EQ("is_blob=true;size=244.0;resident_size=100.0;state=1.0", re.getString("/custom_properties/on_prop_blob_return"));

  // load the rest
  re.loadMoreBlob("/custom_properties/prop_blob");
  re->fBlobRange = std::make_pair(200, 210);
  rack.nextBatch();
  ASSERT_EQ(244, re->fBlobInfo->fResidentSize);
  ASSERT_EQ(244, re->fBlobInfo->fSize);
  ASSERT_EQ(std::vector<TJBox_UInt8>({0x33, 0x9b, 0x8c, 0x9c, 0x3f, 0x9e, 0x5f, 0xa0, 0xd6, 0xa2}),
            std::vector<TJBox_UInt8>(std::begin(re->fBlobData), std::end(re->fBlobData)));
  ASSERT_EQ("Blob{.fSize=244,.fResidentSize=244,.fLoadStatus=resident,.fBlobPath=[/Private/blob.file]}", re->fBlobString);

  re->fBlobRange = std::make_pair(0, 10);

  // nil_blob
  re.setString("/custom_properties/prop_function", "nil_blob");
  rack.nextBatch();
  ASSERT_EQ("nil_blob -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(0, re->fBlobInfo->fResidentSize);
  ASSERT_EQ(0, re->fBlobInfo->fSize);
  ASSERT_EQ("Blob{.fSize=0,.fResidentSize=0,.fLoadStatus=nil,.fBlobPath=[]}", re->fBlobString);
  rack.nextBatch();
  ASSERT_EQ("is_blob=true;size=0.0;resident_size=0.0;state=0.0", re.getString("/custom_properties/on_prop_blob_return"));

  // make /Private/blob.data missing
  re.setResourceLoadingContext("/Private/blob.data", resource::LoadingContext{}.status(resource::LoadStatus::kMissing));

  // load_blob_data (missing)
  re.setString("/custom_properties/prop_function", "load_blob_data");
  rack.nextBatch();
  ASSERT_EQ("load_blob_data -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(0, re->fBlobInfo->fResidentSize);
  ASSERT_EQ(0, re->fBlobInfo->fSize);
  ASSERT_EQ("Blob{.fSize=0,.fResidentSize=0,.fLoadStatus=missing,.fBlobPath=[/Private/blob.data]}", re->fBlobString);
  rack.nextBatch();
  ASSERT_EQ("is_blob=true;size=0.0;resident_size=0.0;state=0.0", re.getString("/custom_properties/on_prop_blob_return"));

  // in error...
  ASSERT_THROW(re.loadMoreBlob("/custom_properties/prop_blob"), Exception);

  // make /Private/blob.data kHasErrors
  re.setResourceLoadingContext("/Private/blob.data", resource::LoadingContext{}.status(resource::LoadStatus::kHasErrors));

  // load_blob_data (missing)
  re.setString("/custom_properties/prop_function", "noop");
  re.setString("/custom_properties/prop_function", "load_blob_data");
  rack.nextBatch();
  ASSERT_EQ("load_blob_data -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(0, re->fBlobInfo->fResidentSize);
  ASSERT_EQ(0, re->fBlobInfo->fSize);
  ASSERT_EQ("Blob{.fSize=0,.fResidentSize=0,.fLoadStatus=has_errors,.fBlobPath=[/Private/blob.data]}", re->fBlobString);
  rack.nextBatch();
  ASSERT_EQ("is_blob=true;size=0.0;resident_size=0.0;state=0.0", re.getString("/custom_properties/on_prop_blob_return"));

  // in error...
  ASSERT_THROW(re.loadMoreBlob("/custom_properties/prop_blob"), Exception);

  // clear error
  re.clearResourceLoadingContext("/Private/blob.data");

  re.setString("/custom_properties/prop_function", "noop");
  re.setString("/custom_properties/prop_function", "load_blob_data");
  rack.nextBatch();
  ASSERT_EQ("load_blob_data -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(blobData.size(), re->fBlobInfo->fResidentSize);
  ASSERT_EQ(blobData.size(), re->fBlobInfo->fSize);
  ASSERT_EQ("Blob{.fSize=4,.fResidentSize=4,.fLoadStatus=resident,.fBlobPath=[/Private/blob.data]}", re->fBlobString);
  rack.nextBatch();
  ASSERT_EQ("is_blob=true;size=4.0;resident_size=4.0;state=2.0", re.getString("/custom_properties/on_prop_blob_return"));

  // this will throw an exception because /custom_properties/prop_blob is not a source for the binding /global_rtc/on_prop_function
  re.setString("/custom_properties/prop_function", "is_blob");
  ASSERT_THROW(rack.nextBatch(), Exception);
}

// RackExtension.RealtimeController_Sample
TEST(RackExtension, RealtimeController_Sample)
{
  Rack rack{};

  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate)
    {}

    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override
    {
      // clear the data first
      std::fill(std::begin(fSampleData), std::end(fSampleData), static_cast<TJBox_AudioSample>(0.0));
      fSampleInfo = std::nullopt;
      fSampleString = "";

      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        switch(diff.fPropertyTag)
        {
          case 2000:
          {
            auto len = JBox_GetStringLength(diff.fCurrentValue);
            char tmp[100];
            RE_MOCK_ASSERT(len < 100);
            JBox_GetSubstring(diff.fCurrentValue, 0, len, tmp);
            fPropFunction = tmp;
            break;
          }
          case 5000:
          {
            fSampleInfo = JBox_GetSampleInfo(diff.fCurrentValue);
            fSampleString = JBox_toString(diff.fCurrentValue);
            if(fSampleInfo->fResidentFrameCount > 0)
              JBox_GetSampleData(diff.fCurrentValue, fSampleRange.first,
                                 std::min(static_cast<TJBox_AudioFramePos>(fSampleInfo->fResidentFrameCount), fSampleRange.second), fSampleData);
          }

          default:
            break;
        }
      }

      // prop_sample_default is loaded with default value
      fDefaultSampleInfo = JBox_GetSampleInfo(
        JBox_LoadMOMPropertyByTag(JBox_GetMotherboardObjectRef("/custom_properties"), 4000));
    }

    std::string fPropFunction{};
    TJBox_SampleInfo fDefaultSampleInfo{};
    std::optional<TJBox_SampleInfo> fSampleInfo{};
    std::string fSampleString{};
    std::pair<TJBox_AudioFramePos, TJBox_AudioFramePos> fSampleRange{0, 10};
    TJBox_AudioSample fSampleData[10];
  };

  std::vector<TJBox_AudioSample> sampleStereoData{1,10,2,20,3,30,4,40,5,50};

  auto c = DeviceConfig<Device>::fromSkeleton()
    .mdef(Config::document_owner_property("prop_function",
                                          lua::jbox_string_property{}.default_value("noop").property_tag(2000)))
    .mdef(Config::rtc_owner_property("prop_function_return", lua::jbox_string_property{}))
    .mdef(Config::rtc_owner_property("on_prop_sample_return", lua::jbox_string_property{}))
    .mdef(Config::rtc_owner_property("prop_sample_default",
                                     lua::jbox_sample_property{}.default_value("/Private/sample.default").property_tag(
                                       4000)))
    .mdef(Config::rtc_owner_property("prop_sample", lua::jbox_sample_property{}.property_tag(5000)))

    .sample_data("/Private/sample.default", resource::Sample{}.sample_rate(44100).channels(1).data({0,1,2,3}))
    .sample_file("/Private/mono_sample.file", fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "audio",
                                                        "sine.wav"))
    .sample_data("/Private/stereo_sample.data", resource::Sample{}.sample_rate(44100).channels(2).data(sampleStereoData))

    .resource_loading_context("/Private/stereo_sample.data", resource::LoadingContext{}.status(resource::LoadStatus::kPartiallyResident).resident_size(2))

    .rtc(resource::File{fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "lua",
                              "RackExtension_RealtimeController_Sample.lua")});

  auto re = rack.newDevice(c);

  // first batch (default to noop)
  ASSERT_EQ("noop -> void", re.getString("/custom_properties/prop_function_return"));
  rack.nextBatch();

  ASSERT_EQ(1, re->fDefaultSampleInfo.fChannels);
  ASSERT_EQ(44100, re->fDefaultSampleInfo.fSampleRate);
  ASSERT_EQ(4, re->fDefaultSampleInfo.fResidentFrameCount);
  ASSERT_EQ(4, re->fDefaultSampleInfo.fFrameCount);

  ASSERT_EQ(1, re->fSampleInfo->fChannels);
  ASSERT_EQ(1, re->fSampleInfo->fSampleRate);
  ASSERT_EQ(0, re->fSampleInfo->fResidentFrameCount);
  ASSERT_EQ(0, re->fSampleInfo->fFrameCount);

  ASSERT_EQ("Sample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}",
            re->fSampleString);

  ASSERT_EQ("is_sample=true;frame_count=0.0;resident_count=0.0;channels=1.0;sample_rate=1.0;state=0.0",
            re.getString("/custom_properties/on_prop_sample_return"));

  // loading all prop_sample_default (should do nothing...)
  re.loadMoreSample("/custom_properties/prop_sample_default");
  rack.nextBatch();
  ASSERT_EQ(1, re->fDefaultSampleInfo.fChannels);
  ASSERT_EQ(44100, re->fDefaultSampleInfo.fSampleRate);
  ASSERT_EQ(4, re->fDefaultSampleInfo.fResidentFrameCount);
  ASSERT_EQ(4, re->fDefaultSampleInfo.fFrameCount);

  ASSERT_EQ(std::nullopt, re->fSampleInfo);

  // load_mono_sample_data
  re.setString("/custom_properties/prop_function", "load_mono_sample_file");
  rack.nextBatch();
  ASSERT_EQ("load_mono_sample_file -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(1, re->fSampleInfo->fChannels);
  ASSERT_EQ(44100, re->fSampleInfo->fSampleRate);
  ASSERT_EQ(100, re->fSampleInfo->fResidentFrameCount);
  ASSERT_EQ(100, re->fSampleInfo->fFrameCount);
  ASSERT_EQ("Sample{.fChannels=1,.fSampleRate=44100,.fFrameCount=100,.fResidentFrameCount=100,.fLoadStatus=resident,.fSamplePath=[/Private/mono_sample.file]}",
            re->fSampleString);
  rack.nextBatch();
  ASSERT_EQ("is_sample=true;frame_count=100.0;resident_count=100.0;channels=1.0;sample_rate=44100.0;state=2.0",
            re.getString("/custom_properties/on_prop_sample_return"));

  // load_stereo_sample_data (2 frames)
  re.setString("/custom_properties/prop_function", "load_stereo_sample_data");
  rack.nextBatch();
  ASSERT_EQ("load_stereo_sample_data -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(2, re->fSampleInfo->fChannels);
  ASSERT_EQ(44100, re->fSampleInfo->fSampleRate);
  ASSERT_EQ(2, re->fSampleInfo->fResidentFrameCount);
  ASSERT_EQ(sampleStereoData.size() / 2, re->fSampleInfo->fFrameCount);
  ASSERT_EQ(std::vector<TJBox_AudioSample>({1,10,2,20,0,0,0,0,0,0}),
            std::vector<TJBox_AudioSample>(std::begin(re->fSampleData), std::end(re->fSampleData)));
  ASSERT_EQ("Sample{.fChannels=2,.fSampleRate=44100,.fFrameCount=5,.fResidentFrameCount=2,.fLoadStatus=partially_resident,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSampleString);
  rack.nextBatch();
  ASSERT_EQ("is_sample=true;frame_count=5.0;resident_count=2.0;channels=2.0;sample_rate=44100.0;state=1.0",
            re.getString("/custom_properties/on_prop_sample_return"));

  // load the rest
  re.loadMoreSample("/custom_properties/prop_sample");
  re->fSampleRange = std::make_pair(2, 3);
  rack.nextBatch();
  ASSERT_EQ(2, re->fSampleInfo->fChannels);
  ASSERT_EQ(44100, re->fSampleInfo->fSampleRate);
  ASSERT_EQ(sampleStereoData.size() / 2, re->fSampleInfo->fResidentFrameCount);
  ASSERT_EQ(sampleStereoData.size() / 2, re->fSampleInfo->fFrameCount);
  ASSERT_EQ(std::vector<TJBox_AudioSample>({3,30,0,0,0,0,0,0,0,0}),
            std::vector<TJBox_AudioSample>(std::begin(re->fSampleData), std::end(re->fSampleData)));
  ASSERT_EQ("Sample{.fChannels=2,.fSampleRate=44100,.fFrameCount=5,.fResidentFrameCount=5,.fLoadStatus=resident,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSampleString);
  ASSERT_EQ("is_sample=true;frame_count=5.0;resident_count=5.0;channels=2.0;sample_rate=44100.0;state=2.0",
            re.getString("/custom_properties/on_prop_sample_return"));

  re->fSampleRange = std::make_pair(0, 10);

  // nil_sample
  re.setString("/custom_properties/prop_function", "nil_sample");
  rack.nextBatch();
  ASSERT_EQ("nil_sample -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(1, re->fSampleInfo->fChannels);
  ASSERT_EQ(1, re->fSampleInfo->fSampleRate);
  ASSERT_EQ(0, re->fSampleInfo->fResidentFrameCount);
  ASSERT_EQ(0, re->fSampleInfo->fFrameCount);
  ASSERT_EQ("Sample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}",
            re->fSampleString);
  rack.nextBatch();
  ASSERT_EQ("is_sample=true;frame_count=0.0;resident_count=0.0;channels=1.0;sample_rate=1.0;state=0.0",
            re.getString("/custom_properties/on_prop_sample_return"));

  // make /Private/sample.data missing
  re.setResourceLoadingContext("/Private/stereo_sample.data", resource::LoadingContext{}.status(resource::LoadStatus::kMissing));

  // load_sample_data (missing)
  re.setString("/custom_properties/prop_function", "noop"); // to force a change
  re.setString("/custom_properties/prop_function", "load_stereo_sample_data");
  rack.nextBatch();
  ASSERT_EQ("load_stereo_sample_data -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(1, re->fSampleInfo->fChannels);
  ASSERT_EQ(1, re->fSampleInfo->fSampleRate);
  ASSERT_EQ(0, re->fSampleInfo->fResidentFrameCount);
  ASSERT_EQ(0, re->fSampleInfo->fFrameCount);
  ASSERT_EQ("Sample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=missing,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSampleString);
  rack.nextBatch();
  ASSERT_EQ("is_sample=true;frame_count=0.0;resident_count=0.0;channels=1.0;sample_rate=1.0;state=0.0",
            re.getString("/custom_properties/on_prop_sample_return"));

  // in error...
  ASSERT_THROW(re.loadMoreSample("/custom_properties/prop_sample"), Exception);

  // make /Private/sample.data kHasErrors
  re.setResourceLoadingContext("/Private/stereo_sample.data", resource::LoadingContext{}.status(resource::LoadStatus::kHasErrors));

  // load_sample_data (missing)
  re.setString("/custom_properties/prop_function", "noop"); // to force a change
  re.setString("/custom_properties/prop_function", "load_stereo_sample_data");
  rack.nextBatch();
  ASSERT_EQ("load_stereo_sample_data -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(1, re->fSampleInfo->fChannels);
  ASSERT_EQ(1, re->fSampleInfo->fSampleRate);
  ASSERT_EQ(0, re->fSampleInfo->fResidentFrameCount);
  ASSERT_EQ(0, re->fSampleInfo->fFrameCount);
  ASSERT_EQ("Sample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=has_errors,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSampleString);
  rack.nextBatch();
  ASSERT_EQ("is_sample=true;frame_count=0.0;resident_count=0.0;channels=1.0;sample_rate=1.0;state=0.0",
            re.getString("/custom_properties/on_prop_sample_return"));

  // in error...
  ASSERT_THROW(re.loadMoreSample("/custom_properties/prop_sample"), Exception);


  // clear error
  re.clearResourceLoadingContext("/Private/stereo_sample.data");

  re.setString("/custom_properties/prop_function", "noop"); // to force a change
  re.setString("/custom_properties/prop_function", "load_stereo_sample_data");
  rack.nextBatch();
  ASSERT_EQ("load_stereo_sample_data -> true", re.getString("/custom_properties/prop_function_return"));
  ASSERT_EQ(2, re->fSampleInfo->fChannels);
  ASSERT_EQ(44100, re->fSampleInfo->fSampleRate);
  ASSERT_EQ(sampleStereoData.size() / 2, re->fSampleInfo->fResidentFrameCount);
  ASSERT_EQ(sampleStereoData.size() / 2, re->fSampleInfo->fFrameCount);
  ASSERT_EQ(sampleStereoData,
            std::vector<TJBox_AudioSample>(std::begin(re->fSampleData), std::end(re->fSampleData)));
  ASSERT_EQ("Sample{.fChannels=2,.fSampleRate=44100,.fFrameCount=5,.fResidentFrameCount=5,.fLoadStatus=resident,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSampleString);
  rack.nextBatch();
  ASSERT_EQ("is_sample=true;frame_count=5.0;resident_count=5.0;channels=2.0;sample_rate=44100.0;state=2.0",
            re.getString("/custom_properties/on_prop_sample_return"));
}

// RackExtension.RealtimeController_UserSample
TEST(RackExtension, RealtimeController_UserSample)
{
  Rack rack{};

  struct Device : public MockDevice
  {
    struct Sample
    {
      std::optional<TJBox_SampleMetaData> fMetadata{};

      std::optional<TJBox_Float64> root_key{};
      std::optional<TJBox_Float64> tune_cents{};
      std::optional<TJBox_Float64> play_range_start{};
      std::optional<TJBox_Float64> play_range_end{};
      std::optional<TJBox_Float64> loop_range_start{};
      std::optional<TJBox_Float64> loop_range_end{};
      std::optional<TJBox_Float64> loop_mode{};
      std::optional<TJBox_Float64> preview_volume_level{};

      std::string fJboxToString{};

      TJBox_AudioSample fData[10]{};

      std::string toString(std::optional<TJBox_Float64> v)
      {
        return v ? fmt::printf("%d", static_cast<int>(*v)) : "?";
      }

      std::string toString()
      {
        auto metadata = fMetadata ?
                        fmt::printf("m.fc=%d;m.rfc=%d;m.ch=%d;m.sr=%d;m.st=%d;m.rk=%d;m.tc=%d;m.prs=%d;m.pre=%d;m.lrs=%d;m.lre=%d;m.lm=%d;m.pvl=%d",
                                    fMetadata->fSpec.fFrameCount,
                                    fMetadata->fResidentFrameCount,
                                    fMetadata->fSpec.fChannels,
                                    fMetadata->fSpec.fSampleRate,
                                    fMetadata->fStatus,
                                    fMetadata->fParameters.fRootNote,
                                    fMetadata->fParameters.fTuneCents,
                                    fMetadata->fParameters.fPlayRangeStart,
                                    fMetadata->fParameters.fPlayRangeEnd,
                                    fMetadata->fParameters.fLoopRangeStart,
                                    fMetadata->fParameters.fLoopRangeEnd,
                                    fMetadata->fParameters.fLoopMode,
                                    fMetadata->fParameters.fVolumeLevel)
                                  : "m=?";
        return fmt::printf("%s;o.rk=%s;o.tc=%s;o.prs=%s;o.pre=%s;o.lrs=%s;o.lre=%s;o.lm=%s;o.pvl=%s",
                           metadata,
                           toString(root_key),
                           toString(tune_cents),
                           toString(play_range_start),
                           toString(play_range_end),
                           toString(loop_range_start),
                           toString(loop_range_end),
                           toString(loop_mode),
                           toString(preview_volume_level));
      }
    };

    Device(int iSampleRate) : MockDevice(iSampleRate)
    {
      fSample0Ref = JBox_GetMotherboardObjectRef("/user_samples/0");
      fSample1Ref = JBox_GetMotherboardObjectRef("/user_samples/1");
    }

    void handleDiff(TJBox_PropertyDiff const &iDiff, std::optional<Sample> &oSample)
    {
      if(oSample == std::nullopt)
        oSample = Sample{};

      switch(iDiff.fPropertyTag)
      {
        case kJBox_UserSampleItem:
        {
          auto info = JBox_GetSampleInfo(iDiff.fCurrentValue);
          oSample->fMetadata = JBox_GetSampleMetaData(iDiff.fCurrentValue);
          oSample->fJboxToString = JBox_toString(iDiff.fCurrentValue);
          RE_MOCK_ASSERT(info.fFrameCount == oSample->fMetadata->fSpec.fFrameCount &&
                         info.fResidentFrameCount == oSample->fMetadata->fResidentFrameCount &&
                         info.fChannels == oSample->fMetadata->fSpec.fChannels &&
                         info.fSampleRate == oSample->fMetadata->fSpec.fSampleRate);
          if(info.fResidentFrameCount > 0)
            JBox_GetSampleData(iDiff.fCurrentValue, fSampleRange.first,
                               std::min(static_cast<TJBox_AudioFramePos>(info.fResidentFrameCount), fSampleRange.second),
                               oSample->fData);
          break;
        }

        case kJBox_UserSampleRootKey:
          oSample->root_key = JBox_GetNumber(iDiff.fCurrentValue);
          break;
        case kJBox_UserSampleTuneCents:
          oSample->tune_cents = JBox_GetNumber(iDiff.fCurrentValue);
          break;
        case kJBox_UserSamplePlayRangeStart:
          oSample->play_range_start = JBox_GetNumber(iDiff.fCurrentValue);
          break;
        case kJBox_UserSamplePlayRangeEnd:
          oSample->play_range_end = JBox_GetNumber(iDiff.fCurrentValue);
          break;
        case kJBox_UserSampleLoopRangeStart:
          oSample->loop_range_start = JBox_GetNumber(iDiff.fCurrentValue);
          break;
        case kJBox_UserSampleLoopRangeEnd:
          oSample->loop_range_end = JBox_GetNumber(iDiff.fCurrentValue);
          break;
        case kJBox_UserSampleLoopMode:
          oSample->loop_mode = JBox_GetNumber(iDiff.fCurrentValue);
          break;
        case kJBox_UserSamplePreviewVolumeLevel:
          oSample->preview_volume_level = JBox_GetNumber(iDiff.fCurrentValue);
          break;
      }
    }

    void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override
    {
      // clear the data first
      fSample0 = std::nullopt;
      fSample1 = std::nullopt;

      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];

        if(diff.fPropertyRef.fObject == fSample0Ref)
          handleDiff(diff, fSample0);
        else if(diff.fPropertyRef.fObject == fSample1Ref)
          handleDiff(diff, fSample1);
      }

    }

    std::pair<TJBox_AudioFramePos, TJBox_AudioFramePos> fSampleRange{0, 10};
    TJBox_ObjectRef fSample0Ref{};
    std::optional<Sample> fSample0{};
    TJBox_ObjectRef fSample1Ref{};
    std::optional<Sample> fSample1{};
  };

  std::vector<TJBox_AudioSample> sampleMonoData{0,1,2,3,4,5};
  std::vector<TJBox_AudioSample> sampleStereoData{1,10,2,20,3,30,4,40,5,50};

  auto c = DeviceConfig<Device>::fromSkeleton()
    .mdef(Config::rtc_owner_property("on_prop_user_sample_return_0", lua::jbox_string_property{}))
    .mdef(Config::rtc_owner_property("on_prop_user_sample_return_1", lua::jbox_string_property{}))
    .mdef(Config::user_sample(0, lua::jbox_user_sample_property{}.all_sample_parameters()))
    .mdef(Config::user_sample(1, lua::jbox_user_sample_property{}.sample_parameter("tune_cents")))

    .sample_data("/Private/mono_sample.data", resource::Sample{}.sample_rate(44100).channels(1).data(sampleMonoData))
    .sample_data("/Private/stereo_sample.data", resource::Sample{}.sample_rate(44100).channels(2).data(sampleStereoData))

    .resource_loading_context("/Private/stereo_sample.data", resource::LoadingContext{}.status(resource::LoadStatus::kPartiallyResident).resident_size(2))

    .rtc(resource::File{fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "lua",
                              "RackExtension_RealtimeController_UserSample.lua")});

  auto re = rack.newDevice(c);

  // first batch
  rack.nextBatch();

  ASSERT_EQ("i=0;i.fc=0.0;i.rfc=0.0;i.ch=1.0;i.sr=1.0;i.st=0.0;"
            "m.fc=0.0;m.rfc=0.0;m.ch=1.0;m.sr=1.0;m.ls=nil;m.sn=[];"
            "m.rk=60.0;m.tc=50.0;m.prs=0.0;m.pre=1.0;m.lrs=0.0;m.lre=1.0;m.lm=loop_off;m.pvl=100.0",
            re.getString("/custom_properties/on_prop_user_sample_return_0"));
  ASSERT_EQ("i=1;i.fc=0.0;i.rfc=0.0;i.ch=1.0;i.sr=1.0;i.st=0.0;"
            "m.fc=0.0;m.rfc=0.0;m.ch=1.0;m.sr=1.0;m.ls=nil;m.sn=[];"
            "m.rk=60.0;m.tc=50.0;m.prs=0.0;m.pre=1.0;m.lrs=0.0;m.lre=1.0;m.lm=loop_off;m.pvl=100.0",
            re.getString("/custom_properties/on_prop_user_sample_return_1"));

  ASSERT_EQ("m.fc=0;m.rfc=0;m.ch=1;m.sr=1;m.st=0;"
            "m.rk=60;m.tc=50;m.prs=0;m.pre=1;m.lrs=0;m.lre=1;m.lm=0;m.pvl=100;"
            "o.rk=60;o.tc=50;o.prs=0;o.pre=1;o.lrs=0;o.lre=1;o.lm=0;o.pvl=100",
            re->fSample0->toString());
  ASSERT_EQ("UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}",
            re->fSample0->fJboxToString);
  ASSERT_EQ("m.fc=0;m.rfc=0;m.ch=1;m.sr=1;m.st=0;"
            "m.rk=60;m.tc=50;m.prs=0;m.pre=1;m.lrs=0;m.lre=1;m.lm=0;m.pvl=100;"
            "o.rk=?;o.tc=50;o.prs=?;o.pre=?;o.lrs=?;o.lre=?;o.lm=?;o.pvl=?",
            re->fSample1->toString());
  ASSERT_EQ("UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}",
            re->fSample1->fJboxToString);

  // we load sample 0 (the "current" sample)
  re.loadCurrentUserSampleAsync("/Private/mono_sample.data");

  rack.nextBatch();

  ASSERT_EQ("i=0;i.fc=6.0;i.rfc=6.0;i.ch=1.0;i.sr=44100.0;i.st=2.0;"
            "m.fc=6.0;m.rfc=6.0;m.ch=1.0;m.sr=44100.0;m.ls=resident;m.sn=[mono_sample.data];"
            "m.rk=60.0;m.tc=50.0;m.prs=0.0;m.pre=1.0;m.lrs=0.0;m.lre=1.0;m.lm=loop_off;m.pvl=100.0",
            re.getString("/custom_properties/on_prop_user_sample_return_0"));
  ASSERT_EQ("m.fc=6;m.rfc=6;m.ch=1;m.sr=44100;m.st=2;"
            "m.rk=60;m.tc=50;m.prs=0;m.pre=1;m.lrs=0;m.lre=1;m.lm=0;m.pvl=100;"
            "o.rk=?;o.tc=?;o.prs=?;o.pre=?;o.lrs=?;o.lre=?;o.lm=?;o.pvl=?",
            re->fSample0->toString());
  ASSERT_EQ("UserSample{.fChannels=1,.fSampleRate=44100,.fFrameCount=6,.fResidentFrameCount=6,.fLoadStatus=resident,.fSamplePath=[/Private/mono_sample.data]}",
            re->fSample0->fJboxToString);
  ASSERT_EQ(std::nullopt, re->fSample1);

  // we load sample 1 (the "current" sample)
  re.selectCurrentUserSample(1);
  re.loadCurrentUserSampleAsync("/Private/stereo_sample.data");

  rack.nextBatch();

  ASSERT_EQ("i=1;i.fc=5.0;i.rfc=2.0;i.ch=2.0;i.sr=44100.0;i.st=1.0;"
            "m.fc=5.0;m.rfc=2.0;m.ch=2.0;m.sr=44100.0;m.ls=partially_resident;m.sn=[stereo_sample.data];"
            "m.rk=60.0;m.tc=50.0;m.prs=0.0;m.pre=1.0;m.lrs=0.0;m.lre=1.0;m.lm=loop_off;m.pvl=100.0",
            re.getString("/custom_properties/on_prop_user_sample_return_1"));
  ASSERT_EQ(std::nullopt, re->fSample0);
  ASSERT_EQ("m.fc=5;m.rfc=2;m.ch=2;m.sr=44100;m.st=1;"
            "m.rk=60;m.tc=50;m.prs=0;m.pre=1;m.lrs=0;m.lre=1;m.lm=0;m.pvl=100;"
            "o.rk=?;o.tc=?;o.prs=?;o.pre=?;o.lrs=?;o.lre=?;o.lm=?;o.pvl=?",
            re->fSample1->toString());
  ASSERT_EQ("UserSample{.fChannels=2,.fSampleRate=44100,.fFrameCount=5,.fResidentFrameCount=2,.fLoadStatus=partially_resident,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSample1->fJboxToString);

  // we finish loading it (and change tune_cents)
  re.loadMoreSample("/user_samples/1/item");
  re.setNum("/user_samples/1/tune_cents", 45);

  rack.nextBatch();

  ASSERT_EQ("i=1;i.fc=5.0;i.rfc=5.0;i.ch=2.0;i.sr=44100.0;i.st=2.0;"
            "m.fc=5.0;m.rfc=5.0;m.ch=2.0;m.sr=44100.0;m.ls=resident;m.sn=[stereo_sample.data];"
            "m.rk=60.0;m.tc=45.0;m.prs=0.0;m.pre=1.0;m.lrs=0.0;m.lre=1.0;m.lm=loop_off;m.pvl=100.0",
            re.getString("/custom_properties/on_prop_user_sample_return_1"));
  ASSERT_EQ(std::nullopt, re->fSample0);
  ASSERT_EQ("m.fc=5;m.rfc=5;m.ch=2;m.sr=44100;m.st=2;"
            "m.rk=60;m.tc=45;m.prs=0;m.pre=1;m.lrs=0;m.lre=1;m.lm=0;m.pvl=100;"
            "o.rk=?;o.tc=45;o.prs=?;o.pre=?;o.lrs=?;o.lre=?;o.lm=?;o.pvl=?",
            re->fSample1->toString());
  ASSERT_EQ("UserSample{.fChannels=2,.fSampleRate=44100,.fFrameCount=5,.fResidentFrameCount=5,.fLoadStatus=resident,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSample1->fJboxToString);

  // we delete sample 1
  re.deleteUserSample(1);

  rack.nextBatch();

  ASSERT_EQ("i=1;i.fc=0.0;i.rfc=0.0;i.ch=1.0;i.sr=1.0;i.st=0.0;"
            "m.fc=0.0;m.rfc=0.0;m.ch=1.0;m.sr=1.0;m.ls=nil;m.sn=[];"
            "m.rk=60.0;m.tc=45.0;m.prs=0.0;m.pre=1.0;m.lrs=0.0;m.lre=1.0;m.lm=loop_off;m.pvl=100.0",
            re.getString("/custom_properties/on_prop_user_sample_return_1"));
  ASSERT_EQ(std::nullopt, re->fSample0);
  ASSERT_EQ("m.fc=0;m.rfc=0;m.ch=1;m.sr=1;m.st=0;"
            "m.rk=60;m.tc=45;m.prs=0;m.pre=1;m.lrs=0;m.lre=1;m.lm=0;m.pvl=100;"
            "o.rk=?;o.tc=?;o.prs=?;o.pre=?;o.lrs=?;o.lre=?;o.lm=?;o.pvl=?",
            re->fSample1->toString());
  ASSERT_EQ("UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}",
            re->fSample1->fJboxToString);

  // we load sample 1 again but with a missing item
  re.loadUserSampleAsync("/user_samples/1/item", "/Private/stereo_sample.data", resource::LoadingContext{}.status(resource::LoadStatus::kMissing));

  rack.nextBatch();

  ASSERT_EQ("i=1;i.fc=0.0;i.rfc=0.0;i.ch=1.0;i.sr=1.0;i.st=0.0;"
            "m.fc=0.0;m.rfc=0.0;m.ch=1.0;m.sr=1.0;m.ls=missing;m.sn=[stereo_sample.data];"
            "m.rk=60.0;m.tc=45.0;m.prs=0.0;m.pre=1.0;m.lrs=0.0;m.lre=1.0;m.lm=loop_off;m.pvl=100.0",
            re.getString("/custom_properties/on_prop_user_sample_return_1"));
  ASSERT_EQ(std::nullopt, re->fSample0);
  ASSERT_EQ("m.fc=0;m.rfc=0;m.ch=1;m.sr=1;m.st=0;"
            "m.rk=60;m.tc=45;m.prs=0;m.pre=1;m.lrs=0;m.lre=1;m.lm=0;m.pvl=100;"
            "o.rk=?;o.tc=?;o.prs=?;o.pre=?;o.lrs=?;o.lre=?;o.lm=?;o.pvl=?",
            re->fSample1->toString());
  ASSERT_EQ("UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=missing,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSample1->fJboxToString);

  // we load sample 1 again but with an error item
  re.loadUserSampleAsync("/user_samples/1/item", "/Private/stereo_sample.data", resource::LoadingContext{}.status(resource::LoadStatus::kHasErrors));

  rack.nextBatch();

  ASSERT_EQ("i=1;i.fc=0.0;i.rfc=0.0;i.ch=1.0;i.sr=1.0;i.st=0.0;"
            "m.fc=0.0;m.rfc=0.0;m.ch=1.0;m.sr=1.0;m.ls=has_errors;m.sn=[stereo_sample.data];"
            "m.rk=60.0;m.tc=45.0;m.prs=0.0;m.pre=1.0;m.lrs=0.0;m.lre=1.0;m.lm=loop_off;m.pvl=100.0",
            re.getString("/custom_properties/on_prop_user_sample_return_1"));
  ASSERT_EQ(std::nullopt, re->fSample0);
  ASSERT_EQ("m.fc=0;m.rfc=0;m.ch=1;m.sr=1;m.st=0;"
            "m.rk=60;m.tc=45;m.prs=0;m.pre=1;m.lrs=0;m.lre=1;m.lm=0;m.pvl=100;"
            "o.rk=?;o.tc=?;o.prs=?;o.pre=?;o.lrs=?;o.lre=?;o.lm=?;o.pvl=?",
            re->fSample1->toString());
  ASSERT_EQ("UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=has_errors,.fSamplePath=[/Private/stereo_sample.data]}",
            re->fSample1->fJboxToString);
}


}