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

// EffectTester.Usage
TEST(EffectTester, Usage)
{
  EffectTester<MAUPst> tester(MAUPst::CONFIG);

  // device is not wired yet!
  ASSERT_EQ(tester.nextFrame(MockAudioDevice::buffer(1.0, 2.0)), MockAudioDevice::buffer(0, 0));

  // wire main inputs
  tester.setMainIn(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);

  // device not fully wired yet (only IN is wired)
  ASSERT_EQ(tester.nextFrame(MockAudioDevice::buffer(3.0, 4.0)), MockAudioDevice::buffer(0, 0));

  // that means the internal buffer of the effect gets the input, but does not send it to the output
  ASSERT_EQ(tester.getDevice()->fBuffer, MockAudioDevice::buffer(3.0, 4.0));

  // wire main outputs
  tester.setMainOut(MAUPst::LEFT_SOCKET, MAUPst::RIGHT_SOCKET);

  // device is fully wired
  ASSERT_EQ(tester.nextFrame(MockAudioDevice::buffer(5.0, 6.0)), MockAudioDevice::buffer(5.0, 6.0));
}

// InstrumentTester.Usage
TEST(InstrumentTester, Usage)
{
  InstrumentTester<MAUSrc> tester(MAUSrc::CONFIG);

  tester.getDevice()->fBuffer = MockAudioDevice::buffer(1.0, 2.0);

  // device is not wired yet!
  ASSERT_EQ(tester.nextFrame(), MockAudioDevice::buffer(0, 0));

  // wire main outputs
  tester.setMainOut(MAUSrc::LEFT_SOCKET, MAUSrc::RIGHT_SOCKET);

  tester.getDevice()->fBuffer = MockAudioDevice::buffer(3.0, 4.0);

  // device is fully wired
  ASSERT_EQ(tester.nextFrame(), MockAudioDevice::buffer(3.0, 4.0));
}

// NotePlayerTester.Usage
TEST(NotePlayerTester, Usage)
{
  NotePlayerTester<MNPPst> tester(MNPPst::CONFIG);

  ASSERT_EQ(MockDevice::NoteEvents{}, tester.getDevice()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.allNotesOff(), tester.nextFrame());
  ASSERT_EQ(MockDevice::NoteEvents{}.allNotesOff(), tester.getDevice()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(69), tester.nextFrame(MockNotePlayer::NoteEvents{}.noteOn(69)));
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(69), tester.getDevice()->fNoteEvents);

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOff(69, 25), tester.nextFrame(MockNotePlayer::NoteEvents{}.noteOff(69, 25)));
  ASSERT_EQ(MockDevice::NoteEvents{}.noteOff(69, 25), tester.getDevice()->fNoteEvents);

  // simulating sequencer note
  tester.getDevice().use([](Motherboard &m) {
    m.setNoteInEvent(70, 120, 3);
  });

  ASSERT_EQ(MockDevice::NoteEvents{}.noteOn(70, 120, 3), tester.nextFrame());
}

}