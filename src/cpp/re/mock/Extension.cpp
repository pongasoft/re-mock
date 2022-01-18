/*
 * Copyright (c) 2022 pongasoft
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

#include "Extension.h"
#include "Rack.h"

namespace re::mock::rack {

//------------------------------------------------------------------------
// Extension::getAudioOutSocket
//------------------------------------------------------------------------
Extension::AudioOutSocket Extension::getAudioOutSocket(std::string const &iSocketName) const
{
  return {{{fImpl->fId, motherboard().getObjectRef(fmt::printf("/audio_outputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Extension::AudioInSocket
//------------------------------------------------------------------------
Extension::AudioInSocket Extension::getAudioInSocket(std::string const &iSocketName) const
{
  return {{{fImpl->fId, motherboard().getObjectRef(fmt::printf("/audio_inputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Extension::getCVOutSocket
//------------------------------------------------------------------------
Extension::CVOutSocket Extension::getCVOutSocket(std::string const &iSocketName) const
{
  return {{{fImpl->fId, motherboard().getObjectRef(fmt::printf("/cv_outputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Extension::getCVInSocket
//------------------------------------------------------------------------
Extension::CVInSocket Extension::getCVInSocket(std::string const &iSocketName) const
{
  return {{{fImpl->fId, motherboard().getObjectRef(fmt::printf("/cv_inputs/%s", iSocketName))}}};
}

//------------------------------------------------------------------------
// Extension::getStereoAudioOutSocket
//------------------------------------------------------------------------
Extension::StereoAudioOutSocket Extension::getStereoAudioOutSocket(std::string const &iLeftSocketName,
                                                                               std::string const &iRightSocketName) const
{
  return { getAudioOutSocket(iLeftSocketName), getAudioOutSocket(iRightSocketName) };
}

//------------------------------------------------------------------------
// Extension::getStereoAudioInSocket
//------------------------------------------------------------------------
Extension::StereoAudioInSocket Extension::getStereoAudioInSocket(std::string const &iLeftSocketName,
                                                                             std::string const &iRightSocketName) const
{
  return { getAudioInSocket(iLeftSocketName), getAudioInSocket(iRightSocketName) };
}

//------------------------------------------------------------------------
// Extension::getNoteOutSocket
//------------------------------------------------------------------------
Extension::NoteOutSocket Extension::getNoteOutSocket() const
{
  return {fImpl->fId};
}

//------------------------------------------------------------------------
// Extension::getNoteInSocket
//------------------------------------------------------------------------
Extension::NoteInSocket Extension::getNoteInSocket() const
{
  return {fImpl->fId};
}

//------------------------------------------------------------------------
// Extension::getInstanceId
//------------------------------------------------------------------------
int Extension::getInstanceId() const
{
  return fImpl->fId;
}

//------------------------------------------------------------------------
// Extension::motherboard
//------------------------------------------------------------------------
Motherboard &Extension::motherboard() const
{
  return *fImpl->fMotherboard.get();
}

//------------------------------------------------------------------------
// Extension::getSequencerTrack
//------------------------------------------------------------------------
sequencer::Track &Extension::getSequencerTrack() const
{
  return fImpl->fSequencerTrack;
}

//------------------------------------------------------------------------
// Extension::loadMidiNotes
//------------------------------------------------------------------------
void Extension::loadMidiNotes(smf::MidiEventList const &iEvents)
{
  fImpl->loadMidiNotes(iEvents);
}

//------------------------------------------------------------------------
// Extension::AudioOutSocket ==
//------------------------------------------------------------------------
bool operator==(Extension::AudioOutSocket const &lhs, Extension::AudioOutSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

//------------------------------------------------------------------------
// Extension::AudioInSocket ==
//------------------------------------------------------------------------
bool operator==(Extension::AudioInSocket const &lhs, Extension::AudioInSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

//------------------------------------------------------------------------
// Extension::AudioWire::overlap
//------------------------------------------------------------------------
bool Extension::AudioWire::overlap(AudioWire const &iWire1, AudioWire const &iWire2)
{
  return iWire1.fFromSocket == iWire2.fFromSocket || iWire1.fToSocket == iWire2.fToSocket;
}

//------------------------------------------------------------------------
// Extension::CVOutSocket ==
//------------------------------------------------------------------------
bool operator==(Extension::CVOutSocket const &lhs, Extension::CVOutSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

//------------------------------------------------------------------------
// Extension::CVInSocket ==
//------------------------------------------------------------------------
bool operator==(Extension::CVInSocket const &lhs, Extension::CVInSocket const &rhs)
{
  return lhs.fExtensionId == rhs.fExtensionId && lhs.fSocketRef == rhs.fSocketRef;
}

//------------------------------------------------------------------------
// Extension::CVWire::overlap
//------------------------------------------------------------------------
bool Extension::CVWire::overlap(CVWire const &iWire1, CVWire const &iWire2)
{
  return iWire1.fFromSocket == iWire2.fFromSocket || iWire1.fToSocket == iWire2.fToSocket;
}


}