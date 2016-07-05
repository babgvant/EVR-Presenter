//////////////////////////////////////////////////////////////////////////
//
// PresentEngine.h: Defines the D3DPresentEngine object.
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//
//////////////////////////////////////////////////////////////////////////

/*	Modifications made from the sample EVR Presenter are covered under
 *
 *      Copyright (C) 2014 Andrew Van Til
 *      http://babgvant.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

 //#include <D3dx9core.h>

#include "IEVRCPSettings.h"

 //-----------------------------------------------------------------------------
 // D3DPresentEngine class
 //
 // This class creates the Direct3D device, allocates Direct3D surfaces for
 // rendering, and presents the surfaces. This class also owns the Direct3D
 // device manager and provides the IDirect3DDeviceManager9 interface via
 // GetService.
 //
 // The goal of this class is to isolate the EVRCustomPresenter class from
 // the details of Direct3D as much as possible.
 //-----------------------------------------------------------------------------

#define MSDK_MEMCPY_VAR(dstVarName, src, count) memcpy_s(&(dstVarName), sizeof(dstVarName), (src), (count))
const DWORD PRESENTER_BUFFER_COUNT = 3;

#define MSDK_ALIGN16(value)                      (((value + 15) >> 4) << 4) // round up to a multiple of 16
#define MSDK_ALIGN32(value)                      (((value + 31) >> 5) << 5) // round up to a multiple of 32

const UINT VIDEO_REQUIED_OP = DXVA2_VideoProcess_YUV2RGB |
								DXVA2_VideoProcess_StretchX |
								DXVA2_VideoProcess_StretchY |
								DXVA2_VideoProcess_SubRects |
								DXVA2_VideoProcess_SubStreams;

const D3DFORMAT VIDEO_RENDER_TARGET_FORMAT = D3DFMT_X8R8G8B8;
const D3DFORMAT VIDEO_MAIN_FORMAT = D3DFMT_YUY2;
const D3DFORMAT VIDEO_SUB_FORMAT = D3DFORMAT('VUYA'); // AYUV
const DWORD DXVA_RENDER_TARGET = DXVA2_VideoProcessorRenderTarget; 
const UINT BACK_BUFFER_COUNT = 1;
const UINT SUB_STREAM_COUNT = 1;
const UINT DWM_BUFFER_COUNT = 4;
const BYTE DEFAULT_PLANAR_ALPHA_VALUE = 0xFF;

extern "C" const GUID __declspec(selectany) DXVA2_VideoProcProgressiveDevice =
{ 0x5a54a0c9, 0xc7ec, 0x4bd9,{ 0x8e, 0xde, 0xf3, 0xc7, 0x5d, 0xc4, 0x39, 0x3b } };

class D3DPresentEngine : public SchedulerCallback
{
public:

  // State of the Direct3D device.
  enum DeviceState
  {
    DeviceOK,
    DeviceReset,    // The device was reset OR re-created.
    DeviceRemoved,  // The device was removed.
  };

  D3DPresentEngine(HRESULT& hr);
  virtual ~D3DPresentEngine();

  // GetService: Returns the IDirect3DDeviceManager9 interface.
  // (The signature is identical to IMFGetService::GetService but 
  // this object does not derive from IUnknown.)
  virtual HRESULT GetService(REFGUID guidService, REFIID riid, void** ppv);
  virtual HRESULT CheckFormat(D3DFORMAT format);

  // Video window / destination rectangle:
  // This object implements a sub-set of the functions defined by the 
  // IMFVideoDisplayControl interface. However, some of the method signatures 
  // are different. The presenter's implementation of IMFVideoDisplayControl 
  // calls these methods.
  HRESULT SetVideoWindow(HWND hwnd);
  HWND    GetVideoWindow() const { return m_hwnd; }
  HRESULT SetDestinationRect(const RECT& rcDest);
  RECT    GetDestinationRect() const { return m_rcDestRect; };

  HRESULT CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList& videoSampleQueue);
  void    ReleaseResources();

  HRESULT CheckDeviceState(DeviceState *pState);
  HRESULT PresentSample(IMFSample* pSample, LONGLONG llTarget, LONGLONG timeDelta, LONGLONG remainingInQueue, LONGLONG frameDurationDiv4);

  UINT    RefreshRate() const { return m_DisplayMode.RefreshRate; }
  UINT    Width() const { return m_DisplayMode.Width; }
  UINT    Height() const { return m_DisplayMode.Height; }

  HRESULT CreateSurface(UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface9** ppSurface);
  RECT ScaleRectangle(const RECT& input, const RECT& src, const RECT& dst);
  bool ClipToSurface(IDirect3DSurface9* pSurface, RECT s, LPRECT d);

  STDMETHODIMP SetInt(EVRCPSetting setting, int value) {
    HRESULT hr = S_OK;

    switch (setting)
    {
    case EVRCP_SETTING_POSITION_OFFSET:
      if (value > 0 && value < 100)
        m_iPositionOffset = value;
      else
        return E_INVALIDARG;
      break;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP GetInt(EVRCPSetting setting, int* value) {
    HRESULT hr = S_OK;

    switch (setting)
    {
    case EVRCP_SETTING_POSITION_OFFSET:
      *value = m_iPositionOffset;
      break;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP SetBool(EVRCPSetting setting, bool value) {
    HRESULT hr = S_OK;
    
    switch (setting)
    {
    case EVRCP_SETTING_REQUEST_OVERLAY:
      m_bRequestOverlay = value;
      break;
    case EVRCP_SETTING_POSITION_FROM_BOTTOM:
      m_bPositionFromBottom = value;
      break;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP GetBool(EVRCPSetting setting, bool* value) {
    HRESULT hr = S_OK;
    CheckPointer(value, E_POINTER);
    
    switch (setting)
    {
    case EVRCP_SETTING_REQUEST_OVERLAY:
      *value = m_bRequestOverlay;
      break;
    case EVRCP_SETTING_POSITION_FROM_BOTTOM:
      *value = m_bPositionFromBottom;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }

  void SetSubtitle(IDirect3DSurface9 *pSurfaceSubtitle, const RECT& src, const RECT& dst) {
    AutoLock lock(m_SubtitleLock);

    SAFE_RELEASE(m_pSurfaceSubtitle);
    if (pSurfaceSubtitle)
    {
      m_pSurfaceSubtitle = pSurfaceSubtitle;
      m_rcSubSrcRect = src;
      m_rcSubDstRect = dst;
    }
  }

  HRESULT CreateSubSurface(UINT Width, UINT Height, IDirect3DSurface9** ppSurface)
  {
    return CreateSurface(Width, Height, m_VideoSubFormat, ppSurface);
  }


protected:
  HRESULT InitializeD3D();
  //HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP);
  HRESULT CreateD3DDevice();
  HRESULT CreateD3DSample(IDirect3DSwapChain9 *pSwapChain, IMFSample **ppVideoSample);
  HRESULT UpdateDestRect();

  // A derived class can override these handlers to allocate any additional D3D resources.
  virtual HRESULT OnCreateVideoSamples(D3DPRESENT_PARAMETERS& pp) { return S_OK; }
  virtual void    OnReleaseResources() { }

  virtual HRESULT PresentSurface(IDirect3DSurface9* pSurface);
  virtual HRESULT PresentSwapChain(IDirect3DSwapChain9* pSwapChain, IDirect3DSurface9* pSurface);
  virtual void    PaintFrameWithGDI();
  virtual void    BlackBackBuffer();

protected:
  UINT                        m_DeviceResetToken;     // Reset token for the D3D device manager.
  int							            m_bufferCount;
  int							            m_SampleWidth;
  int							            m_SampleHeight;
  bool                        m_bRequestOverlay;
  char                       m_AdapterName[MAX_DEVICE_IDENTIFIER_STRING];

  HWND                        m_hwnd;                 // Application-provided destination window.
  RECT                        m_rcDestRect;           // Destination rectangle.
  D3DDISPLAYMODE              m_DisplayMode;          // Adapter's display mode.
  RECT                        m_rcSubSrcRect;
  RECT                        m_rcSubDstRect;

  CritSec                     m_ObjectLock;           // Thread lock for the D3D device.
  CritSec                     m_SubtitleLock;           // Thread lock for the subtitle surface.

  // COM interfaces
  IDirect3D9Ex                *m_pD3D9;
  IDirect3DDevice9Ex          *m_pDevice;
  IDirect3DDeviceManager9     *m_pDeviceManager;        // Direct3D device manager.
  IDirect3DSurface9           *m_pSurfaceRepaint;       // Surface for repaint requests.
  IDirect3DSurface9           *m_pSurfaceSubtitle;       // Surface for repaint requests.

  int m_DroppedFrames;
  int m_GoodFrames;
  int m_FramesInQueue;
  double m_AvgTimeDelta;

  // various structures for DXVA2 calls
  DXVA2_VideoDesc                 m_VideoDesc;
  DXVA2_VideoProcessBltParams     m_BltParams;
  DXVA2_VideoSample               m_Sample[2] = { 0 };

  IDirectXVideoProcessorService   *m_pDXVAVPS;            // Service required to create video processors
  IDirectXVideoProcessor          *m_pDXVAVP;
  IDirect3DSurface9               *m_pRenderSurface;      // The surface which is passed to render
  IDirect3DSurface9               *m_pMixerSurfaces[PRESENTER_BUFFER_COUNT]; // The surfaces, which are used by mixer
  DXVA2_VideoProcessorCaps        m_VPCaps = { 0 };

private: // disallow copy and assign
  D3DPresentEngine(const D3DPresentEngine&);
  void operator=(const D3DPresentEngine&);

  //ID3DXFont* pFont;
  bool				        m_bPositionFromBottom;
  int                       m_iPositionOffset;
  D3DFORMAT                 m_VideoSubFormat;
  bool				        m_bProcessSubs;
};