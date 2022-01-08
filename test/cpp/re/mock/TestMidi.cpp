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

#include <gtest/gtest.h>
#include <re/mock/FileManager.h>
#include <re/mock/Errors.h>
#include <re/mock/DeviceTesters.h>
#include <re_mock_build.h>

namespace re::mock::Test {

using namespace mock;

// Midi.NotExists
TEST(Midi, NotExists)
{
  auto midiFile = FileManager::loadMidi(ConfigFile{"/not exists/foo.mid"});

  ASSERT_EQ(std::nullopt, midiFile);
}

// Midi.Tempo
TEST(Midi, Tempo)
{
  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      auto const transport = JBox_GetMotherboardObjectRef("/transport");
      auto const transportPlayPos = JBox_GetNumber(JBox_LoadMOMPropertyByTag(transport, kJBox_TransportPlayPos));

      auto noteStates = JBox_GetMotherboardObjectRef("/note_states");
      for(int i = 0; i < iDiffCount; i++)
      {
        auto diff = iPropertyDiffs[i];
        if(diff.fPropertyRef.fObject == noteStates)
        {
          auto note = JBox_AsNoteEvent(diff);
          // with tempo == 187.5, it makes 1ppq == 1 sample == 1 fAtFrameIndex => we can simply add transportPlayPos and diff.fAtFrameIndex
          auto time = sequencer::Time::from(transportPlayPos + diff.fAtFrameIndex).normalize(sequencer::TimeSignature{});
          fOutput += fmt::printf("%s | %d | %d\n", time.toString(), note.fNoteNumber, note.fVelocity);
        }
      }

      fBatchCount++;
    }

    int fBatchCount{};
    std::string fOutput{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton(DeviceType::kInstrument)
    .accept_notes(true)
    .rtc(Config::rt_input_setup_notify("/note_states/*"))
  ;

  auto tester = InstrumentTester<Device>(c, 48000);

  auto playMidi = [&tester](std::string iMidiFile, int iTrack, int iExpectedTempo, std::string iExpectedOutput) {
    tester.getSequencerTrack().reset(); // removes all notes from sequencer track
    tester.loadMidi(ConfigFile{iMidiFile}, iTrack); // load midi notes
    ASSERT_EQ(iExpectedTempo, static_cast<int>(std::round(tester.rack().getTransportTempo())));
    tester.rack().setTransportTempo(187.5); // change tempo to 187.5 because it makes 1ppq == 1 sample == 1 fAtFrameIndex
    tester.nextBatch(); // we run 1 batch to make sure that everything is initialized properly (note stop generates 128 diffs)
    tester.device()->fOutput = ""; // clear output
    tester.rack().setTransportPlayPos(0); // reset transport position

    // play for 2 bars
    tester.play(sequencer::Duration::k1Bar * 2);

    ASSERT_EQ(iExpectedOutput, tester.device()->fOutput);
  };

  auto Reason_1track_99tempo = fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "midi", "Reason_1track_99tempo.mid");
  auto expected_Reason_1track_99tempo = R"(1.1.1.0 | 60 | 100
1.1.1.120 | 60 | 0
1.2.1.0 | 62 | 101
1.2.3.0 | 62 | 0
1.3.1.0 | 64 | 102
1.3.3.0 | 65 | 103
1.4.1.0 | 64 | 0
2.1.1.0 | 65 | 0
2.2.3.0 | 67 | 104
2.3.3.0 | 67 | 0
)";

  playMidi(Reason_1track_99tempo, -1, 99, expected_Reason_1track_99tempo);

  auto Reason_2track_99tempo = fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "midi", "Reason_2track_99tempo.mid");
  auto expected_Reason_2track_99tempo = R"(1.1.1.0 | 60 | 100
1.1.1.120 | 60 | 0
1.2.1.0 | 62 | 101
1.2.3.0 | 62 | 0
1.3.1.0 | 64 | 102
1.3.3.0 | 65 | 103
1.4.1.0 | 64 | 0
2.1.1.0 | 65 | 0
2.1.1.0 | 72 | 110
2.2.3.0 | 67 | 104
2.3.1.0 | 72 | 0
2.3.3.0 | 67 | 0
)";

  auto expected_Reason_2track_99tempo_track2 = R"(2.1.1.0 | 72 | 110
2.3.1.0 | 72 | 0
)";

  // play all tracks (track 1 + track 2)
  playMidi(Reason_2track_99tempo, -1, 99, expected_Reason_2track_99tempo);

  // play only track 1 (which is the same as Reason_1track_99tempo)
  playMidi(Reason_2track_99tempo, 1, 99, expected_Reason_1track_99tempo);

  // play only track 2 (which is the same as just one note)
  playMidi(Reason_2track_99tempo, 2, 99, expected_Reason_2track_99tempo_track2);

  // Logic uses a ppq of 480 (vs 15360 for Reason) so conversion will happen
  auto Logic_1track_99tempo = fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "midi", "Logic_1track_99tempo.mid");

  playMidi(Logic_1track_99tempo, -1, 99, expected_Reason_1track_99tempo);

  auto Logic_2track_99tempo = fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "midi", "Logic_2track_99tempo.mid");

  // play all tracks (track 1 + track 2)
  playMidi(Logic_2track_99tempo, -1, 99, expected_Reason_2track_99tempo);

  // play only track 1 (which is the same as Reason_1track_99tempo)
  playMidi(Logic_2track_99tempo, 1, 99, expected_Reason_1track_99tempo);
  ASSERT_EQ(expected_Reason_1track_99tempo, tester.device()->fOutput);

  // play only track 2 (which is the same as just one note)
  playMidi(Logic_2track_99tempo, 2, 99, expected_Reason_2track_99tempo_track2);

  // Make sure that there is an exception if we try to read a non existent track
  ASSERT_THROW(tester.loadMidi(ConfigFile{Reason_1track_99tempo}, 3), Exception);
}



}