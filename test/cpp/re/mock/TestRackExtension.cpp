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
    .mdef(Config::document_owner_property("prop_int", lua::jbox_number_property{}.default_value(4)))
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

      rt.destroy_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount, void *iNativeObject) {
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

  rack.nextFrame();

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
    // after nextFrame input buffers are cleared (they have been consumed)
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
  re.setNum<int>("/custom_properties/prop_int", 104);
  ASSERT_EQ(104, re.getNum<int>("/custom_properties/prop_int"));
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
}


// RackExtension.RealtimeController
TEST(RackExtension, RealtimeController)
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

        // clear the data first
        std::fill(std::begin(fBlobData), std::end(fBlobData), 0);

        // prop_blob_default is loaded with default value
        fDefaultBlobInfo = JBox_GetBLOBInfo(JBox_LoadMOMPropertyByTag(JBox_GetMotherboardObjectRef("/custom_properties"), 4000));

        auto blobValue = JBox_LoadMOMPropertyByTag(JBox_GetMotherboardObjectRef("/custom_properties"), 5000);
        if(JBox_GetType(blobValue) == kJBox_Nil)
        {
          fBlobInfo = std::nullopt;
        }
        else
        {
          fBlobInfo = JBox_GetBLOBInfo(blobValue);

          if(fPropFunction == "load_blob_data")
          {
            JBox_GetBLOBData(blobValue, 0, std::min(fBlobInfo->fResidentSize, 10UL), fBlobData);
          }

          if(fPropFunction == "load_blob_file")
          {
            if(fBlobInfo->fResidentSize < 1100)
              JBox_GetBLOBData(blobValue, 0, std::min(fBlobInfo->fResidentSize, 10UL), fBlobData);
            else
              JBox_GetBLOBData(blobValue, 1000, 1010, fBlobData);
          }
        }
      }

      std::string fPropFunction{};
      std::optional<TJBox_Float64> fVolume{};
      TJBox_BLOBInfo fDefaultBlobInfo{};
      std::optional<TJBox_BLOBInfo> fBlobInfo{};
      TJBox_UInt8 fBlobData[10];
    };

    std::vector<char> blobData{'A', 'L', '\0', 'z'};

    auto c = DeviceConfig<Device>::fromSkeleton()
      .mdef(Config::document_owner_property("prop_function", lua::jbox_string_property{}.default_value("noop").property_tag(2000)))
      .mdef(Config::rtc_owner_property("prop_function_return", lua::jbox_string_property{}))
      .mdef(Config::rtc_owner_property("prop_gain", lua::jbox_native_object{}.property_tag(3000)))
      .mdef(Config::rtc_owner_property("prop_blob_default", lua::jbox_blob_property{}.default_value("/Private/blob.default").property_tag(4000)))
      .mdef(Config::rtc_owner_property("prop_blob", lua::jbox_blob_property{}.property_tag(5000)))

      .blob_data("/Private/blob.default", std::vector<char>{'d', 'e', 'f', 'a', 'u', 'l', 't' })
      .blob_data("/Private/blob.data", blobData)
      .blob_file("/Private/blob.file", fmt::path(RE_MOCK_PROJECT_DIR, "LICENSE.txt"))

      .rtc(ConfigFile{fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "lua",
                                "function-test-realtime_controller.lua")})
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

        rt.destroy_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount,
                                      void *iNativeObject) {
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

    // first frame (default to noop)
    ASSERT_EQ(0, Gain::fCount);
    ASSERT_EQ("noop -> void", re.getString("/custom_properties/prop_function_return"));
    rack.nextFrame();

    // trace
    re.setString("/custom_properties/prop_function", "trace");
    ASSERT_EQ("trace -> void", re.getString("/custom_properties/prop_function_return"));
    rack.nextFrame();

    // new_gain
    ASSERT_EQ(std::nullopt, re->fVolume);
    re.setString("/custom_properties/prop_function", "new_gain");
    ASSERT_EQ(1, Gain::fCount);
    ASSERT_EQ("new_gain -> true", re.getString("/custom_properties/prop_function_return"));
    rack.nextFrame();
    ASSERT_FLOAT_EQ(0.7, *re->fVolume);

    // nil_gain
    re.setString("/custom_properties/prop_function", "nil_gain");
    ASSERT_EQ("nil_gain -> true", re.getString("/custom_properties/prop_function_return"));
    rack.nextFrame();
    ASSERT_EQ(0, Gain::fCount);
    ASSERT_EQ(std::nullopt, re->fVolume);

    //------------------------------------------------------------------------
    // blobs
    //------------------------------------------------------------------------
    ASSERT_EQ(0, re->fDefaultBlobInfo.fResidentSize);
    ASSERT_EQ(7, re->fDefaultBlobInfo.fSize);
    ASSERT_EQ(std::nullopt, re->fBlobInfo);

    // loading all prop_blob_default
    re.loadMoreBlob("/custom_properties/prop_blob_default");
    rack.nextFrame();
    ASSERT_EQ(7, re->fDefaultBlobInfo.fResidentSize);
    ASSERT_EQ(7, re->fDefaultBlobInfo.fSize);
    ASSERT_EQ(std::nullopt, re->fBlobInfo);

    // load_blob_data
    re.setString("/custom_properties/prop_function", "load_blob_data");
    ASSERT_EQ("load_blob_data -> true", re.getString("/custom_properties/prop_function_return"));
    rack.nextFrame();
    ASSERT_EQ(0, re->fBlobInfo->fResidentSize);
    ASSERT_EQ(blobData.size(), re->fBlobInfo->fSize);

    re.loadMoreBlob("/custom_properties/prop_blob");
    rack.nextFrame();
    ASSERT_EQ(blobData.size(), re->fBlobInfo->fResidentSize);
    ASSERT_EQ(blobData.size(), re->fBlobInfo->fSize);
    ASSERT_EQ(std::vector<TJBox_UInt8>({'A', 'L', 0, 'z', 0, 0, 0, 0, 0, 0}),
              std::vector<TJBox_UInt8>(std::begin(re->fBlobData), std::end(re->fBlobData)));

    // load_blob_file
    re.setString("/custom_properties/prop_function", "load_blob_file");
    ASSERT_EQ("load_blob_file -> true", re.getString("/custom_properties/prop_function_return"));
    rack.nextFrame();
    ASSERT_EQ(0, re->fBlobInfo->fResidentSize);
    ASSERT_EQ(13600, re->fBlobInfo->fSize);

    re.loadMoreBlob("/custom_properties/prop_blob");
    rack.nextFrame();
    ASSERT_EQ(13600, re->fBlobInfo->fResidentSize);
    ASSERT_EQ(13600, re->fBlobInfo->fSize);
    ASSERT_EQ(std::vector<TJBox_UInt8>({' ', 's', 'h', 'a', 'r', 'e', 's', ',', ' ', 'o'}),
              std::vector<TJBox_UInt8>(std::begin(re->fBlobData), std::end(re->fBlobData)));

    // nil_blob
    re.setString("/custom_properties/prop_function", "nil_blob");
    ASSERT_EQ("nil_blob -> true", re.getString("/custom_properties/prop_function_return"));
    rack.nextFrame();
    ASSERT_EQ(std::nullopt, re->fBlobInfo);
  }

  ASSERT_EQ(0, Gain::fCount);
}


}