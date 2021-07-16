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
protected:
  class ExtensionImpl;

public:
  class Extension
  {
  public:
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
    void use(std::function<void (Motherboard *)> iCallback) { fImpl->use(std::move(iCallback)); }
    inline void use(std::function<void ()> iCallback) {
      use([callback = std::move(iCallback)](auto motherboard) { callback(); });
    }

    AudioOutSocket getAudioOutSocket(std::string const &iSocketName) const { return fImpl->getAudioOutSocket(iSocketName); }
    AudioInSocket getAudioInSocket(std::string const &iSocketName) const { return fImpl->getAudioInSocket(iSocketName); }
    CVOutSocket getCVOutSocket(std::string const &iSocketName) const { return fImpl->getCVOutSocket(iSocketName); }
    CVInSocket getCVInSocket(std::string const &iSocketName) const { return fImpl->getCVInSocket(iSocketName); }

    template<typename T>
    inline T* getInstance() const;

    friend class Rack;

  private:
    explicit Extension(std::shared_ptr<ExtensionImpl> iExtensionImpl) : fImpl{iExtensionImpl} {}

  private:
    std::shared_ptr<ExtensionImpl> fImpl;
  };

  template<typename Device>
  class ExtensionDevice : public Extension
  {
  public:
    Device *operator->() { return getInstance<Device>(); };
    Device const *operator->() const { return getInstance<Device>(); };

    friend class Rack;

  private:
    explicit ExtensionDevice(std::shared_ptr<ExtensionImpl> iExtensionImpl) : Extension{iExtensionImpl} {}
  };

protected:
  class ExtensionImpl
  {
  public:
    void use(std::function<void (Motherboard *)> iCallback);

    Extension::AudioOutSocket getAudioOutSocket(std::string const &iSocketName) const;
    Extension::AudioInSocket getAudioInSocket(std::string const &iSocketName) const;
    Extension::CVOutSocket getCVOutSocket(std::string const &iSocketName) const;
    Extension::CVInSocket getCVInSocket(std::string const &iSocketName) const;

    friend class Rack;

  private:
    ExtensionImpl(int id, Rack *iRack, std::unique_ptr<Motherboard> iMotherboard) :
      fId{id}, fRack{iRack}, fMotherboard{std::move(iMotherboard)} {};

    void wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket);
    void wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket);

    inline std::set<int> const &getDependents() const { return fDependents; }

    template<typename T>
    inline T* getInstance() const { return fMotherboard->template getInstance<T>(); }

  private:
    int fId;
    [[maybe_unused]] Rack *fRack; // unused at this time...
    std::unique_ptr<Motherboard> fMotherboard;
    std::vector<Extension::AudioWire> fAudioWires{};
    std::vector<Extension::CVWire> fCVWires{};
    std::set<int> fDependents{};
  };

public:
  Rack(int iSampleRate = 44100);

  Extension newExtension(Config const &iConfig);
  Extension newExtension(Config::callback_t iConfigCallback);

  template<typename Device>
  ExtensionDevice<Device> newDevice(Config const &iConfig);

  template<typename Device>
  ExtensionDevice<Device> newDevice(Config::callback_t iConfigCallback);

  template<typename Device>
  ExtensionDevice<Device> newDeviceWithDefault(Config::callback_t iDefaultConfigCallback);

  void wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket);
  void wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket);

  void nextFrame();

  static Motherboard &currentMotherboard();

protected:
  void copyAudioBuffers(Extension::AudioWire const &iWire);
  void copyCVValue(Extension::CVWire const &iWire);
  void nextFrame(ExtensionImpl &iExtension);
  void nextFrame(ExtensionImpl &iExtension, std::set<int> &iProcessedExtensions);

protected:
  int fSampleRate;
  ObjectManager<std::shared_ptr<ExtensionImpl>> fExtensions{};
};

//------------------------------------------------------------------------
// Rack::Extension::getInstance
//------------------------------------------------------------------------
template<typename T>
T *Rack::Extension::getInstance() const
{
  return fImpl->template getInstance<T>();
}

//------------------------------------------------------------------------
// Rack::newDevice
//------------------------------------------------------------------------
template<typename Device>
Rack::ExtensionDevice<Device> Rack::newDevice(Config const &iConfig)
{
  return ExtensionDevice<Device>(newExtension(iConfig).fImpl);
}

//------------------------------------------------------------------------
// Rack::newDevice
//------------------------------------------------------------------------
template<typename Device>
Rack::ExtensionDevice<Device> Rack::newDevice(Config::callback_t iConfigCallback)
{
  return newDevice<Device>(Config::with(std::move(iConfigCallback)));
}

//------------------------------------------------------------------------
// Rack::newDeviceWithDefault
//------------------------------------------------------------------------
template<typename Device>
Rack::ExtensionDevice<Device> Rack::newDeviceWithDefault(Config::callback_t iDefaultConfigCallback)
{
  return newDevice<Device>(Config::withDefault<Device>(std::move(iDefaultConfigCallback)));
}

}

#endif //__PongasoftCommon_re_mock_rack_h__