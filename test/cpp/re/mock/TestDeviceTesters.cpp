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
#include <re/mock/FileManager.h>
#include <gtest/gtest.h>
#include <re_mock_build.h>

namespace re::mock::Test {

using namespace mock;

// StudioEffectTester.Usage
TEST(StudioEffectTester, Usage)
{
  StudioEffectTester<MAUPst> tester(MAUPst::CONFIG);

  // should be enabled by default
  ASSERT_EQ(kJBox_EnabledOn, tester.getBypassState());

  // device is not wired yet!
  ASSERT_EQ(tester.nextFrame(MockAudioDevice::buffer(1.0, 2.0)), MockAudioDevice::buffer(0, 0));

  // wire main inputs
  tester.wireMainIn(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);

  // device not fully wired yet (only IN is wired)
  ASSERT_EQ(tester.nextFrame(MockAudioDevice::buffer(3.0, 4.0)), MockAudioDevice::buffer(0, 0));

  // that means the internal buffer of the effect gets the input, but does not send it to the output
  ASSERT_EQ(tester.device()->fBuffer, MockAudioDevice::buffer(3.0, 4.0));

  // wire main outputs
  tester.wireMainOut(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);

  // device is fully wired
  ASSERT_EQ(tester.nextFrame(MockAudioDevice::buffer(5.0, 6.0)), MockAudioDevice::buffer(5.0, 6.0));

  // change to off
  tester.setBypassState(kJBox_EnabledOff);
  ASSERT_EQ(kJBox_EnabledOff, tester.getBypassState());

  // change to bypass
  tester.setBypassState(kJBox_EnabledBypass);
  ASSERT_EQ(kJBox_EnabledBypass, tester.getBypassState());
}

// StudioEffectTester.Sample
TEST(StudioEffectTester, Sample)
{
  StudioEffectTester<MAUPst> tester(MAUPst::CONFIG);
  tester.wireMainIn(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);
  tester.wireMainOut(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);

  auto sinePath = fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "audio", "sine.wav");
  auto sine = FileManager::loadSample(ConfigFile{sinePath});

  {
    auto processedSine = tester.processSample(ConfigFile{sinePath});

    // processedSine should be sine (since MAUPst is pass through)
    ASSERT_EQ(sine->fChannels, processedSine.fChannels);
    ASSERT_EQ(sine->fSampleRate, tester.rack().getSampleRate());
    ASSERT_EQ(sine->fData, processedSine.fData);
  }

  {
    auto processedSine = tester.processSample(ConfigFile{sinePath}, Duration::SampleFrames{30});
    auto expectedSine = sine;
    for(int i = 0; i < 30; i++)
      expectedSine->fData.emplace_back(0);
    ASSERT_EQ(expectedSine, processedSine);
  }

  {
    auto processedSine = tester.processSample(ConfigFile{sinePath}, Duration::Time{1});
    // 1ms at 44100 is 44.1 samples => 45
    auto expectedSine = sine;
    for(int i = 0; i < 45; i++)
      expectedSine->fData.emplace_back(0);
    ASSERT_EQ(expectedSine, processedSine);
  }
}

// Duration.Conversion
TEST(Duration, Conversion)
{
  ASSERT_EQ(45, Duration::toSampleFrames(Duration::Time{1}, 44100).fCount);
  ASSERT_EQ(4410, Duration::toSampleFrames(Duration::Time{100}, 44100).fCount);
  ASSERT_EQ(100*64, Duration::toSampleFrames(Duration::RackFrames{100}, 44100).fCount);
  ASSERT_EQ(100, Duration::toSampleFrames(Duration::SampleFrames{100}, 44100).fCount);

  ASSERT_EQ(1, Duration::toRackFrames(Duration::Time{1}, 44100).fCount);
  ASSERT_EQ(69, Duration::toRackFrames(Duration::Time{100}, 44100).fCount);
  ASSERT_EQ(100, Duration::toRackFrames(Duration::RackFrames{100}, 44100).fCount);
  ASSERT_EQ(2, Duration::toRackFrames(Duration::SampleFrames{100}, 44100).fCount);
}

// InstrumentTester.Usage
TEST(InstrumentTester, Usage)
{
  // the MAUSrc configuration defines it as a helper, so we need to redefine it as an instrument
  auto config = DeviceConfig<MAUSrc>::fromSkeleton(DeviceType::kInstrument).mdef(Config::stereo_audio_out());

  InstrumentTester<MAUSrc> tester(config);

  tester.device()->fBuffer = MockAudioDevice::buffer(1.0, 2.0);

  // device is not wired yet!
  ASSERT_EQ(tester.nextFrame(), MockAudioDevice::buffer(0, 0));

  // wire main outputs
  tester.wireMainOut(MAUSrc::LEFT_SOCKET, MAUSrc::RIGHT_SOCKET);

  tester.device()->fBuffer = MockAudioDevice::buffer(3.0, 4.0);

  // device is fully wired
  ASSERT_EQ(tester.nextFrame(), MockAudioDevice::buffer(3.0, 4.0));
}

// NotePlayerTester.Usage
TEST(NotePlayerTester, Usage)
{
  NotePlayerTester<MNPPst> tester(MNPPst::CONFIG);

  // should be enabled by default
  ASSERT_FALSE(tester.isBypassed());

  ASSERT_EQ(MockDevice::NoteEvents{}, tester.device()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.allNotesOff(), tester.nextFrame());
  ASSERT_EQ(MockDevice::NoteEvents{}.allNotesOff(), tester.device()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(69), tester.nextFrame(MockNotePlayer::NoteEvents{}.noteOn(69)));
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(69), tester.device()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOff(69, 25), tester.nextFrame(MockNotePlayer::NoteEvents{}.noteOff(69, 25)));
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOff(69, 25), tester.device()->fNoteEvents);

  // simulating sequencer note
  tester.device().use([](Motherboard &m) {
    m.setNoteInEvent(70, 120, 3);
  });

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(70, 120, 3), tester.nextFrame());

  // bypass note player (does not receive notes anymore)
  tester.setBypassed(true);
  ASSERT_EQ(MockDevice::NoteEvents{}, tester.nextFrame(MockNotePlayer::NoteEvents{}.noteOn(69)));

  // revert
  tester.setBypassed(false);
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(69), tester.nextFrame(MockNotePlayer::NoteEvents{}.noteOn(69)));

  // unwire src
  tester.unwire(tester.src());
  ASSERT_EQ(MockDevice::NoteEvents{}, tester.nextFrame(MockNotePlayer::NoteEvents{}.noteOn(69)));
}

// HelperTester.Usage
TEST(HelperTester, Usage)
{
  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      fFrameCount++;
    }

    int fFrameCount{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .mdef(Config::stereo_audio_in("aui_left", "aui_right"))
    .mdef(Config::stereo_audio_out("auo_left", "auo_right"))
    .mdef(Config::cv_in("cvi"))
    .mdef(Config::cv_out("cvo"));

  auto tester = HelperTester<Device>(c);

  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_right/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_outputs/auo_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_outputs/auo_right/connected"));
  ASSERT_FALSE(tester.device().getBool("/cv_inputs/cvi/connected"));
  ASSERT_FALSE(tester.device().getBool("/cv_outputs/cvo/connected"));

  auto audioSrc = tester.wireNewAudioSrc("aui_left", "aui_right");
  auto audioDst = tester.wireNewAudioDst("auo_left", "auo_right");
  auto cvSrc    = tester.wireNewCVSrc("cvi");
  auto cvDst    = tester.wireNewCVDst("cvo");

  ASSERT_TRUE(tester.device().getBool("/audio_inputs/aui_left/connected"));
  ASSERT_TRUE(tester.device().getBool("/audio_inputs/aui_right/connected"));
  ASSERT_TRUE(tester.device().getBool("/audio_outputs/auo_left/connected"));
  ASSERT_TRUE(tester.device().getBool("/audio_outputs/auo_right/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_inputs/cvi/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_outputs/cvo/connected"));

  ASSERT_EQ(0, tester.device()->fFrameCount);

  audioSrc->fBuffer = MockAudioDevice::buffer(0.5, 0.6);
  cvSrc->fValue = 1.0;

  // make sure it runs
  tester.nextFrame();

  ASSERT_EQ(1, tester.device()->fFrameCount);

  // unchanged
  ASSERT_EQ(MockAudioDevice::StereoBuffer{}, audioDst->fBuffer);
  ASSERT_EQ(0, cvDst->fValue);

  ASSERT_TRUE(tester.device().getBool("/audio_inputs/aui_left/connected"));
  ASSERT_TRUE(tester.device().getBool("/audio_inputs/aui_right/connected"));
  ASSERT_TRUE(tester.device().getBool("/audio_outputs/auo_left/connected"));
  ASSERT_TRUE(tester.device().getBool("/audio_outputs/auo_right/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_inputs/cvi/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_outputs/cvo/connected"));

  // disconnect audioSrc
  tester.unwire(audioSrc);

  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_right/connected"));
  ASSERT_TRUE(tester.device().getBool("/audio_outputs/auo_left/connected"));
  ASSERT_TRUE(tester.device().getBool("/audio_outputs/auo_right/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_inputs/cvi/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_outputs/cvo/connected"));

  // disconnect audioDst
  tester.unwire(audioDst);

  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_right/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_outputs/auo_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_outputs/auo_right/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_inputs/cvi/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_outputs/cvo/connected"));

  // disconnect cvSrc
  tester.unwire(cvSrc);

  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_right/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_outputs/auo_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_outputs/auo_right/connected"));
  ASSERT_FALSE(tester.device().getBool("/cv_inputs/cvi/connected"));
  ASSERT_TRUE(tester.device().getBool("/cv_outputs/cvo/connected"));

  // disconnect cvDst
  tester.unwire(cvDst);

  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_inputs/aui_right/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_outputs/auo_left/connected"));
  ASSERT_FALSE(tester.device().getBool("/audio_outputs/auo_right/connected"));
  ASSERT_FALSE(tester.device().getBool("/cv_inputs/cvi/connected"));
  ASSERT_FALSE(tester.device().getBool("/cv_outputs/cvo/connected"));
}

}