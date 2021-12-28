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
  ASSERT_EQ(tester.nextBatch(MockAudioDevice::buffer(1.0, 2.0)), MockAudioDevice::buffer(0, 0));

  // wire main inputs
  tester.wireMainIn(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);

  // device not fully wired yet (only IN is wired)
  ASSERT_EQ(tester.nextBatch(MockAudioDevice::buffer(3.0, 4.0)), MockAudioDevice::buffer(0, 0));

  // that means the internal buffer of the effect gets the input, but does not send it to the output
  ASSERT_EQ(tester.device()->fBuffer, MockAudioDevice::buffer(3.0, 4.0));

  // wire main outputs
  tester.wireMainOut(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);

  // device is fully wired
  ASSERT_EQ(tester.nextBatch(MockAudioDevice::buffer(5.0, 6.0)), MockAudioDevice::buffer(5.0, 6.0));

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
  auto c = MAUPst::CONFIG.clone().device_resources_dir(fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources"));
  StudioEffectTester<MAUPst> tester(c);
  tester.wireMainIn(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);
  tester.wireMainOut(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);

  auto sinePath = fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "audio", "sine.wav");
  auto sine = tester.loadSample(ConfigFile{sinePath});

  {
    auto processedSine = tester.processSample(ConfigFile{sinePath});

    // processedSine should be sine (since MAUPst is pass through)
    ASSERT_EQ(sine.fChannels, processedSine.fChannels);
    ASSERT_EQ(sine.fSampleRate, tester.rack().getSampleRate());
    ASSERT_EQ(sine.fData, processedSine.fData);
  }

  {
    auto processedSine = tester.processSample("/re/mock/audio/sine.wav", sample::Duration{30});
    auto expectedSine = sine; // sine (100 samples) + tail of 30 samples
    for(int i = 0; i < 30; i++)
      expectedSine.fData.emplace_back(0);
    ASSERT_EQ(expectedSine, processedSine);
  }

  {
    std::vector<int> frames{};

    auto processedSine = tester.processSample(ConfigFile{sinePath},
                                              time::Duration{1},
                                              tester.newTimeline().onEveryBatch([&frames](long f) { frames.emplace_back(f); return true; }));

    // 1ms at 44100 is 44.1 samples => 45 samples
    auto expectedSine = sine; // sine (100 samples) + tail of 45 samples
    for(int i = 0; i < 45; i++)
      expectedSine.fData.emplace_back(0);

    ASSERT_EQ(expectedSine, processedSine);
    ASSERT_EQ(std::vector<int>({0, 1, 2}), frames); // 145 samples = 3 rack frames
  }
}

//// DeviceTesters.SaveSample
// TODO write a non hardcoded version of this test
//TEST(DeviceTesters, SaveSample)
//{
//  StudioEffectTester<MAUPst> tester(MAUPst::CONFIG);
//
//  auto sinePath = fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "audio", "sine.wav");
//  auto sine = tester.loadSample(ConfigFile{sinePath});
//
//  tester.saveSample(sine, ConfigFile{"/tmp/sine2.wav"});
//
//  auto sine2 = tester.loadSample(ConfigFile{"/tmp/sine2.wav"});
//
//  ASSERT_EQ(sine, sine2);
//}

// Duration.Conversion
TEST(Duration, Conversion)
{
  Rack rack(44100);
  
  ASSERT_EQ(45, rack.toSampleDuration(time::Duration{1}).fFrames);
  ASSERT_EQ(4410, rack.toSampleDuration(time::Duration{100}).fFrames);
  ASSERT_EQ(100*64, rack.toSampleDuration(rack::Duration{100}).fFrames);
  ASSERT_EQ(100, rack.toSampleDuration(sample::Duration{100}).fFrames);

  ASSERT_EQ(1, rack.toRackDuration(time::Duration{1}).fBatches);
  ASSERT_EQ(69, rack.toRackDuration(time::Duration{100}).fBatches);
  ASSERT_EQ(100, rack.toRackDuration(rack::Duration{100}).fBatches);
  ASSERT_EQ(2, rack.toRackDuration(sample::Duration{100}).fBatches);

  ASSERT_EQ(345, rack.toRackDuration(sequencer::Duration::k1Beat).fBatches);

}

// InstrumentTester.Usage
TEST(InstrumentTester, Usage)
{
  // the MAUSrc configuration defines it as a helper, so we need to redefine it as an instrument
  auto config = DeviceConfig<MAUSrc>::fromSkeleton(DeviceType::kInstrument).mdef(Config::stereo_audio_out());

  InstrumentTester<MAUSrc> tester(config);

  tester.device()->fBuffer = MockAudioDevice::buffer(1.0, 2.0);

  // device is not wired yet!
  ASSERT_EQ(tester.nextBatch(), MockAudioDevice::buffer(0, 0));

  // wire main outputs
  tester.wireMainOut(MAUSrc::LEFT_SOCKET, MAUSrc::RIGHT_SOCKET);

  tester.device()->fBuffer = MockAudioDevice::buffer(3.0, 4.0);

  // device is fully wired
  ASSERT_EQ(tester.nextBatch(), MockAudioDevice::buffer(3.0, 4.0));
}

// InstrumentTester.Usage
TEST(InstrumentTester, TimelineUsage)
{
  // the MAUSrc configuration defines it as a helper, so we need to redefine it as an instrument
  auto config = DeviceConfig<MAUSrc>::fromSkeleton(DeviceType::kInstrument).mdef(Config::stereo_audio_out());

  InstrumentTester<MAUSrc> tester(config);

  tester.newTimeline()
    .event([&tester]() { tester.device()->fBuffer = MockAudioDevice::buffer(1.0, 2.0); })

    .nextBatch()

      // device is not wired yet!
    .event([&tester]() { ASSERT_EQ(tester.dst()->fBuffer, MockAudioDevice::buffer(0, 0)); })

      // wire main outputs
    .event([&tester]() { tester.wireMainOut(MAUSrc::LEFT_SOCKET, MAUSrc::RIGHT_SOCKET); })
    .event([&tester]() { tester.device()->fBuffer = MockAudioDevice::buffer(3.0, 4.0); })

    .nextBatch()

      // device is fully wired
    .event([&tester]() { ASSERT_EQ(tester.dst()->fBuffer, MockAudioDevice::buffer(3.0, 4.0)); })

    .execute()
    ;
}

// NotePlayerTester.Usage
TEST(NotePlayerTester, Usage)
{
  NotePlayerTester<MNPPst> tester(MNPPst::CONFIG);

  // should be enabled by default
  ASSERT_FALSE(tester.isBypassed());

  ASSERT_EQ(MockDevice::NoteEvents{}, tester.device()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.allNotesOff(), tester.nextBatch());
  ASSERT_EQ(MockDevice::NoteEvents{}.allNotesOff(), tester.device()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(Midi::A_440), tester.nextBatch(MockNotePlayer::NoteEvents{}.noteOn(Midi::A_440)));
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(Midi::A_440), tester.device()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOff(Midi::A_440, 25), tester.nextBatch(MockNotePlayer::NoteEvents{}.noteOff(Midi::A_440, 25)));
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOff(Midi::A_440, 25), tester.device()->fNoteEvents);

  // simulating sequencer note
  tester.device().use([](Motherboard &m) {
    m.setNoteInEvent(70, 120, 3);
  });

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(70, 120, 3), tester.nextBatch());

  // bypass note player (does not receive notes anymore)
  tester.setBypassed(true);
  ASSERT_EQ(MockDevice::NoteEvents{}, tester.nextBatch(MockNotePlayer::NoteEvents{}.noteOn(Midi::A_440)));

  // revert
  tester.setBypassed(false);
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(Midi::A_440), tester.nextBatch(MockNotePlayer::NoteEvents{}.noteOn(Midi::A_440)));

  // unwire src
  tester.unwire(tester.src());
  ASSERT_EQ(MockDevice::NoteEvents{}, tester.nextBatch(MockNotePlayer::NoteEvents{}.noteOn(Midi::A_440)));
}

// HelperTester.Usage
TEST(HelperTester, Usage)
{
  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      fBatchCount++;
    }

    int fBatchCount{};
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

  ASSERT_EQ(0, tester.device()->fBatchCount);

  audioSrc->fBuffer = MockAudioDevice::buffer(0.5, 0.6);
  cvSrc->fValue = 1.0;

  // make sure it runs
  tester.nextBatch();

  ASSERT_EQ(1, tester.device()->fBatchCount);

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

// Timeline.Usage
TEST(Timeline, Usage)
{
  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      auto const transport = JBox_GetMotherboardObjectRef("/transport");
      fTransportPlaying = JBox_GetBoolean(JBox_LoadMOMPropertyByTag(transport, kJBox_TransportPlaying));
      fTransportPlayPos = JBox_GetNumber(JBox_LoadMOMPropertyByTag(transport, kJBox_TransportPlayPos));
      fBatchCount++;
      fNoteEvents.clear();

      auto noteStates = JBox_GetMotherboardObjectRef("/note_states");
      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        if(diff.fPropertyRef.fObject == noteStates)
          fNoteEvents.emplace_back(JBox_AsNoteEvent(diff));
      }
    }

    int fBatchCount{};
    bool fTransportPlaying{};
    int fTransportPlayPos{};
    std::vector<TJBox_NoteEvent> fNoteEvents{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .accept_notes(true)
    .rtc(Config::rt_input_setup_notify("/note_states/*"))
  ;

  auto tester = HelperTester<Device>(c);

  ASSERT_EQ(0, tester.device()->fBatchCount);
  ASSERT_FALSE(tester.device()->fTransportPlaying);

  // empty timeline
  tester.newTimeline().execute();

  ASSERT_EQ(0, tester.device()->fBatchCount);
  ASSERT_FALSE(tester.device()->fTransportPlaying);

  long expectedBatch = 0;

  auto checkBatch = [](long iExpectedAtBatch, long iAtBatch) { ASSERT_EQ(iExpectedAtBatch, iAtBatch); };

  auto checkEveryBatch = [&expectedBatch, &checkBatch](long iAtBatch) { checkBatch(expectedBatch, iAtBatch); expectedBatch++; return true; };

  // check for proper batch count
  tester.newTimeline()
    .onEveryBatch(checkEveryBatch)
    .event([&checkBatch](long iAtBatch) { checkBatch(0, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(0, tester.device()->fBatchCount); })
    .nextBatch()
    .event([&checkBatch](long iAtBatch) { checkBatch(1, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(1, tester.device()->fBatchCount); })
    .event([&tester]() { ASSERT_EQ(128, tester.device()->fNoteEvents.size()); }) // initial setup
    .after(sequencer::Duration::k1Beat) // 344 batches (see Duration.Conversion test)
    .event([&checkBatch](long iAtBatch) { checkBatch(346, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(346, tester.device()->fBatchCount); })
    .execute();

  ASSERT_EQ(347, tester.device()->fBatchCount);
  ASSERT_FALSE(tester.device()->fTransportPlaying);
  ASSERT_EQ(0, tester.device()->fNoteEvents.size());

  // reset frame count
  tester.device()->fBatchCount = 0;
  expectedBatch = 0;

  // test note on/off
  tester.newTimeline()
    .onEveryBatch(checkEveryBatch)
    .event([&checkBatch](long iAtBatch) { checkBatch(0, iAtBatch); return true; })
    .note(Midi::A_440, sequencer::Duration::k1Beat) // note On at batch 0 | note off at batch 345

    .nextBatch() // 1
    .event([&checkBatch](long iAtBatch) { checkBatch(1, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(1, tester.device()->fBatchCount); })
    .event([&tester]() {
      ASSERT_EQ(1, tester.device()->fNoteEvents.size());
      ASSERT_EQ(Midi::A_440, tester.device()->fNoteEvents[0].fNoteNumber);
      ASSERT_EQ(100, tester.device()->fNoteEvents[0].fVelocity);
      ASSERT_EQ(0, tester.device()->fNoteEvents[0].fAtFrameIndex);
    })

    .nextBatch() // 2
    .event([&checkBatch](long iAtBatch) { checkBatch(2, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(2, tester.device()->fBatchCount); })
    .event([&tester]() { ASSERT_EQ(0, tester.device()->fNoteEvents.size()); })

    .after(rack::Duration{343}) // 2 + 343 = 345
    .event([&checkBatch](long iAtBatch) { checkBatch(345, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(345, tester.device()->fBatchCount); })
    .event([&tester]() { ASSERT_EQ(0, tester.device()->fNoteEvents.size()); })

    .nextBatch() // 346
    .event([&checkBatch](long iAtBatch) { checkBatch(346, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(346, tester.device()->fBatchCount); })
    .event([&tester]() {
      ASSERT_EQ(1, tester.device()->fNoteEvents.size());
      ASSERT_EQ(Midi::A_440, tester.device()->fNoteEvents[0].fNoteNumber);
      ASSERT_EQ(0, tester.device()->fNoteEvents[0].fVelocity);
      ASSERT_EQ(0, tester.device()->fNoteEvents[0].fAtFrameIndex);
    })

    .execute();

  // reset frame count
  tester.device()->fBatchCount = 0;

  ASSERT_FALSE(tester.device()->fTransportPlaying);

  ASSERT_EQ(0, tester.device()->fTransportPlayPos);

  // testing play (uses transportStart / transportStop)
  tester.newTimeline()
    .event([&checkBatch](long iAtBatch) { checkBatch(0, iAtBatch); return true; })
    .nextBatch()
    .event([&checkBatch](long iAtBatch) { checkBatch(1, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(1, tester.device()->fBatchCount); })
    .event([&tester]() { ASSERT_TRUE(tester.device()->fTransportPlaying); })
    .event([&tester]() { ASSERT_EQ(0, tester.device()->fTransportPlayPos); })
    .nextBatch()
    .event([&tester]() { ASSERT_EQ(45, tester.device()->fTransportPlayPos); })
    .play();

  // see PPQAccumulator.next test for values: 0 / 45 / 89 / 134
  // only propagated to device on nextBatch
  ASSERT_EQ(89, tester.device()->fTransportPlayPos);
  ASSERT_TRUE(tester.device()->fTransportPlaying);

  // but rack already updated
  ASSERT_FALSE(tester.rack().getTransportPlaying());
  ASSERT_EQ(134, tester.rack().getTransportPlayPos());

  tester.nextBatch(); // propagate to device on nextBatch
  ASSERT_EQ(134, tester.device()->fTransportPlayPos);
  ASSERT_FALSE(tester.device()->fTransportPlaying);
  ASSERT_EQ(128, tester.device()->fNoteEvents.size()); // all notes are reset
}

}