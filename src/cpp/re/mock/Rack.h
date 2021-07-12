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

#pragma once
#ifndef __PongasoftCommon_re_mock_rack_h__
#define __PongasoftCommon_re_mock_rack_h__

#include <map>
#include "Motherboard.h"
#include "LuaJBox.h"
#include "ObjectManager.hpp"

namespace re::mock {

extern LuaJbox jbox;

class Rack
{
public:
  class Extension
  {
  public:
    using Configuration = std::function<void (MotherboardDef &, RealtimeController &, Realtime &)>;

    struct Socket {
      int fExtensionId{};
      TJBox_ObjectRef fSocketRef{};
    };

    struct AudioSocket : public Socket {};
    struct AudioOutSocket : public AudioSocket {};
    struct AudioInSocket : public AudioSocket {};
    struct CVSocket : public Socket {};
    struct CVOutSocket : public CVSocket {};
    struct CVInSocket : public CVSocket {};

    struct AudioWire {
      AudioOutSocket fFromSocket{};
      AudioInSocket fToSocket{};

      static bool overlap(AudioWire const &iWire1, AudioWire const &iWire2);
    };

    struct CVWire {
      CVOutSocket fFromSocket{};
      CVInSocket fToSocket{};

      static bool overlap(CVWire const &iWire1, CVWire const &iWire2);
    };

  public:
    void use(std::function<void (Motherboard *)> iCallback);
    inline void use(std::function<void ()> iCallback) {
      use([&iCallback](auto motherboard) { iCallback(); });
    }

    AudioOutSocket getAudioOutSocket(std::string const &iSocketName) const;
    AudioInSocket getAudioInSocket(std::string const &iSocketName) const;
    CVOutSocket getCVOutSocket(std::string const &iSocketName) const;
    CVInSocket getCVInSocket(std::string const &iSocketName) const;

    friend class Rack;

  private:
    Extension(int id, Rack *iRack, std::unique_ptr<Motherboard> iMotherboard) :
      fId{id}, fRack{iRack}, fMotherboard{std::move(iMotherboard)} {};

    void wire(AudioOutSocket const &iOutSocket, AudioInSocket const &iInSocket);
    void wire(CVOutSocket const &iOutSocket, CVInSocket const &iInSocket);

  private:
    int fId;
    Rack *fRack;
    std::unique_ptr<Motherboard> fMotherboard;
    std::vector<AudioWire> fAudioWires{};
    std::vector<CVWire> fCVWires{};
  };

  Rack(int iSampleRate = 44100);

  std::shared_ptr<Extension> newExtension(Extension::Configuration iConfig);

  void wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket);
  void wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket);

  void nextFrame();

  static Motherboard &currentMotherboard();

protected:
  void copyAudioBuffers(Extension::AudioWire const &iWire);
  void copyCVValue(Extension::CVWire const &iWire);

protected:
  int fSampleRate;
  ObjectManager<std::shared_ptr<Extension>> fExtensions{};
};


}

#endif //__PongasoftCommon_re_mock_rack_h__