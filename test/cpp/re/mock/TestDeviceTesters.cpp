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
  class Device : public MAUPst
  {
  public:
    explicit Device(int iSampleRate) : MAUPst(iSampleRate) {}
    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      MAUPst::renderBatch(iPropertyDiffs, iDiffCount);
      fBatchCount++;
    }
    int fBatchCount{};
  };
  auto c = DeviceConfig<Device>::fromSkeleton(DeviceType::kStudioFX)
    .mdef(Config::stereo_audio_out())
    .mdef(Config::stereo_audio_in())
    .device_resources_dir(fs::path(RE_MOCK_PROJECT_DIR) / "test" / "resources");

  StudioEffectTester<Device> tester(c);
  tester.wireMainIn(MockAudioDevice::LEFT_SOCKET, MockAudioDevice::RIGHT_SOCKET);
  tester.wireMainOut(MockAudioDevice::LEFT_SOCKET, MockAudioDevice::RIGHT_SOCKET);

  auto sinePath = fs::path(RE_MOCK_PROJECT_DIR) / "test" / "resources" / "re" / "mock" / "audio" / "sine.wav";
  auto const sine = tester.loadSample(resource::File{sinePath});

  {
    tester.device()->fBatchCount = 0;
    auto processedSine = tester.processSample(resource::File{sinePath});

    // processedSine should be sine (since MAUPst is pass through)
    ASSERT_EQ(sine->fChannels, processedSine->fChannels);
    ASSERT_EQ(sine->fSampleRate, tester.rack().getSampleRate());
    ASSERT_EQ(sine->fData, processedSine->fData);
    ASSERT_EQ(2, tester.device()->fBatchCount); // 100 = 64 + 36
  }

  {
    tester.device()->fBatchCount = 0;
    auto processedSine = tester.processSample("/re/mock/audio/sine.wav", sample::Duration{30});
    auto expectedSine = sine->clone(); // sine (100 samples) + tail of 30 samples
    for(int i = 0; i < 30; i++)
      expectedSine.fData.emplace_back(static_cast<TJBox_AudioSample>(0));
    ASSERT_EQ(expectedSine, *processedSine);
    ASSERT_EQ(3, tester.device()->fBatchCount); // 130 = 64 + 64 + 2
  }

  {
    std::vector<int> batches{};
    tester.device()->fBatchCount = 0;
    auto processedSine = tester.processSample(resource::File{sinePath},
                                              time::Duration{1},
                                              tester.newTimeline().onEveryBatch([&batches](long f) { batches.emplace_back(f); return true; }));

    // 1ms at 44100 is 44.1 samples => 45 samples
    auto expectedSine = sine->clone(); // sine (100 samples) + tail of 45 samples
    for(int i = 0; i < 45; i++)
      expectedSine.fData.emplace_back(static_cast<TJBox_AudioSample>(0));

    ASSERT_EQ(expectedSine, *processedSine);
    ASSERT_EQ(std::vector<int>({0, 1, 2}), batches);
    ASSERT_EQ(3, tester.device()->fBatchCount); // 145 = 64 + 64 + 17
  }
}

// DeviceTesters.SaveSample
// TODO write a non hardcoded version of this test
TEST(DeviceTesters, SaveSample)
{
  StudioEffectTester<MAUPst> tester(MAUPst::CONFIG);

  // Implementation note: sine.wav is encoded as 16-bit signed integer
  // sine2.wav is encoded as 32-bit floating point

  auto sinePath = fs::path(RE_MOCK_PROJECT_DIR) / "test" / "resources" / "re" / "mock" / "audio" / "sine.wav";
  auto sine = tester.loadSample(resource::File{sinePath});

  auto sine2Path = fs::temp_directory_path() / "sine2.wav";

  tester.saveSample(*sine.get(), resource::File{sine2Path});

  auto sine2 = tester.loadSample(resource::File{sine2Path});

  ASSERT_EQ(sine->getChannels(), sine2->getChannels());
  ASSERT_EQ(sine->getFrameCount(), sine2->getFrameCount());
  ASSERT_EQ(sine->getSampleCount(), sine2->getSampleCount());
  // due to math precision of conversion, we need to check with ASSERT_FLOAT_EQ
  for(auto i = 0; i < sine->getSampleCount(); i++)
  {
    ASSERT_FLOAT_EQ(sine->getData()[i], sine2->getData()[i]);
  }
}

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

  ASSERT_EQ(345, rack.toRackDuration(sequencer::Duration::k1Beat_4x4).fBatches);

}

// InstrumentTester.Usage
TEST(InstrumentTester, Usage)
{
  // the MAUSrc configuration defines it as a helper, so we need to redefine it as an instrument
  auto config = DeviceConfig<MAUSrc>::fromSkeleton(DeviceType::kInstrument).mdef(Config::stereo_audio_out());

  InstrumentTester<MAUSrc> tester(config);

  tester.device()->fBuffer = MockAudioDevice::buffer(1.0, 2.0);

  // device is not wired yet!
  ASSERT_EQ(tester.nextBatch({}), MockAudioDevice::buffer(0, 0));

  // wire main outputs
  tester.wireMainOut(MAUSrc::LEFT_SOCKET, MAUSrc::RIGHT_SOCKET);

  tester.device()->fBuffer = MockAudioDevice::buffer(3.0, 4.0);

  // device is fully wired
  ASSERT_EQ(tester.nextBatch({}), MockAudioDevice::buffer(3.0, 4.0));
}

// InstrumentTester.Usage
TEST(InstrumentTester, TimelineUsage)
{
  // the MAUSrc configuration defines it as a helper, so we need to redefine it as an instrument
  auto config = DeviceConfig<MAUSrc>::fromSkeleton(DeviceType::kInstrument).mdef(Config::stereo_audio_out());

  InstrumentTester<MAUSrc> tester(config);

  tester.newTimeline()
    .event([&tester]() { tester.device()->fBuffer = MockAudioDevice::buffer(1.0, 2.0); })

    .after1Batch()

      // device is not wired yet!
    .event([&tester]() { ASSERT_EQ(tester.dst()->fBuffer, MockAudioDevice::buffer(0, 0)); })

      // wire main outputs
    .event([&tester]() { tester.wireMainOut(MAUSrc::LEFT_SOCKET, MAUSrc::RIGHT_SOCKET); })
    .event([&tester]() { tester.device()->fBuffer = MockAudioDevice::buffer(3.0, 4.0); })

    .after1Batch()

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

  ASSERT_EQ(MockDevice::NoteEvents{}.allNotesOff(), tester.nextBatch({}));
  ASSERT_EQ(MockDevice::NoteEvents{}.allNotesOff(), tester.device()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(Midi::A_440), tester.nextBatch(MockNotePlayer::NoteEvents{}.noteOn(Midi::A_440)));
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(Midi::A_440), tester.device()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOff(Midi::A_440, 25), tester.nextBatch(MockNotePlayer::NoteEvents{}.noteOff(Midi::A_440, 25)));
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOff(Midi::A_440, 25), tester.device()->fNoteEvents);

  // simulating sequencer note
  tester.device().withJukebox([](Motherboard &m) {
    m.setNoteInEvent(70, 120, 3);
  });

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(70, 120, 3), tester.nextBatch({}));

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
    .after1Batch()
    .event([&checkBatch](long iAtBatch) { checkBatch(1, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(1, tester.device()->fBatchCount); })
    .event([&tester]() { ASSERT_EQ(128, tester.device()->fNoteEvents.size()); }) // initial setup
    .after(sequencer::Duration::k1Beat_4x4) // 344 batches (see Duration.Conversion test)
    .event([&checkBatch](long iAtBatch) { checkBatch(346, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(346, tester.device()->fBatchCount); })
    .execute();

  ASSERT_EQ(347, tester.device()->fBatchCount);
  ASSERT_FALSE(tester.device()->fTransportPlaying);
  ASSERT_EQ(0, tester.device()->fNoteEvents.size());

  // reset batch count
  tester.device()->fBatchCount = 0;
  expectedBatch = 0;

  // test note on/off
  tester.newTimeline()
    .onEveryBatch(checkEveryBatch)
    .event([&checkBatch](long iAtBatch) { checkBatch(0, iAtBatch); return true; })
    .note(Midi::A_440, sequencer::Duration::k1Beat_4x4) // note On at batch 0 | note off at batch 345

    .after1Batch() // 1
    .event([&checkBatch](long iAtBatch) { checkBatch(1, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(1, tester.device()->fBatchCount); })
    .event([&tester]() {
      ASSERT_EQ(1, tester.device()->fNoteEvents.size());
      ASSERT_EQ(Midi::A_440, tester.device()->fNoteEvents[0].fNoteNumber);
      ASSERT_EQ(100, tester.device()->fNoteEvents[0].fVelocity);
      ASSERT_EQ(0, tester.device()->fNoteEvents[0].fAtFrameIndex);
    })

    .after1Batch() // 2
    .event([&checkBatch](long iAtBatch) { checkBatch(2, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(2, tester.device()->fBatchCount); })
    .event([&tester]() { ASSERT_EQ(0, tester.device()->fNoteEvents.size()); })

    .after(rack::Duration{343}) // 2 + 343 = 345
    .event([&checkBatch](long iAtBatch) { checkBatch(345, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(345, tester.device()->fBatchCount); })
    .event([&tester]() { ASSERT_EQ(0, tester.device()->fNoteEvents.size()); })

    .after1Batch() // 346
    .event([&checkBatch](long iAtBatch) { checkBatch(346, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(346, tester.device()->fBatchCount); })
    .event([&tester]() {
      ASSERT_EQ(1, tester.device()->fNoteEvents.size());
      ASSERT_EQ(Midi::A_440, tester.device()->fNoteEvents[0].fNoteNumber);
      ASSERT_EQ(0, tester.device()->fNoteEvents[0].fVelocity);
      ASSERT_EQ(0, tester.device()->fNoteEvents[0].fAtFrameIndex);
    })

    .execute();

  // reset batch count
  tester.device()->fBatchCount = 0;

  ASSERT_FALSE(tester.device()->fTransportPlaying);

  ASSERT_EQ(0, tester.device()->fTransportPlayPos);

  // testing play (uses transportStart / transportStop)
  tester.newTimeline()
    .event([&checkBatch](long iAtBatch) { checkBatch(0, iAtBatch); return true; })
    .after1Batch()
    .event([&checkBatch](long iAtBatch) { checkBatch(1, iAtBatch); return true; })
    .event([&tester]() { ASSERT_EQ(1, tester.device()->fBatchCount); })
    .event([&tester]() { ASSERT_TRUE(tester.device()->fTransportPlaying); })
    .event([&tester]() { ASSERT_EQ(0, tester.device()->fTransportPlayPos); })
    .after1Batch()
    .event([&tester]() { ASSERT_EQ(44, tester.device()->fTransportPlayPos); })
    .play();

  // play() stops the transport and does NOT call nextBatch to propagate
  // see PPQAccumulator.next test for values: 0 / 44 / 89 / 134
  ASSERT_EQ(89, tester.device()->fTransportPlayPos);
  ASSERT_TRUE(tester.device()->fTransportPlaying);

  // the rack has the proper values
  ASSERT_FALSE(tester.rack().getTransportPlaying());
  ASSERT_EQ(134, tester.rack().getTransportPlayPos());

  // propagate the values to the device including the "all notes stop" event
  tester.nextBatch();

  ASSERT_EQ(134, tester.device()->fTransportPlayPos);
  ASSERT_FALSE(tester.device()->fTransportPlaying);
  ASSERT_EQ(128, tester.device()->fNoteEvents.size()); // all notes are reset

  // check for invalid usage: events in the timeline should NOT call nextBatch
  ASSERT_THROW(tester.newTimeline().event([&tester]{ tester.nextBatch(); }).execute(), Exception);
  ASSERT_THROW(tester.newTimeline().event([&tester]{ tester.nextBatch(); }).play(), Exception);
}

}