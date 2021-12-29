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
    auto time = Time().beats(5).normalize(TimeSignature(4, 4));
    ASSERT_EQ(2, time.bars());
    ASSERT_EQ(1, time.beats());
    ASSERT_EQ(1, time.sixteenth());
    ASSERT_EQ(0, time.ticks());
  }

  // 1.5.1.1000 at 4/4 is 2.2.1.40
  ASSERT_EQ("2.2.1.40", Time(1,5,1,1000).normalize(TimeSignature(4, 4)).toString());

  // 1.5.1.1000 at 4/16 is 3.1.1.40
  ASSERT_EQ("3.1.1.40", Time(1,5,1,1000).normalize(TimeSignature(4, 16)).toString());

  // 1.1.3.0 at 4/16 is 1.3.1.0
  ASSERT_EQ("1.3.1.0", Time(1,1,3).normalize(TimeSignature(4, 16)).toString());

  // 1.5.1.1000 at 5/16 is 3.1.1.40
  ASSERT_EQ("2.4.1.40", Time(1,5,1,1000).normalize(TimeSignature(5, 16)).toString());
}

// Time.toPPQ
TEST(Time, toPPQ)
{
  {
    // 4/4
    auto ts = TimeSignature(4,4);
    ASSERT_EQ(0, Time().toPPQ(ts).fCount);
    ASSERT_EQ(0, Time(1, 1, 1, 0).toPPQ(ts).fCount);
    ASSERT_EQ(1600, Time(1, 1, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(5440, Time(1, 1, 2, 100).toPPQ(ts).fCount);
    ASSERT_EQ(20800, Time(1, 2, 2, 100).toPPQ(ts).fCount);
    ASSERT_EQ(82240, Time(2, 2, 2, 100).toPPQ(ts).fCount);

    ASSERT_EQ(Time(1, 1, 1, 100000).toPPQ(ts).fCount, Time(1, 1, 1, 100000).normalize(ts).toPPQ(ts).fCount);
  }
  {
    // 5/8
    auto ts = TimeSignature(5,8);
    ASSERT_EQ(0, Time(1, 1, 1, 0).toPPQ(ts).fCount);
    ASSERT_EQ(1600, Time(1, 1, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(5440, Time(1, 1, 2, 100).toPPQ(ts).fCount);
    ASSERT_EQ(13120, Time(1, 2, 2, 100).toPPQ(ts).fCount);
    ASSERT_EQ(51520, Time(2, 2, 2, 100).toPPQ(ts).fCount);

    ASSERT_EQ(Time(1, 1, 1, 100000).toPPQ(ts).fCount, Time(1, 1, 1, 100000).normalize(ts).toPPQ(ts).fCount);
  }
  {
    // 5/16
    auto ts = TimeSignature(5, 16);
    ASSERT_EQ(0, Time(1, 1, 1, 0).toPPQ(ts).fCount);
    ASSERT_EQ(1600, Time(1, 1, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(5440, Time(1, 1, 2, 100).toPPQ(ts).fCount);
    ASSERT_EQ(9280, Time(1, 2, 2, 100).toPPQ(ts).fCount);
    ASSERT_EQ(28480, Time(2, 2, 2, 100).toPPQ(ts).fCount);


    ASSERT_EQ(28480, Time(2, 2, 2, 100).toPPQ(ts).fCount);
    auto time = Time(1, 1, 1, 50) + sequencer::Duration(1, 1, 1, 50);
    ASSERT_EQ(28480, time.toPPQ(ts).fCount);
  }
}

// Duration.toPPQ
TEST(Duration, toPPQ)
{
  {
    // 4/4
    auto ts = TimeSignature(4,4);
    ASSERT_EQ(0, sequencer::Duration().toPPQ(ts).fCount);
    ASSERT_EQ(0, sequencer::Duration(0, 0, 0, 0).toPPQ(ts).fCount);
    ASSERT_EQ(1600, sequencer::Duration(0, 0, 0, 100).toPPQ(ts).fCount);
    ASSERT_EQ(5440, sequencer::Duration(0, 0, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(20800, sequencer::Duration(0, 1, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(82240, sequencer::Duration(1, 1, 1, 100).toPPQ(ts).fCount);
  }

  {
    // 5/8
    auto ts = TimeSignature(5,8);
    ASSERT_EQ(0, sequencer::Duration(0, 0, 0, 0).toPPQ(ts).fCount);
    ASSERT_EQ(1600, sequencer::Duration(0, 0, 0, 100).toPPQ(ts).fCount);
    ASSERT_EQ(5440, sequencer::Duration(0, 0, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(13120, sequencer::Duration(0, 1, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(51520, sequencer::Duration(1, 1, 1, 100).toPPQ(ts).fCount);
  }

  {
    // 5/16
    auto ts = TimeSignature(5,16);
    ASSERT_EQ(0, sequencer::Duration(0, 0, 0, 0).toPPQ(ts).fCount);
    ASSERT_EQ(1600, sequencer::Duration(0, 0, 0, 100).toPPQ(ts).fCount);
    ASSERT_EQ(5440, sequencer::Duration(0, 0, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(9280, sequencer::Duration(0, 1, 1, 100).toPPQ(ts).fCount);
    ASSERT_EQ(28480, sequencer::Duration(1, 1, 1, 100).toPPQ(ts).fCount);
  }}

// Sequencer.executeEvents
TEST(Track, executeEvents)
{
  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
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

  auto checkEveryBatch = [&expectedPlayBatchStartPos, &checkEveryBatchCount](Motherboard &, TJBox_Int64 iPlayBatchStartPos, TJBox_Int64 iPlayBatchEndPos, TJBox_UInt16 iAtFrameIndex) {
    ASSERT_EQ(expectedPlayBatchStartPos, iPlayBatchStartPos);
    ASSERT_EQ(0, iAtFrameIndex);
    expectedPlayBatchStartPos = iPlayBatchEndPos;
    checkEveryBatchCount++;
  };

  // at 44100, 1 batch is 44.582313 ppq

  // sequencer::Time(1,1,1,100) | PPQ = 1600 | Batch 35 [1560, 1605) | Frame Index (1600 - 1560) / (1605 - 1560) * 64 = 56
  ASSERT_EQ(1600, sequencer::Time(1,1,1,100).toPPQ(TimeSignature()).fCount);

  // sequencer::Time(1,1,1,50) | PPQ = 800 | Batch 17 [758, 802) | Frame Index (800 - 758) / (802 - 758) * 64 = 61
  ASSERT_EQ(800, sequencer::Time(1,1,1,50).toPPQ(TimeSignature()).fCount);

  // sequencer::Duration(0,0,1,0) | PPQ = 3840
  ASSERT_EQ(3840, sequencer::Duration(0,0,1,0).toPPQ(TimeSignature()).fCount);

  // sequencer::Duration(0,0,0,200) | PPQ = 3200
  ASSERT_EQ(3200, sequencer::Duration(0,0,0,200).toPPQ(TimeSignature()).fCount);

  tester.getSequencerTrack()
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

  tester.newTimeline().play(sequencer::Duration::k1Beat);

  // 345: see Duration.Conversion test
  ASSERT_EQ(345, checkEveryBatchCount);

  ASSERT_EQ("batch=17,pos=758,note=61,vel=102,idx=61\n"
            "batch=35,pos=1560,note=60,vel=101,idx=56\n"
            "batch=35,pos=1560,note=62,vel=103,idx=56\n"
            "batch=89,pos=3968,note=61,vel=0,idx=0\n"
            "batch=107,pos=4770,note=62,vel=0,idx=0\n"
            "batch=122,pos=5439,note=60,vel=0,idx=0\n", tester.device()->fOutput);
}

}