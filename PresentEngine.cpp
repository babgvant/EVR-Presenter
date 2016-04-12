//////////////////////////////////////////////////////////////////////////
//
// PresentEngine.cpp: Defines the D3DPresentEngine object.
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

#include "stdafx.h"
#include "EVRPresenter.h"

HRESULT FindAdapter(IDirect3D9 *pD3D9, HMONITOR hMonitor, UINT *puAdapterID);


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

D3DPresentEngine::D3DPresentEngine(HRESULT& hr) :
  m_hwnd(NULL)
  , m_DeviceResetToken(0)
  , m_pD3D9(NULL)
  , m_pDevice(NULL)
  , m_pDeviceManager(NULL)
  , m_pSurfaceRepaint(NULL)
  , m_bufferCount(4)
  , m_pRenderSurface(NULL)
  , m_pDXVAVPS(NULL)
  , m_pDXVAVP(NULL)
  , m_bRequestOverlay(false)
  , m_pSurfaceSubtitle(NULL)
{
  SetRectEmpty(&m_rcDestRect);

  ZeroMemory(&m_DisplayMode, sizeof(m_DisplayMode));
  ZeroMemory(&m_VideoDesc, sizeof(m_VideoDesc));
  ZeroMemory(&m_BltParams, sizeof(m_BltParams));
  ZeroMemory(&m_Sample, sizeof(m_Sample));

  for (UINT i = 0; i < PRESENTER_BUFFER_COUNT; i++)
  {
    m_pMixerSurfaces[i] = NULL;
  }

  //Initialize DXVA structures
  DXVA2_AYUVSample16 color = { 0x8000, 0x8000, 0x1000, 0xffff };

  DXVA2_ExtendedFormat format = { DXVA2_SampleProgressiveFrame,           // SampleFormat
                                  DXVA2_VideoChromaSubsampling_MPEG2,     // VideoChromaSubsampling
                                  DXVA2_NominalRange_Normal,              // NominalRange
                                  DXVA2_VideoTransferMatrix_BT709,        // VideoTransferMatrix
                                  DXVA2_VideoLighting_dim,                // VideoLighting
                                  DXVA2_VideoPrimaries_BT709,             // VideoPrimaries
                                  DXVA2_VideoTransFunc_709                // VideoTransferFunction            
  };

  // init m_VideoDesc structure
  MSDK_MEMCPY_VAR(m_VideoDesc.SampleFormat, &format, sizeof(DXVA2_ExtendedFormat));
  m_VideoDesc.SampleWidth = 256;
  m_VideoDesc.SampleHeight = 256;
  m_VideoDesc.InputSampleFreq.Numerator = 60;
  m_VideoDesc.InputSampleFreq.Denominator = 1;
  m_VideoDesc.OutputFrameFreq.Numerator = 60;
  m_VideoDesc.OutputFrameFreq.Denominator = 1;

  // init m_BltParams structure
  MSDK_MEMCPY_VAR(m_BltParams.DestFormat, &format, sizeof(DXVA2_ExtendedFormat));
  MSDK_MEMCPY_VAR(m_BltParams.BackgroundColor, &color, sizeof(DXVA2_AYUVSample16));

  m_BltParams.BackgroundColor = color;
  m_BltParams.DestFormat = format;
  //m_BltParams.Alpha = DXVA2_Fixed32OpaqueAlpha();

  //// init m_Sample structure
  m_Sample[0].Start = 0;
  m_Sample[0].End = 1;
  m_Sample[0].SampleFormat = format;
  m_Sample[0].PlanarAlpha.Fraction = 0;
  m_Sample[0].PlanarAlpha.Value = 1;

  //
  // Initialize sub stream video sample.
  //
  m_Sample[1] = m_Sample[0];

  // DXVA2_VideoProcess_SubStreamsExtended
  m_Sample[1].SampleFormat = m_Sample[0].SampleFormat;

  // DXVA2_VideoProcess_SubStreams
  m_Sample[1].SampleFormat.SampleFormat = DXVA2_SampleSubStream;

  ZeroMemory(&m_DisplayMode, sizeof(m_DisplayMode));
  m_SampleWidth = -1;
  m_SampleHeight = -1;
  m_DroppedFrames = 0;
  m_GoodFrames = 0;
  m_FramesInQueue = 0;
  m_AvgTimeDelta = 0;

  //pFont = NULL;

  hr = InitializeD3D();

  if (SUCCEEDED(hr))
  {
    hr = CreateD3DDevice();
  }
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

D3DPresentEngine::~D3DPresentEngine()
{
  SAFE_RELEASE(m_pDevice);
  SAFE_RELEASE(m_pSurfaceRepaint);
  SAFE_RELEASE(m_pRenderSurface);
  SAFE_RELEASE(m_pSurfaceSubtitle);
  SAFE_RELEASE(m_pDeviceManager);
  SAFE_RELEASE(m_pD3D9);

  for (int i = 0; i < PRESENTER_BUFFER_COUNT; i++)
  {
    SAFE_RELEASE(m_pMixerSurfaces[i]);
  }

  SAFE_RELEASE(m_pDXVAVPS);
  SAFE_RELEASE(m_pDXVAVP);
}

//-----------------------------------------------------------------------------
// GetService
//
// Returns a service interface from the presenter engine.
// The presenter calls this method from inside it's implementation of 
// IMFGetService::GetService.
//
// Classes that derive from D3DPresentEngine can override this method to return 
// other interfaces. If you override this method, call the base method from the 
// derived class.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::GetService(REFGUID guidService, REFIID riid, void** ppv)
{
  assert(ppv != NULL);

  if (riid == __uuidof(IDirect3DDeviceManager9))
  {
    if (m_pDeviceManager != NULL)
    {
      if (guidService == MR_VIDEO_ACCELERATION_SERVICE)
      {
        //this enables DXVA2 support for the connected video decoder
        return m_pDeviceManager->QueryInterface(riid, ppv);
      }
      else
      {
        *ppv = m_pDeviceManager;
        m_pDeviceManager->AddRef();
        return S_OK;
      }
    }
  }

  return MF_E_UNSUPPORTED_SERVICE;
}


//-----------------------------------------------------------------------------
// CheckFormat
//
// Queries whether the D3DPresentEngine can use a specified Direct3D format.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CheckFormat(D3DFORMAT format)
{
  HRESULT hr = S_OK;

  UINT uAdapter = D3DADAPTER_DEFAULT;
  D3DDEVTYPE type = D3DDEVTYPE_HAL;

  D3DDISPLAYMODE mode;
  D3DDEVICE_CREATION_PARAMETERS params;

  if (m_pDevice)
  {
    CHECK_HR(hr = m_pDevice->GetCreationParameters(&params));

    uAdapter = params.AdapterOrdinal;
    type = params.DeviceType;

  }

  CHECK_HR(hr = m_pD3D9->GetAdapterDisplayMode(uAdapter, &mode));

  CHECK_HR(hr = m_pD3D9->CheckDeviceType(uAdapter, type, mode.Format, format, TRUE));

done:
  return hr;
}



//-----------------------------------------------------------------------------
// SetVideoWindow
// 
// Sets the window where the video is drawn.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::SetVideoWindow(HWND hwnd)
{
  // Assertions: EVRCustomPresenter checks these cases.
  assert(IsWindow(hwnd));
  assert(hwnd != m_hwnd);

  HRESULT hr = S_OK;

  AutoLock lock(m_ObjectLock);

  TRACE((L"SetVideoWindow: %d\n", hwnd));

  m_hwnd = hwnd;

  UpdateDestRect();

  // Recreate the device.
  hr = CreateD3DDevice();

  return hr;
}

//-----------------------------------------------------------------------------
// SetDestinationRect
// 
// Sets the region within the video window where the video is drawn.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::SetDestinationRect(const RECT& rcDest)
{
  if (EqualRect(&rcDest, &m_rcDestRect))
  {
    return S_OK; // No change.
  }

  HRESULT hr = S_OK;

  AutoLock lock(m_ObjectLock);

  TRACE((L"SetDestinationRect: OLD b: %d t: %d l: %d r: %d NEW  b: %d t: %d l: %d r: %d\n", m_rcDestRect.bottom, m_rcDestRect.top, m_rcDestRect.left, rcDest.right, rcDest.bottom, rcDest.top, rcDest.left, rcDest.right));

  PaintFrameWithGDI();

  m_rcDestRect = rcDest;

  //UpdateDestRect();

  return hr;
}



//-----------------------------------------------------------------------------
// CreateVideoSamples
// 
// Creates video samples based on a specified media type.
// 
// pFormat: Media type that describes the video format.
// videoSampleQueue: List that will contain the video samples.
//
// Note: For each video sample, the method creates a swap chain with a
// single back buffer. The video sample object holds a pointer to the swap
// chain's back buffer surface. The mixer renders to this surface, and the
// D3DPresentEngine renders the video frame by presenting the swap chain.
//-----------------------------------------------------------------------------

//HRESULT D3DPresentEngine::CreateVideoSamples(
//    IMFMediaType *pFormat, 
//    VideoSampleList& videoSampleQueue
//    )
//{
//    if (m_hwnd == NULL)
//    {
//        return MF_E_INVALIDREQUEST;
//    }
//
//    if (pFormat == NULL)
//    {
//        return MF_E_UNEXPECTED;
//    }
//
//    HRESULT hr = S_OK;
//    D3DPRESENT_PARAMETERS pp;
//
//    IDirect3DSwapChain9 *pSwapChain = NULL;    // Swap chain
//    IMFSample *pVideoSample = NULL;            // Sampl
//    
//    AutoLock lock(m_ObjectLock);
//
//    ReleaseResources();
//
//    // Get the swap chain parameters from the media type.
//    CHECK_HR(hr = GetSwapChainPresentParameters(pFormat, &pp));
//
//    //UpdateDestRect();
//
//	// Create the video samples.
//    for (int i = 0; i < m_bufferCount; i++)
//    {
//        // Create a new swap chain.
//        CHECK_HR(hr = m_pDevice->CreateAdditionalSwapChain(&pp, &pSwapChain));
//        
//        // Create the video sample from the swap chain.
//        CHECK_HR(hr = CreateD3DSample(pSwapChain, &pVideoSample));
//
//        // Add it to the list.
//        CHECK_HR(hr = videoSampleQueue.InsertBack(pVideoSample));
//
//        // Set the swap chain pointer as a custom attribute on the sample. This keeps
//        // a reference count on the swap chain, so that the swap chain is kept alive
//        // for the duration of the sample's lifetime.
//        CHECK_HR(hr = pVideoSample->SetUnknown(MFSamplePresenter_SampleSwapChain, pSwapChain));
//
//        SAFE_RELEASE(pVideoSample);
//        SAFE_RELEASE(pSwapChain);
//    }
//
//    // Let the derived class create any additional D3D resources that it needs.
//    CHECK_HR(hr = OnCreateVideoSamples(pp));
//
//done:
//    if (FAILED(hr))
//    {
//        ReleaseResources();
//    }
//        
//    SAFE_RELEASE(pSwapChain);
//    SAFE_RELEASE(pVideoSample);
//    return hr;
//}

//-----------------------------------------------------------------------------
// CreateVideoSamples
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList& videoSampleQueue)
{
  if (m_hwnd == NULL)
  {
    return MF_E_INVALIDREQUEST;
  }

  if (pFormat == NULL)
  {
    return MF_E_UNEXPECTED;
  }

  HRESULT     hr = S_OK;
  D3DCOLOR    clrBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);
  IMFSample*  pVideoSample = NULL;
  //HANDLE      hDevice = 0;
  UINT        nWidth(0), nHeight(0);
  //IDirectXVideoProcessorService* pVideoProcessorService = NULL;

  AutoLock lock(m_ObjectLock);

  ReleaseResources();

  UpdateDestRect();

  // Get sizes for allocated surfaces
  CHECK_HR(hr = MFGetAttributeSize(pFormat, MF_MT_FRAME_SIZE, &nWidth, &nHeight));

  // Get device handle
  //CHECK_HR(hr = m_pDeviceManager->OpenDeviceHandle(&hDevice));

  // Get IDirectXVideoProcessorService
  //CHECK_HR(hr = m_pDeviceManager->GetVideoService(hDevice, __uuidof(IDirectXVideoProcessorService), (void**)&pVideoProcessorService));

  // Create IDirect3DSurface9 surface
  CHECK_HR(hr = m_pDXVAVPS->CreateSurface(nWidth, nHeight, PRESENTER_BUFFER_COUNT - 1, D3DFMT_X8R8G8B8, m_VPCaps.InputPool, 0, DXVA2_VideoProcessorRenderTarget, (IDirect3DSurface9 **)&m_pMixerSurfaces, NULL));

  // Create the video samples.
  for (int i = 0; i < PRESENTER_BUFFER_COUNT; i++)
  {
    // Fill it with black.
    CHECK_HR(hr = m_pDevice->ColorFill(m_pMixerSurfaces[i], NULL, clrBlack));

    // Create the sample.
    CHECK_HR(hr = MFCreateVideoSampleFromSurface(m_pMixerSurfaces[i], &pVideoSample));

    pVideoSample->SetUINT32(MFSampleExtension_CleanPoint, 0);

    // Add it to the list.
    hr = videoSampleQueue.InsertBack(pVideoSample);
    SAFE_RELEASE(pVideoSample);
    CHECK_HR(hr);
  }

done:
  //SAFE_RELEASE(pVideoProcessorService);
  //m_pDeviceManager->CloseDeviceHandle(hDevice);

  if (FAILED(hr))
  {
    ReleaseResources();
  }

  return hr;
}

//-----------------------------------------------------------------------------
// ReleaseResources
// 
// Released Direct3D resources used by this object. 
//-----------------------------------------------------------------------------

void D3DPresentEngine::ReleaseResources()
{
  // Let the derived class release any resources it created.
  OnReleaseResources();

  SAFE_RELEASE(m_pSurfaceRepaint);
  SAFE_RELEASE(m_pRenderSurface);

  for (int i = 0; i < PRESENTER_BUFFER_COUNT; i++)
  {
    SAFE_RELEASE(m_pMixerSurfaces[i]);
  }
}


//-----------------------------------------------------------------------------
// CheckDeviceState
// 
// Tests the Direct3D device state.
//
// pState: Receives the state of the device (OK, reset, removed)
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CheckDeviceState(DeviceState *pState)
{
  HRESULT hr = S_OK;

  AutoLock lock(m_ObjectLock);

  // Check the device state. Not every failure code is a critical failure.
  hr = m_pDevice->CheckDeviceState(m_hwnd);

  *pState = DeviceOK;

  switch (hr)
  {
  case S_OK:
  case S_PRESENT_OCCLUDED:
  case S_PRESENT_MODE_CHANGED:
    // state is DeviceOK
    hr = S_OK;
    break;

  case D3DERR_DEVICELOST:
  case D3DERR_DEVICEHUNG:
    // Lost/hung device. Destroy the device and create a new one.
    CHECK_HR(hr = CreateD3DDevice());
    *pState = DeviceReset;
    hr = S_OK;
    break;

  case D3DERR_DEVICEREMOVED:
    // This is a fatal error.
    *pState = DeviceRemoved;
    break;

  case E_INVALIDARG:
    // CheckDeviceState can return E_INVALIDARG if the window is not valid
    // We'll assume that the window was destroyed; we'll recreate the device 
    // if the application sets a new window.
    hr = S_OK;
  }

done:
  return hr;
}

//-----------------------------------------------------------------------------
// PresentSurface
//
// Presents a surface that contains a video frame.
//
// pSurface: Pointer to the surface.

HRESULT D3DPresentEngine::PresentSurface(IDirect3DSurface9* pSurface)
{
  //TRACE((L"PresentSurface\n"));

  HRESULT hr = S_OK;
  RECT target, targetRect;
  UINT sampleCount = 1;

  if (m_hwnd == NULL)
  {
    return MF_E_INVALIDREQUEST;
  }

  if (NULL == m_pDXVAVP)
  {
    return E_FAIL;
  }

  D3DSURFACE_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  hr = pSurface->GetDesc(&desc);

  //scope the lock just around rect retrival
  {
    // Race condition b/w presentation and the rectangle changing size
    AutoLock lock(m_ObjectLock);

    //GetClientRect(m_hwnd, &target);
    target = targetRect = m_rcDestRect;    
  }

  if (ClipToSurface(pSurface, m_Sample[0].SrcRect, &target))
  {
    m_BltParams.TargetRect = target;
    m_Sample[0].DstRect = target;
    //m_Sample.SrcRect = 
    m_Sample[0].SrcSurface = pSurface;

    hr = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pRenderSurface);
    LOG_MSG_IF_FAILED(L"D3DPresentEngine::PresentSurface m_pDevice->GetBackBuffer failed.", hr);
    // process the surface

    if (SUCCEEDED(hr) && m_pRenderSurface)
    {      
      AutoLock lock(m_SubtitleLock);
      //process subtitle
      if (m_pSurfaceSubtitle)
      {
        //D3DSURFACE_DESC sub_desc;
        //ZeroMemory(&sub_desc, sizeof(sub_desc));
        //hr = m_pSurfaceSubtitle->GetDesc(&sub_desc);
        //LOG_MSG_IF_FAILED(L"D3DPresentEngine::PresentSurface  m_pSurfaceSubtitle->GetDesc failed.", hr);
        m_Sample[1].SrcSurface = m_pSurfaceSubtitle;        
        //m_Sample[1].SrcRect.right = sub_desc.Width;
        //m_Sample[1].SrcRect.bottom = sub_desc.Height;
        m_Sample[1].SrcRect = m_rcSubSrcRect;
        m_Sample[1].DstRect = m_rcSubDstRect;

        //m_Sample[1].DstRect.top = abs(target.top - target.bottom) * m_nrcDest.top;
        //m_Sample[1].DstRect.left = abs(target.left- target.right) * m_nrcDest.left;
        //m_Sample[1].DstRect.right = abs(target.left - target.right) * m_nrcDest.right;
        //m_Sample[1].DstRect.bottom = abs(target.top - target.bottom) * m_nrcDest.bottom;
        sampleCount = 2;
      }

      hr = m_pDXVAVP->VideoProcessBlt(m_pRenderSurface, &m_BltParams, m_Sample, sampleCount, NULL);
      LOG_MSG_IF_FAILED(L"D3DPresentEngine::PresentSurface m_pDXVAVP->VideoProcessBlt failed.", hr);
    }

    SAFE_RELEASE(m_pRenderSurface);

    if (SUCCEEDED(hr))
    {
      hr = m_pDevice->PresentEx(&target, &targetRect, m_hwnd, NULL, 0);
      LOG_MSG_IF_FAILED(L"D3DPresentEngine::PresentSurface m_pDevice->PresentEx failed.", hr);
    }

    LOG_MSG_IF_FAILED(L"D3DPresentEngine::PresentSurface failed.", hr);
  }  

  return hr;
}

RECT D3DPresentEngine::ScaleRectangle(const RECT& input, const RECT& src, const RECT& dst)
{
  RECT rect;

  UINT src_dx = src.right - src.left;
  UINT src_dy = src.bottom - src.top;

  UINT dst_dx = dst.right - dst.left;
  UINT dst_dy = dst.bottom - dst.top;

  //
  // Scale input rectangle within src rectangle to dst rectangle.
  //
  rect.left = input.left   * dst_dx / src_dx;
  rect.right = input.right  * dst_dx / src_dx;
  rect.top = input.top    * dst_dy / src_dy;
  rect.bottom = input.bottom * dst_dy / src_dy;

  return rect;
}

bool D3DPresentEngine::ClipToSurface(IDirect3DSurface9* pSurface, RECT s, LPRECT d)
{
  D3DSURFACE_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  if (FAILED(pSurface->GetDesc(&desc))) {
    return false;
  }

  int w = desc.Width, h = desc.Height;
  int sw = s.right - s.left, sh = s.bottom - s.top;
  int dw = d->right - d->left, dh = d->bottom - d->top;

  if (d->left >= w || d->right < 0 || d->top >= h || d->bottom < 0
    || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) {
    s.top=s.bottom=s.left=s.right = 0;
    d->top = d->bottom = d->left = d->right = 0;
    return true;
  }

  if (d->right > w) {
    s.right -= (d->right - w) * sw / dw;
    d->right = w;
  }
  if (d->bottom > h) {
    s.bottom -= (d->bottom - h) * sh / dh;
    d->bottom = h;
  }
  if (d->left < 0) {
    s.left += (0 - d->left) * sw / dw;
    d->left = 0;
  }
  if (d->top < 0) {
    s.top += (0 - d->top) * sh / dh;
    d->top = 0;
  }

  return true;
}

//-----------------------------------------------------------------------------
// PresentSample
//
// Presents a video frame.
//
// pSample:  Pointer to the sample that contains the surface to present. If 
//           this parameter is NULL, the method paints a black rectangle.
// llTarget: Target presentation time.
//
// This method is called by the scheduler and/or the presenter.
//-----------------------------------------------------------------------------
HRESULT D3DPresentEngine::PresentSample(IMFSample* pSample, LONGLONG llTarget, LONGLONG timeDelta, LONGLONG remainingInQueue, LONGLONG frameDurationDiv4)
{
  HRESULT hr = S_OK;

  IMFMediaBuffer* pBuffer = NULL;
  IDirect3DSurface9* pSurface = NULL;
  IDirect3DSwapChain9* pSwapChain = NULL;
  MFTIME sampleDuration = 0;
  BOOL currentSampleIsTooLate = FALSE;

  m_FramesInQueue = remainingInQueue;

  double lastDelta = m_AvgTimeDelta;
  if (m_AvgTimeDelta == 0)
  {
    m_AvgTimeDelta = timeDelta;
  }
  else
  {
    m_AvgTimeDelta = (m_AvgTimeDelta + timeDelta) / 2;
  }

  if (pSample != NULL && lastDelta > m_AvgTimeDelta && (lastDelta - m_AvgTimeDelta) > frameDurationDiv4)
  {
    //TRACE((L"PresentSample drop frame llTarget=%I64d timeDelta=%I64d remainingInQueue=%I64d frameDurationDiv4=%I64d lastDelta=%f m_AvgTimeDelta=%f\n", llTarget, timeDelta, remainingInQueue, frameDurationDiv4, lastDelta, m_AvgTimeDelta));
    m_DroppedFrames++;
    currentSampleIsTooLate = TRUE;
  }
  else
  {
    m_GoodFrames++;
  }

  if (pSample && (!currentSampleIsTooLate || !m_pSurfaceRepaint))
  {
    // Get the buffer from the sample.
    CHECK_HR(hr = pSample->GetBufferByIndex(0, &pBuffer));

    // Get the surface from the buffer.
    CHECK_HR(hr = MFGetService(pBuffer, MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&pSurface));
    CHECK_HR(hr = pSample->GetSampleDuration(&sampleDuration));
    //TRACE((L"PresentSample llTarget=%I64d timeDelta=%I64d remainingInQueue=%I64d frameDurationDiv4=%I64d sampleDuration=%I64d lastDelta=%f m_AvgTimeDelta=%f\n", llTarget, timeDelta, remainingInQueue, frameDurationDiv4, sampleDuration, lastDelta, m_AvgTimeDelta));
  }
  else if (m_pSurfaceRepaint && !currentSampleIsTooLate)
  {
    // Redraw from the last surface.
    pSurface = m_pSurfaceRepaint;
    pSurface->AddRef();
  }

  if (pSurface)
  {
    D3DSURFACE_DESC d;
    CHECK_HR(hr = pSurface->GetDesc(&d));

    m_SampleWidth = d.Width;
    m_SampleHeight = d.Height;

    m_Sample[0].SrcRect.right = d.Width;
    m_Sample[0].SrcRect.bottom = d.Height;

    // Get the swap chain from the surface.
//        CHECK_HR(hr = pSurface->GetContainer(__uuidof(IDirect3DSwapChain9), (LPVOID*)&pSwapChain));

        // Present the swap chain.
        //CHECK_HR(hr = PresentSwapChain(pSwapChain, pSurface));

    CHECK_HR(hr = PresentSurface(pSurface));

    // Store this pointer in case we need to repaint the surface.
    CopyComPointer(m_pSurfaceRepaint, pSurface);
  }
  else
  {
    // No surface. All we can do is paint a black rectangle.
    //PaintFrameWithGDI();
//TRACE((L"PresentSample: Drop frame\n"));
  }

done:
  SAFE_RELEASE(pSwapChain);
  SAFE_RELEASE(pSurface);
  SAFE_RELEASE(pBuffer);

  if (FAILED(hr))
  {
    if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET || hr == D3DERR_DEVICEHUNG)
    {
      // We failed because the device was lost. Fill the destination rectangle.
      PaintFrameWithGDI();

      // Ignore. We need to reset or re-create the device, but this method
      // is probably being called from the scheduler thread, which is not the
      // same thread that created the device. The Reset(Ex) method must be
      // called from the thread that created the device.

      // The presenter will detect the state when it calls CheckDeviceState() 
      // on the next sample.
      hr = S_OK;
    }
  }
  return hr;
}



//-----------------------------------------------------------------------------
// private/protected methods
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// InitializeD3D
// 
// Initializes Direct3D and the Direct3D device manager.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::InitializeD3D()
{
  HRESULT hr = S_OK;

  TRACE((L"InitializeD3D\n"));

  assert(m_pD3D9 == NULL);
  assert(m_pDeviceManager == NULL);

  // Create Direct3D
  CHECK_HR(hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D9));

  // Create the device manager
  CHECK_HR(hr = DXVA2CreateDirect3DDeviceManager9(&m_DeviceResetToken, &m_pDeviceManager));

done:
  return hr;
}

//-----------------------------------------------------------------------------
// CreateD3DDevice
// 
// Creates the Direct3D device.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CreateD3DDevice()
{
  HRESULT     hr = S_OK;
  HWND        hwnd = NULL;
  HMONITOR    hMonitor = NULL;
  UINT        uAdapterID = D3DADAPTER_DEFAULT;
  DWORD       vp = 0;

  D3DCAPS9    ddCaps;
  ZeroMemory(&ddCaps, sizeof(ddCaps));

  IDirect3DDevice9Ex* pDevice = NULL;

  // Hold the lock because we might be discarding an exisiting device.
  AutoLock lock(m_ObjectLock);

  if (!m_pD3D9 || !m_pDeviceManager)
  {
    return MF_E_NOT_INITIALIZED;
  }

  if (m_hwnd)
  {
    hwnd = m_hwnd;
  }
  else
  {
    hwnd = GetDesktopWindow();
  }

  //if (m_rcDestRect.bottom == 0 || m_rcDestRect.right == 0)
  //{
  //  GetClientRect(hwnd, &m_rcDestRect);
  //}

  // Find the monitor for this window.
  if (m_hwnd)
  {
    hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);

    // Find the corresponding adapter.
    CHECK_HR(hr = FindAdapter(m_pD3D9, hMonitor, &uAdapterID));
  }

  // Get the adapter display mode.
  CHECK_HR(hr = m_pD3D9->GetAdapterDisplayMode(uAdapterID, &m_DisplayMode));

  D3DADAPTER_IDENTIFIER9 adapterIdentifier;
  TCHAR strGUID[50];

  if (m_pD3D9->GetAdapterIdentifier(uAdapterID, 0, &adapterIdentifier) == S_OK) {
    if ((::StringFromGUID2(adapterIdentifier.DeviceIdentifier, strGUID, 50) > 0)) {
      strncpy(m_AdapterName, adapterIdentifier.Description, MAX_DEVICE_IDENTIFIER_STRING);
       TRACE((L"Adapter Found: %S\n", m_AdapterName));
    }
  }

  
  TRACE((L"InitializeD3D\n"));

  // Get the device caps for this adapter.
  CHECK_HR(hr = m_pD3D9->GetDeviceCaps(uAdapterID, D3DDEVTYPE_HAL, &ddCaps));

  if (ddCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
  {
    vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
  }
  else
  {
    vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
  }

  // Note: The presenter creates additional swap chains to present the
  // video frames. Therefore, it does not use the device's implicit 
  // swap chain, so the size of the back buffer here is 1 x 1.

  D3DPRESENT_PARAMETERS pp;
  ZeroMemory(&pp, sizeof(pp));

  pp.BackBufferWidth = m_DisplayMode.Width;// abs(m_rcDestRect.right - m_rcDestRect.left);//1;
  pp.BackBufferHeight = m_DisplayMode.Height;// abs(m_rcDestRect.bottom - m_rcDestRect.top);//1;
  //pp.BackBufferWidth = abs(m_rcDestRect.right - m_rcDestRect.left);//1;
  //pp.BackBufferHeight = abs(m_rcDestRect.bottom - m_rcDestRect.top);//1;
  pp.Windowed = TRUE;
  pp.SwapEffect = D3DSWAPEFFECT_COPY;
  pp.BackBufferFormat = D3DFMT_X8R8G8B8;// D3DFMT_UNKNOWN;
  pp.BackBufferCount = 1; //added
  pp.hDeviceWindow = hwnd;
  pp.Flags = D3DPRESENTFLAG_VIDEO | D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

  if (m_bRequestOverlay && (ddCaps.Caps & D3DCAPS_OVERLAY))
  {
    pp.SwapEffect = D3DSWAPEFFECT_OVERLAY;
  }

  // Create the device.
  CHECK_HR(hr = m_pD3D9->CreateDeviceEx(
    uAdapterID,
    D3DDEVTYPE_HAL,
    pp.hDeviceWindow,
    vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
    &pp,
    NULL,
    &pDevice
    ));

  CHECK_HR(hr = pDevice->ResetEx(&pp, NULL));

  CHECK_HR(hr = pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0));

  // Reset the D3DDeviceManager with the new device 
  CHECK_HR(hr = m_pDeviceManager->ResetDevice(pDevice, m_DeviceResetToken));

  SAFE_RELEASE(m_pDXVAVPS);
  SAFE_RELEASE(m_pDXVAVP);

  // Create DXVA2 Video Processor Service.
  CHECK_HR(hr = DXVA2CreateVideoService(pDevice, _uuidof(IDirectXVideoProcessorService), (void**)&m_pDXVAVPS));

  UINT count;
  GUID* guids = NULL;

  CHECK_HR(hr = m_pDXVAVPS->GetVideoProcessorDeviceGuids(&m_VideoDesc, &count, &guids));

  // Create VPP device 
  if (count > 0)
  {
    CHECK_HR(hr = m_pDXVAVPS->CreateVideoProcessor(guids[0], &m_VideoDesc, D3DFMT_X8R8G8B8, 1, &m_pDXVAVP));
    CHECK_HR(hr = m_pDXVAVPS->GetVideoProcessorCaps(guids[0], &m_VideoDesc, D3DFMT_X8R8G8B8, &m_VPCaps));
  }
  else
  {
    CHECK_HR(hr = m_pDXVAVPS->CreateVideoProcessor(DXVA2_VideoProcProgressiveDevice, &m_VideoDesc, D3DFMT_X8R8G8B8, 1, &m_pDXVAVP));
    CHECK_HR(hr = m_pDXVAVPS->GetVideoProcessorCaps(DXVA2_VideoProcProgressiveDevice, &m_VideoDesc, D3DFMT_X8R8G8B8, &m_VPCaps));
  }

  SAFE_RELEASE(m_pDevice);
  CoTaskMemFree(guids);

  m_pDevice = pDevice;
  m_pDevice->AddRef();

  /*if (pFont != NULL)
  {
    SAFE_RELEASE(pFont);
  }

  hr = D3DXCreateFont( m_pDevice, 25, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
              L"Arial", &pFont );*/

done:
  SAFE_RELEASE(pDevice);
  return hr;
}


//-----------------------------------------------------------------------------
// CreateD3DSample
//
// Creates an sample object (IMFSample) to hold a Direct3D swap chain.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CreateD3DSample(IDirect3DSwapChain9 *pSwapChain, IMFSample **ppVideoSample)
{
  // Caller holds the object lock.

  HRESULT hr = S_OK;
  D3DCOLOR clrBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);

  IDirect3DSurface9* pSurface = NULL;
  IMFSample* pSample = NULL;

  // Get the back buffer surface.
  CHECK_HR(hr = pSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pSurface));

  // Fill it with black.
  CHECK_HR(hr = m_pDevice->ColorFill(pSurface, NULL, clrBlack));

  // Create the sample.
  CHECK_HR(hr = MFCreateVideoSampleFromSurface(pSurface, &pSample));

  // Return the pointer to the caller.
  *ppVideoSample = pSample;
  (*ppVideoSample)->AddRef();

done:
  SAFE_RELEASE(pSurface);
  SAFE_RELEASE(pSample);
  return hr;
}



//-----------------------------------------------------------------------------
// PresentSwapChain
//
// Presents a swap chain that contains a video frame.
//
// pSwapChain: Pointer to the swap chain.
// pSurface: Pointer to the swap chain's back buffer surface.

//
// Note: This method simply calls IDirect3DSwapChain9::Present, but a derived 
// class could do something fancier.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::PresentSwapChain(IDirect3DSwapChain9* pSwapChain, IDirect3DSurface9* pSurface)
{
  HRESULT hr = S_OK;

  if (m_hwnd == NULL)
  {
    return MF_E_INVALIDREQUEST;
  }

  hr = pSwapChain->Present(NULL, &m_rcDestRect, m_hwnd, NULL, 0);

  LOG_MSG_IF_FAILED(L"D3DPresentEngine::PresentSwapChain, IDirect3DSwapChain9::Present failed.", hr);


  return hr;
}

//-----------------------------------------------------------------------------
// PaintFrameWithGDI
// 
// Fills the destination rectangle with black.
//-----------------------------------------------------------------------------

void D3DPresentEngine::PaintFrameWithGDI()
{
  HDC hdc = GetDC(m_hwnd);

  if (hdc)
  {
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));

    if (hBrush)
    {
      FillRect(hdc, &m_rcDestRect, hBrush);
      DeleteObject(hBrush);
    }

    ReleaseDC(m_hwnd, hdc);

    TRACE((L"PaintFrameWithGDI: b: %d t: %d l: %d r: %d\n", m_rcDestRect.bottom, m_rcDestRect.top, m_rcDestRect.left, m_rcDestRect.right));
  }
  else
    TRACE((L"PaintFrameWithGDI failed\n"));
}

void D3DPresentEngine::BlackBackBuffer()
{
  HRESULT hr = S_OK;
  D3DCOLOR    clrBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);
  IDirect3DSurface9* pSurface = NULL;

  if (m_pDevice) 
  {
    hr = m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurface);
    LOG_MSG_IF_FAILED(L"D3DPresentEngine::BlackBackBuffer m_pDevice->GetBackBuffer failed.", hr);

    if (pSurface)
    {
      CHECK_HR(hr = m_pDevice->ColorFill(pSurface, NULL, clrBlack));
    }
  }

done:
  SAFE_RELEASE(pSurface);
}

//HRESULT D3DPresentEngine::CreateSurface(UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface9** ppSurface)
//{
//  HRESULT hr = E_FAIL;
//
//  if (m_pDevice)
//  {
//    IDirect3DSurface9* pSurface = NULL;
//
//    if (SUCCEEDED(hr = m_pDevice->CreateOffscreenPlainSurface(Width, Height, Format, D3DPOOL_SYSTEMMEM, &pSurface, NULL)))
//    {
//      *ppSurface = pSurface;
//    }
//  }
//
//  return hr;
//}

HRESULT D3DPresentEngine::CreateSurface(UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface9** ppSurface)
{
  HRESULT hr = E_FAIL;

  if (m_pDevice)
  {
    IDirect3DSurface9* pSurface = NULL;

    if (SUCCEEDED(hr = m_pDXVAVPS->CreateSurface(Width, Height, 0, Format, m_VPCaps.InputPool, 0, DXVA2_VideoProcessorRenderTarget, &pSurface, NULL)))
    {
      *ppSurface = pSurface;
    }
  }

  return hr;
}

//-----------------------------------------------------------------------------
// GetSwapChainPresentParameters
//
// Given a media type that describes the video format, fills in the
// D3DPRESENT_PARAMETERS for creating a swap chain.
//-----------------------------------------------------------------------------

//HRESULT D3DPresentEngine::GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP)
//{
//  // Caller holds the object lock.
//
//  HRESULT hr = S_OK;
//
//  UINT32 width = 0, height = 0;
//  DWORD d3dFormat = 0;
//
//  // Helper object for reading the proposed type.
//  VideoType videoType(pType);
//
//  if (m_hwnd == NULL)
//  {
//    return MF_E_INVALIDREQUEST;
//  }
//
//  ZeroMemory(pPP, sizeof(D3DPRESENT_PARAMETERS));
//
//  // Get some information about the video format.
//  CHECK_HR(hr = videoType.GetFrameDimensions(&width, &height));
//  CHECK_HR(hr = videoType.GetFourCC(&d3dFormat));
//
//  ZeroMemory(pPP, sizeof(D3DPRESENT_PARAMETERS));
//  pPP->BackBufferWidth = width;
//  pPP->BackBufferHeight = height;
//  pPP->Windowed = TRUE;
//  pPP->SwapEffect = D3DSWAPEFFECT_COPY;
//  pPP->BackBufferFormat = (D3DFORMAT)d3dFormat;
//  pPP->hDeviceWindow = m_hwnd;
//  pPP->Flags = D3DPRESENTFLAG_VIDEO;
//  pPP->PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
//
//  D3DDEVICE_CREATION_PARAMETERS params;
//  CHECK_HR(hr = m_pDevice->GetCreationParameters(&params));
//
//  if (params.DeviceType != D3DDEVTYPE_HAL)
//  {
//    pPP->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
//  }
//
//done:
//  return S_OK;
//}


//-----------------------------------------------------------------------------
// UpdateDestRect
//
// Updates the target rectangle by clipping it to the video window's client
// area.
//
// Called whenever the application sets the video window or the destination
// rectangle.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::UpdateDestRect()
{
  if (m_hwnd == NULL)
  {
    return S_FALSE;
  }


  RECT rcView;
  GetClientRect(m_hwnd, &rcView);

  // Clip the destination rectangle to the window's client area.
  if (m_rcDestRect.right > rcView.right || m_rcDestRect.right == 0)
  {
    m_rcDestRect.right = rcView.right;
  }

  if (m_rcDestRect.bottom > rcView.bottom || m_rcDestRect.bottom == 0)
  {
    m_rcDestRect.bottom = rcView.bottom;
  }

  return S_OK;
}

//-----------------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FindAdapter
//
// Given a handle to a monitor, returns the ordinal number that D3D uses to 
// identify the adapter.
//-----------------------------------------------------------------------------

HRESULT FindAdapter(IDirect3D9 *pD3D9, HMONITOR hMonitor, UINT *puAdapterID)
{
  HRESULT hr = E_FAIL;
  UINT cAdapters = 0;
  UINT uAdapterID = (UINT)-1;

  cAdapters = pD3D9->GetAdapterCount();
  for (UINT i = 0; i < cAdapters; i++)
  {
    HMONITOR hMonitorTmp = pD3D9->GetAdapterMonitor(i);

    if (hMonitorTmp == NULL)
    {
      break;
    }
    if (hMonitorTmp == hMonitor)
    {
      uAdapterID = i;
      break;
    }
  }

  if (uAdapterID != (UINT)-1)
  {
    *puAdapterID = uAdapterID;
    hr = S_OK;
  }
  return hr;
}
