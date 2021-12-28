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
      fFrameCount++;
      fNoteEvents.clear();

      auto noteStates = JBox_GetMotherboardObjectRef("/note_states");
      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        if(diff.fPropertyRef.fObject == noteStates)
          fNoteEvents.emplace_back(JBox_AsNoteEvent(diff));
      }
    }

    int fFrameCount{};
    bool fTransportPlaying{};
    int fTransportPlayPos{};
    std::vector<TJBox_NoteEvent> fNoteEvents{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .accept_notes(true)
    .rtc(Config::rt_input_setup_notify("/note_states/*"))
  ;

  auto tester = HelperTester<Device>(c);

  auto checkValue = [](auto iExpectedValue, auto iValue) { ASSERT_EQ(iExpectedValue, iValue); };

  TJBox_Int64 expectedPlayBatchStartPos = 0;
  TJBox_Int64 checkEveryBatchCount = 0;

  auto checkEveryBatch = [&checkValue, &expectedPlayBatchStartPos, &checkEveryBatchCount](Motherboard &, TJBox_Int64 iPlayBatchStartPos, TJBox_Int64 iPlayBatchEndPos, TJBox_UInt16 iAtFrameIndex) {
    checkValue(expectedPlayBatchStartPos, iPlayBatchStartPos);
    checkValue(0, iAtFrameIndex);
    expectedPlayBatchStartPos = iPlayBatchEndPos;
    checkEveryBatchCount++;
  };

  tester.getSequencerTrack()
    .onEveryBatch(checkEveryBatch);

  tester.newTimeline().play(sequencer::Duration::k1Beat);

  // 345: see Duration.Conversion test
  ASSERT_EQ(345, checkEveryBatchCount);
}

}