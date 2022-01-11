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

#include <re/mock/Sequencer.h>
#include <re/mock/DeviceTesters.h>
#include <gtest/gtest.h>

namespace re::mock::Test {

using namespace re::mock::sequencer;

// Time.normalize
TEST(Time, normalize)
{
  {
    // 1.5.1.0 at 4/4 is 2.1.1.0
    auto time = Time().beats(5).normalize();
    ASSERT_EQ(2, time.bars());
    ASSERT_EQ(1, time.beats());
    ASSERT_EQ(1, time.sixteenth());
    ASSERT_EQ(0, time.ticks());
  }

  // 1.5.1.1000 at 4/4 is 2.2.1.40
  ASSERT_EQ("2.2.1.40", Time(1,5,1,1000).normalize().toString());

  // 1.5.1.1000 at 4/16 is 3.1.1.40
  ASSERT_EQ("3.1.1.40", Time(1,5,1,1000,TimeSignature(4, 16)).normalize().toString());

  // 1.1.3.0 at 4/16 is 1.3.1.0
  ASSERT_EQ("1.3.1.0", Time(1,1,3,0,TimeSignature(4, 16)).normalize().toString());

  // 1.5.1.1000 at 5/16 is 2.4.1.40
  ASSERT_EQ("2.4.1.40", Time(1,5,1,1000,TimeSignature(5, 16)).normalize().toString());
  ASSERT_EQ("2.4.1.40", Time(1,5,1,1000).signature(TimeSignature(5, 16)).normalize().toString());

  // Conversion: Time(1,5,1,1000) == Time(1,5,1,1000, 4/4) == 2.2.1.40
  ASSERT_EQ("5.1.1.40", Time(1,5,1,1000).withOtherSignature(TimeSignature(5, 16)).toString());
}

// Time.toPPQ
TEST(Time, toPPQ)
{
  {
    // 4/4 (default)
    ASSERT_EQ(0, Time().toPPQCount());
    ASSERT_EQ(0, Time(1, 1, 1, 0).toPPQCount());
    ASSERT_EQ(1600, Time(1, 1, 1, 100).toPPQCount());
    ASSERT_EQ(5440, Time(1, 1, 2, 100).toPPQCount());
    ASSERT_EQ(20800, Time(1, 2, 2, 100).toPPQCount());
    ASSERT_EQ(82240, Time(2, 2, 2, 100).toPPQCount());

    ASSERT_EQ("2.2.2.100", Time::from(82240).normalize().toString());

    ASSERT_EQ(Time(1, 1, 1, 100000).toPPQCount(), Time(1, 1, 1, 100000).normalize().toPPQCount());
  }
  {
    // 5/8
    auto ts = TimeSignature(5,8);
    ASSERT_EQ(0, Time(1, 1, 1, 0, ts).toPPQCount());
    ASSERT_EQ(1600, Time(1, 1, 1, 100, ts).toPPQCount());
    ASSERT_EQ(5440, Time(1, 1, 2, 100, ts).toPPQCount());
    ASSERT_EQ(13120, Time(1, 2, 2, 100, ts).toPPQCount());
    ASSERT_EQ(51520, Time(2, 2, 2, 100, ts).toPPQCount());

    ASSERT_EQ(Time(1, 1, 1, 100000, ts).toPPQCount(), Time(1, 1, 1, 100000, ts).normalize().toPPQCount());
  }
  {
    // 5/16
    auto ts = TimeSignature(5, 16);
    ASSERT_EQ(0, Time(1, 1, 1, 0, ts).toPPQCount());
    ASSERT_EQ(1600, Time(1, 1, 1, 100, ts).toPPQCount());
    ASSERT_EQ(5440, Time(1, 1, 2, 100, ts).toPPQCount());
    ASSERT_EQ(9280, Time(1, 2, 2, 100, ts).toPPQCount());
    ASSERT_EQ(28480, Time(2, 2, 2, 100, ts).toPPQCount());

    ASSERT_EQ("2.3.1.100", Time(2, 2, 2, 100, ts).normalize().toString());
    ASSERT_EQ("2.3.1.100", Time::from(28480, ts).toString());


    ASSERT_EQ(28480, Time(2, 2, 2, 100, ts).toPPQCount());
    auto time = Time(1, 1, 1, 50, ts) + sequencer::Duration(1, 1, 1, 50, ts);
    ASSERT_EQ(28480, time.toPPQCount());
  }
}

// Duration.toPPQ
TEST(Duration, toPPQ)
{
  {
    // 4/4 (default)
    ASSERT_EQ(0, sequencer::Duration().toPPQCount());
    ASSERT_EQ(0, sequencer::Duration(0, 0, 0, 0).toPPQCount());
    ASSERT_EQ(1600, sequencer::Duration(0, 0, 0, 100).toPPQCount());
    ASSERT_EQ(5440, sequencer::Duration(0, 0, 1, 100).toPPQCount());
    ASSERT_EQ(20800, sequencer::Duration(0, 1, 1, 100).toPPQCount());
    ASSERT_EQ(82240, sequencer::Duration(1, 1, 1, 100).toPPQCount());
  }

  {
    // 5/8
    auto ts = TimeSignature(5,8);
    ASSERT_EQ(0, sequencer::Duration(0, 0, 0, 0, ts).toPPQCount());
    ASSERT_EQ(1600, sequencer::Duration(0, 0, 0, 100, ts).toPPQCount());
    ASSERT_EQ(5440, sequencer::Duration(0, 0, 1, 100, ts).toPPQCount());
    ASSERT_EQ(13120, sequencer::Duration(0, 1, 1, 100, ts).toPPQCount());
    ASSERT_EQ(51520, sequencer::Duration(1, 1, 1, 100, ts).toPPQCount());
  }

  {
    // 5/16
    auto ts = TimeSignature(5,16);
    ASSERT_EQ(0, sequencer::Duration(0, 0, 0, 0, ts).toPPQCount());
    ASSERT_EQ(1600, sequencer::Duration(0, 0, 0, 100, ts).toPPQCount());
    ASSERT_EQ(5440, sequencer::Duration(0, 0, 1, 100, ts).toPPQCount());
    ASSERT_EQ(9280, sequencer::Duration(0, 1, 1, 100, ts).toPPQCount());
    ASSERT_EQ(28480, sequencer::Duration(1, 1, 1, 100, ts).toPPQCount());
  }}

// Track.Usage
TEST(Track, Usage)
{
  Track track{};

  ASSERT_THROW(track.getFirstEventTime(), Exception);
  ASSERT_THROW(track.getLastEventTime(), Exception);

  track.event(sequencer::Time(3,1,1,0), []{});

  ASSERT_EQ(sequencer::Time(3,1,1,0).toString(), track.getFirstEventTime().toString());
  ASSERT_EQ(sequencer::Time(3,1,1,0).toString(), track.getLastEventTime().toString());

  track.event(sequencer::Time(2,1,1,0), []{});

  ASSERT_EQ(sequencer::Time(2,1,1,0).toString(), track.getFirstEventTime().toString());
  ASSERT_EQ(sequencer::Time(3,1,1,0).toString(), track.getLastEventTime().toString());

}

// Sequencer.executeEvents
TEST(Track, executeEvents)
{
  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      if(iDiffCount > 100) // skipping note reset
        return;

      auto const transport = JBox_GetMotherboardObjectRef("/transport");
      fTransportPlaying = JBox_GetBoolean(JBox_LoadMOMPropertyByTag(transport, kJBox_TransportPlaying));
      fTransportPlayPos = JBox_GetNumber(JBox_LoadMOMPropertyByTag(transport, kJBox_TransportPlayPos));

      auto noteStates = JBox_GetMotherboardObjectRef("/note_states");
      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        if(diff.fPropertyRef.fObject == noteStates)
        {
          auto note = JBox_AsNoteEvent(diff);
          fOutput += fmt::printf("batch=%d,pos=%d,note=%d,vel=%d,idx=%d\n", fBatchCount, fTransportPlayPos,
                                 note.fNoteNumber, note.fVelocity, note.fAtFrameIndex);
        }
      }

      fBatchCount++;
    }

    int fBatchCount{};
    bool fTransportPlaying{};
    int fTransportPlayPos{};
    std::string fOutput{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .accept_notes(true)
    .rtc(Config::rt_input_setup_notify("/note_states/*"))
  ;

  auto tester = HelperTester<Device>(c);

  TJBox_Int64 expectedPlayBatchStartPos = 0;
  TJBox_Int64 checkEveryBatchCount = 0;

  tester.nextBatch(); // initializes the device

  // reset the device
  tester.device()->fOutput = "";
  tester.device()->fBatchCount = 0;

  auto checkEveryBatch = [&expectedPlayBatchStartPos, &checkEveryBatchCount](Motherboard &, Track::Batch const &iBatch) {
    ASSERT_EQ(expectedPlayBatchStartPos, iBatch.fPlayBatchStartPos);
    ASSERT_EQ(0, iBatch.fAtFrameIndex);
    expectedPlayBatchStartPos = iBatch.fPlayBatchEndPos;
    checkEveryBatchCount++;
  };

  // at 44100, 1 batch is 44.582313 ppq

  // sequencer::Time(1,1,1,100) | PPQ = 1600 | Batch 35 [1560, 1605) | Frame Index (1600 - 1560) / (1605 - 1560) * 64 = 56
  ASSERT_EQ(1600, sequencer::Time(1,1,1,100).toPPQCount());

  // sequencer::Time(1,1,1,50) | PPQ = 800 | Batch 17 [758, 802) | Frame Index (800 - 758) / (802 - 758) * 64 = 61
  ASSERT_EQ(800, sequencer::Time(1,1,1,50).toPPQCount());

  // sequencer::Duration(0,0,1,0) | PPQ = 3840
  ASSERT_EQ(3840, sequencer::Duration(0,0,1,0).toPPQCount());

  // sequencer::Duration(0,0,0,200) | PPQ = 3200
  ASSERT_EQ(3200, sequencer::Duration(0,0,0,200).toPPQCount());

  tester.sequencerTrack()
    .onEveryBatch(checkEveryBatch)

    // equivalent to Time{1,1,1,100}
    // note on PPQ = 1600 | batch 35 [1560, 1605) | frameIndex 56
    // note off PPQ = 1600 + 3840 = 5440 | batch 122 [5439, 5483) | frameIndex 1
    .after(sequencer::Duration{0,0,0,100})
    .note(60, sequencer::Duration{0,0,1,0}, 101)

    // note on  PPQ = 800 | Batch 17 [758, 802) | frameIndex = 61
    // note off PPQ = 800 + 3200 = 4000 | Batch 89 [3968, 4012) | frameIndex = 46
    .at(sequencer::Time{1,1,1,50}) // go back in time!
    .note(61, sequencer::Duration{0,0,0,200}, 102)

    // note on PPQ = 1600 | batch 35 [1560, 1605) | frameIndex 56
    // note off PPQ = 1600 + 3200 = 4800 | batch 107 [4770, 4815) | frameIndex 42
    .at(sequencer::Time{1,1,1,100}) // same as 60 time
    .note(62, sequencer::Duration{0,0,0,200}, 103)
    ;

  tester.newTimeline().play(sequencer::TimeSignature{}.oneBeat());

  // 345: see Duration.Conversion test
  ASSERT_EQ(345, checkEveryBatchCount);

  ASSERT_EQ("batch=17,pos=758,note=61,vel=102,idx=61\n"
            "batch=35,pos=1560,note=60,vel=101,idx=57\n"
            "batch=35,pos=1560,note=62,vel=103,idx=57\n"
            "batch=89,pos=3968,note=61,vel=0,idx=0\n"
            "batch=107,pos=4770,note=62,vel=0,idx=0\n"
            "batch=122,pos=5439,note=60,vel=0,idx=0\n", tester.device()->fOutput);
}

// Track.looping
TEST(Track, looping)
{
  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      if(iDiffCount > 100) // skipping note reset
        return;

      auto const transport = JBox_GetMotherboardObjectRef("/transport");
      fTransportPlaying = JBox_GetBoolean(JBox_LoadMOMPropertyByTag(transport, kJBox_TransportPlaying));
      fTransportPlayPos = JBox_GetNumber(JBox_LoadMOMPropertyByTag(transport, kJBox_TransportPlayPos));

      auto noteStates = JBox_GetMotherboardObjectRef("/note_states");
      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        if(diff.fPropertyRef.fObject == transport)
        {
          if(diff.fPropertyTag == kJBox_TransportPlaying)
          {
            if(JBox_GetBoolean(diff.fPreviousValue) == false && JBox_GetBoolean(diff.fCurrentValue) == true)
            {
              fOutput += re::mock::fmt::printf("batch=%d,pos=%d\n", fBatchCount, fTransportPlayPos);
            }
          }
          if(diff.fPropertyTag == kJBox_TransportPlayPos)
          {
            if(JBox_GetNumber(diff.fPreviousValue) > JBox_GetNumber(diff.fCurrentValue))
            {
              fOutput += fmt::printf("Looping: batch=%d,pos %d => %d\n", fBatchCount,
                                                 static_cast<int>(JBox_GetNumber(diff.fPreviousValue)),
                                                 static_cast<int>(JBox_GetNumber(diff.fCurrentValue)));
            }
          }
        }
        if(diff.fPropertyRef.fObject == noteStates)
        {
          auto note = JBox_AsNoteEvent(diff);
          fOutput += fmt::printf("batch=%d,pos=%d,note=%d,vel=%d,idx=%d\n", fBatchCount, fTransportPlayPos,
                                 note.fNoteNumber, note.fVelocity, note.fAtFrameIndex);
        }
      }

      fBatchCount++;
    }

    int fBatchCount{};
    bool fTransportPlaying{};
    int fTransportPlayPos{};
    std::string fOutput{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .accept_notes(true)
    .rtc(Config::rt_input_setup_notify("/note_states/*"))
    .rtc(Config::rt_input_setup_notify("/transport/*"))
  ;

  auto tester = HelperTester<Device>(c);

  tester.nextBatch(); // initializes the device

  // reset the device
  tester.device()->fOutput = "";
  tester.device()->fBatchCount = 0;

  // we test looping position first
  tester.rack().setTransportLoopEnabled(true);
  tester.rack().setTransportLoop(sequencer::Time(2,1,1,0), sequencer::TimeSignature{}.oneBeat());
  tester.rack().setTransportPlayPos(sequencer::Time(2,1,1,0));

  tester.newTimeline().play(sequencer::Duration(0,12,0,0));

  auto expected = R"(batch=0,pos=61440
Looping: batch=345,pos 76776 => 61461
Looping: batch=690,pos 76797 => 61482
Looping: batch=1034,pos 76773 => 61458
Looping: batch=1379,pos 76794 => 61479
Looping: batch=1723,pos 76771 => 61455
Looping: batch=2068,pos 76791 => 61476
Looping: batch=2412,pos 76768 => 61452
Looping: batch=2757,pos 76789 => 61473
Looping: batch=3101,pos 76765 => 61450
Looping: batch=3446,pos 76786 => 61470
Looping: batch=3790,pos 76762 => 61447
)";

  ASSERT_EQ(expected, tester.device()->fOutput);

  // reset the device
  tester.rack().setTransportLoopEnabled(false);
  tester.rack().setTransportPlayPos(sequencer::Time(2,1,1,0));
  tester.nextBatch();
  tester.device()->fOutput = "";
  tester.device()->fBatchCount = 0;

  tester.rack().setTransportLoopEnabled(true);

  // now we use notes
  tester.sequencerTrack()
    .note(60, sequencer::Time(1,4,4,120), sequencer::TimeSignature{}.one16th())
    .note(61, sequencer::Time(2,1,2,0), sequencer::TimeSignature{}.one16th())
    .note(62, sequencer::Time(2,1,4,120), sequencer::TimeSignature{}.one16th())
    .note(63, sequencer::Time(1,4,4,120), sequencer::Duration(0,1,1,0))
    ;

  tester.newTimeline().play(sequencer::Duration(0,2,1,0));

  std::cout << tester.device()->fOutput << std::endl;

  auto s = R"(batch=0,pos=61440
batch=86,pos=65274,note=61,vel=100,idx=9
batch=172,pos=69108,note=61,vel=0,idx=0
batch=301,pos=74859,note=62,vel=100,idx=30
batch=344,pos=76776,note=62,vel=0,idx=0
Looping: batch=345,pos 76776 => 61461
batch=430,pos=65250,note=61,vel=100,idx=43
batch=516,pos=69084,note=61,vel=0,idx=0
batch=646,pos=74880,note=62,vel=100,idx=0
batch=689,pos=76797,note=62,vel=0,idx=0
Looping: batch=690,pos 76797 => 61482
batch=775,pos=65271,note=61,vel=100,idx=13
)";

  ASSERT_EQ(s, tester.device()->fOutput);
}

}