/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
#pragma once

#include <functional>

extern "C"
{
#include "lib_ip_ctrl/IpCtrl.h"
}

class IpCtrlAdapter : public AL_TIpCtrl
{
public:
  IpCtrlAdapter()
  {
    static const AL_IpCtrlVtable myVtable =
    {
      &sDestroy,
      &sReadRegister,
      &sWriteRegister,
      &sRegisterCallback,
    };

    AL_TIpCtrl::vtable = &myVtable;
  }

  virtual ~IpCtrlAdapter() {}

  virtual void WriteRegister(uint32_t uReg, uint32_t uVal) = 0;
  virtual uint32_t ReadRegister(uint32_t uReg) = 0;
  virtual void RegisterCallBack(std::function<void(void)> callback, uint8_t uNumInt) = 0;

private:
  static void sDestroy(AL_TIpCtrl* handle)
  {
    auto pThis = static_cast<IpCtrlAdapter*>(handle);
    delete pThis;
  }

  static void sWriteRegister(AL_TIpCtrl* handle, uint32_t uReg, uint32_t uVal)
  {
    auto pThis = static_cast<IpCtrlAdapter*>(handle);
    pThis->WriteRegister(uReg, uVal);
  }

  static uint32_t sReadRegister(AL_TIpCtrl* handle, uint32_t uReg)
  {
    auto pThis = static_cast<IpCtrlAdapter*>(handle);
    return pThis->ReadRegister(uReg);
  }

  static void sRegisterCallback(AL_TIpCtrl* handle, AL_PFN_IpCtrl_CallBack pfnCallBack, void* pUserData, uint8_t uNumInt)
  {
    auto pThis = static_cast<IpCtrlAdapter*>(handle);

    auto callback = [=]()
                    {
                      if(pfnCallBack)
                        pfnCallBack(pUserData);
                    };

    pThis->RegisterCallBack(callback, uNumInt);
  }
};

