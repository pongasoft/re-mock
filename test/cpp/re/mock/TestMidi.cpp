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
#include <MidiFile.h>

namespace re::mock::Test {

using namespace mock;

// Midi.Usage
TEST(Midi, Usage)
{
  smf::MidiFile midifile;
  midifile.read("/Volumes/Vault/tmp/com.presteign.Macro-plugin/Macro test d25 granular clicks.mid");
  ASSERT_TRUE(midifile.status());
}

}