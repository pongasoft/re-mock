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

#include <re/mock/MockDevices.h>
#include <gtest/gtest.h>

namespace re::mock::Test {

using namespace mock;

// MockDevices.Sample
TEST(MockDevices, Sample)
{
  using sample = MockAudioDevice::Sample;

  auto s = MockAudioDevice::Sample::from(MockAudioDevice::StereoBuffer{}.fill(1.0, 2.0), 44100);

  ASSERT_TRUE(s.isStereo());
  ASSERT_EQ(44100, s.getSampleRate());
  ASSERT_FLOAT_EQ(MockAudioDevice::NUM_SAMPLES_PER_FRAME * 1000.0 / 44100.0, s.getDurationMilliseconds());
  ASSERT_EQ(MockAudioDevice::NUM_SAMPLES_PER_FRAME, s.getFrameCount());
  ASSERT_EQ(MockAudioDevice::NUM_SAMPLES_PER_FRAME * 2, s.getSampleCount());

  {
    auto channel = s.getLeftChannelSample();
    ASSERT_EQ(44100, channel.getSampleRate());
    ASSERT_TRUE(channel.isMono());
    ASSERT_EQ(MockAudioDevice::NUM_SAMPLES_PER_FRAME, channel.getFrameCount());
    ASSERT_EQ(MockAudioDevice::NUM_SAMPLES_PER_FRAME, channel.getSampleCount());
    ASSERT_EQ(std::vector<TJBox_AudioSample>(MockAudioDevice::NUM_SAMPLES_PER_FRAME, 1.0), channel.getData());
  }

  {
    auto channel = s.getRightChannelSample();
    ASSERT_EQ(44100, channel.getSampleRate());
    ASSERT_TRUE(channel.isMono());
    ASSERT_EQ(MockAudioDevice::NUM_SAMPLES_PER_FRAME, channel.getFrameCount());
    ASSERT_EQ(MockAudioDevice::NUM_SAMPLES_PER_FRAME, channel.getSampleCount());
    ASSERT_EQ(std::vector<TJBox_AudioSample>(MockAudioDevice::NUM_SAMPLES_PER_FRAME, 2.0), channel.getData());
  }

  s = MockAudioDevice::Sample{2, 44100};
  for(int i = 0; i < 10; i++)
    s.fData.emplace_back(i);

  ASSERT_EQ(s, s.subSample());
  ASSERT_EQ(s, s.clone());

  ASSERT_EQ(5, s.getFrameCount());
  ASSERT_EQ(10, s.getSampleCount());

  auto sub = s.subSample(1, 3); // [1, 2, 3]
  ASSERT_EQ(44100, sub.getSampleRate());
  ASSERT_TRUE(sub.isStereo());
  ASSERT_EQ(3, sub.getFrameCount());
  ASSERT_EQ(6, sub.getSampleCount());
  ASSERT_EQ(std::vector<TJBox_AudioSample>({2, 3, 4, 5, 6, 7}), sub.getData());

  sub.applyGain(2.0);
  ASSERT_EQ(44100, sub.getSampleRate());
  ASSERT_TRUE(sub.isStereo());
  ASSERT_EQ(3, sub.getFrameCount());
  ASSERT_EQ(6, sub.getSampleCount());
  ASSERT_EQ(std::vector<TJBox_AudioSample>({4, 6, 8, 10, 12, 14}), sub.getData());

  sub.mixWith(s); // [4, 6, 8, 10, 12, 14] +
  //                 [0, 1, 2,  3,  4,  5, 6, 7, 8, 9]
  ASSERT_EQ(5, sub.getFrameCount());
  ASSERT_EQ(10, sub.getSampleCount());
  ASSERT_EQ(std::vector<TJBox_AudioSample>({4, 7, 10, 13, 16, 19, 6, 7, 8, 9}), sub.getData());

}

}