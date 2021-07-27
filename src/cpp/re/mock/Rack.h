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
#ifndef __Pongasoft_re_mock_rack_h__
#define __Pongasoft_re_mock_rack_h__

#include <map>
#include "Motherboard.h"
#include "ObjectManager.hpp"

namespace re::mock {

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
    struct StereoAudioOutSocket { AudioOutSocket fLeft; AudioOutSocket fRight; };
    struct StereoAudioInSocket { AudioInSocket fLeft; AudioInSocket fRight; };

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
    inline void use(std::function<void (Motherboard &)> iCallback) { fImpl->use(std::move(iCallback)); }
    inline void use(std::function<void ()> iCallback) {
      use([callback = std::move(iCallback)](auto &motherboard) { callback(); });
    }

    AudioOutSocket getAudioOutSocket(std::string const &iSocketName) const;
    StereoAudioOutSocket getStereoAudioOutSocket(std::string const &iLeftSocketName, std::string const &iRightSocketName) const;
    AudioInSocket getAudioInSocket(std::string const &iSocketName) const;
    StereoAudioInSocket getStereoAudioInSocket(std::string const &iLeftSocketName, std::string const &iRightSocketName) const;
    CVOutSocket getCVOutSocket(std::string const &iSocketName) const;
    CVInSocket getCVInSocket(std::string const &iSocketName) const;

    inline TJBox_Value getValue(std::string const &iPropertyPath) const { return motherboard().getValue(iPropertyPath); }
    inline void setValue(std::string const &iPropertyPath, TJBox_Value const &iValue) { motherboard().setValue(iPropertyPath, iValue); }
    inline bool getBool(std::string const &iPropertyPath) const { return motherboard().getBool(iPropertyPath); }
    inline void setBool(std::string const &iPropertyPath, bool iValue) { motherboard().setBool(iPropertyPath, iValue); }

    inline std::string toString(TJBox_Value const &iValue) { return motherboard().toString(iValue); }
    inline std::string toString(std::string const &iPropertyPath) { return motherboard().toString(iPropertyPath); }
    inline std::string toString(TJBox_PropertyRef const &iPropertyRef) const { return motherboard().toString(iPropertyRef); }
    inline std::string getObjectPath(TJBox_ObjectRef iObjectRef) const { return motherboard().getObjectPath(iObjectRef); }

    template<typename T = TJBox_Float64>
    T getNum(std::string const &iPropertyPath) const { return motherboard().getNum<T>(iPropertyPath); }
    template<typename T = TJBox_Float64>
    void setNum(std::string const &iPropertyPath, T iValue) { motherboard().setNum<T>(iPropertyPath, iValue);}

    inline TJBox_Float64 getCVSocketValue(std::string const &iSocketPath) const { return motherboard().getCVSocketValue(iSocketPath); }
    inline void setCVSocketValue(std::string const &iSocketPath, TJBox_Float64 iValue) { motherboard().setCVSocketValue(iSocketPath, iValue); }
    inline TJBox_Float64 getCVSocketValue(CVSocket const &iSocket) const { return motherboard().getCVSocketValue(iSocket.fSocketRef); }
    inline void setCVSocketValue(CVSocket const &iSocket, TJBox_Float64 iValue) const { motherboard().setCVSocketValue(iSocket.fSocketRef, iValue); }

    Motherboard::DSPBuffer getDSPBuffer(std::string const &iAudioSocketPath) const { return motherboard().getDSPBuffer(iAudioSocketPath); }
    void setDSPBuffer(std::string const &iAudioSocketPath, Motherboard::DSPBuffer iBuffer) { motherboard().setDSPBuffer(iAudioSocketPath, std::move(iBuffer)); }
    Motherboard::DSPBuffer getDSPBuffer(AudioSocket const &iSocket) const { return motherboard().getDSPBuffer(iSocket.fSocketRef); }
    void setDSPBuffer(AudioSocket const &iSocket, Motherboard::DSPBuffer iBuffer) { motherboard().setDSPBuffer(iSocket.fSocketRef, std::move(iBuffer)); }

    template<typename T>
    inline T *getNativeObjectRW(std::string const &iPropertyPath) const { return motherboard().getNativeObjectRW<T>(iPropertyPath); }

    template<typename T>
    inline const T *getNativeObjectRO(std::string const &iPropertyPath) const { return motherboard().getNativeObjectRO<T>(iPropertyPath); }


    template<typename T>
    inline T* getInstance() const { return motherboard().getInstance<T>(); }

    inline int getInstanceId() const { return fImpl->fId; }

    friend class Rack;

  protected:
    Motherboard &motherboard() const { return *fImpl->fMotherboard.get(); }

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
    void use(std::function<void (Motherboard &)> iCallback);

    friend class Rack;
    friend class Rack::Extension;

  private:
    ExtensionImpl(int id, Rack *iRack, std::unique_ptr<Motherboard> iMotherboard) :
      fId{id}, fRack{iRack}, fMotherboard{std::move(iMotherboard)} {};

    void wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket);
    void wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket);
    std::optional<Extension::AudioInSocket> unwire(Extension::AudioOutSocket const &iOutSocket);
    std::optional<Extension::AudioOutSocket> unwire(Extension::AudioInSocket const &iInSocket);
    std::optional<Extension::CVInSocket> unwire(Extension::CVOutSocket const &iOutSocket);
    std::optional<Extension::CVOutSocket> unwire(Extension::CVInSocket const &iInSocket);

    std::set<int> const &getDependents() const;

  private:
    int fId;
    [[maybe_unused]] Rack *fRack; // unused at this time...
    std::unique_ptr<Motherboard> fMotherboard;
    std::vector<Extension::AudioWire> fAudioOutWires{};
    std::vector<Extension::AudioWire> fAudioInWires{};
    std::vector<Extension::CVWire> fCVOutWires{};
    std::vector<Extension::CVWire> fCVInWires{};
    mutable std::optional<std::set<int>> fDependents{};
  };

public:
  Rack(int iSampleRate = 44100);

  Extension newExtension(Config const &iConfig);

  template<typename Device>
  ExtensionDevice<Device> newDevice(DeviceConfig<Device> const &iConfig);

  void wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket);
  void unwire(Extension::AudioOutSocket const &iOutSocket);
  void wire(Extension::StereoAudioOutSocket const &iOutSocket, Extension::StereoAudioInSocket const &iInSocket);
  void unwire(Extension::StereoAudioOutSocket const &iOutSocket);
  void wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket);
  void unwire(Extension::CVOutSocket const &iOutSocket);

  void nextFrame();

  static Motherboard &currentMotherboard();

  template<typename Device>
  ExtensionDevice<Device> getDevice(int iExtensionId);

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
// Rack::newDevice
//------------------------------------------------------------------------
template<typename Device>
Rack::ExtensionDevice<Device> Rack::newDevice(DeviceConfig<Device> const &iConfig)
{
  return ExtensionDevice<Device>(newExtension(iConfig.getConfig()).fImpl);
}

//------------------------------------------------------------------------
// Rack::getDevice
//------------------------------------------------------------------------
template<typename Device>
Rack::ExtensionDevice<Device> Rack::getDevice(int iExtensionId)
{
  return Rack::ExtensionDevice<Device>{ fExtensions.get(iExtensionId) };
}

}

#endif //__Pongasoft_re_mock_rack_h__