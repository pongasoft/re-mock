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

#include <re/mock/DeviceTesters.h>
#include <gtest/gtest.h>

namespace re::mock::Test {

using namespace mock;

// Misc.RequestReset
TEST(Misc, RequestReset)
{
  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      auto transportRef = JBox_GetMotherboardObjectRef("/transport");
      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        if(diff.fPropertyRef.fObject == transportRef)
        {
          switch(diff.fPropertyTag)
          {
            case kJBox_TransportRequestResetAudio:
              fRequestResetAudio = static_cast<int>(JBox_GetNumber(diff.fCurrentValue));
              break;

            case kJBox_TransportRequestStop:
              fRequestStop = static_cast<int>(JBox_GetNumber(diff.fCurrentValue));
              break;

            default:
              break;
          }
        }
      }
    }

    int fRequestResetAudio{};
    int fRequestStop{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .rtc(Config::rt_input_setup_notify("/transport/request_reset_audio"))
    .rtc(Config::rt_input_setup_notify("/transport/request_stop"));

  auto tester = HelperTester<Device>(c);

  // first batch
  tester.nextBatch();

  // store values for requestResetAudio and requestResetStop
  auto requestResetAudio = tester.device()->fRequestResetAudio;
  auto requestResetStop = tester.device()->fRequestStop;

  // next batch => nothing should have changed
  tester.nextBatch();
  ASSERT_EQ(requestResetAudio, tester.device()->fRequestResetAudio);
  ASSERT_EQ(requestResetStop, tester.device()->fRequestStop);

  // requestResetAudio / next batch => requestResetAudio should have changed
  tester.device().requestResetAudio();
  tester.nextBatch();

  ASSERT_NE(requestResetAudio, tester.device()->fRequestResetAudio);
  ASSERT_EQ(requestResetStop, tester.device()->fRequestStop);

  requestResetAudio = tester.device()->fRequestResetAudio;

  // requestStop / next batch => requestStop should have changed
  tester.device().requestStop();
  tester.nextBatch();
  ASSERT_EQ(requestResetAudio, tester.device()->fRequestResetAudio);
  ASSERT_NE(requestResetStop, tester.device()->fRequestStop);

  requestResetStop = tester.device()->fRequestStop;

  // next batch => nothing should have changed
  tester.nextBatch();
  ASSERT_EQ(requestResetAudio, tester.device()->fRequestResetAudio);
  ASSERT_EQ(requestResetStop, tester.device()->fRequestStop);

}

}