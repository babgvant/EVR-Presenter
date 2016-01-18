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

	HRESULT CreateSurface(UINT Width,UINT Height,D3DFORMAT Format,IDirect3DSurface9** ppSurface);

protected:
    HRESULT InitializeD3D();
    HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP);
    HRESULT CreateD3DDevice();
    HRESULT CreateD3DSample(IDirect3DSwapChain9 *pSwapChain, IMFSample **ppVideoSample);
    HRESULT UpdateDestRect();

    // A derived class can override these handlers to allocate any additional D3D resources.
    virtual HRESULT OnCreateVideoSamples(D3DPRESENT_PARAMETERS& pp) { return S_OK; }
    virtual void    OnReleaseResources() { }

    virtual HRESULT PresentSwapChain(IDirect3DSwapChain9* pSwapChain, IDirect3DSurface9* pSurface);
    virtual void    PaintFrameWithGDI();

protected:
    UINT                        m_DeviceResetToken;     // Reset token for the D3D device manager.
	int							m_bufferCount;
	int							m_SampleWidth;
	int							m_SampleHeight;

    HWND                        m_hwnd;                 // Application-provided destination window.
    RECT                        m_rcDestRect;           // Destination rectangle.
    D3DDISPLAYMODE              m_DisplayMode;          // Adapter's display mode.

    CritSec                     m_ObjectLock;           // Thread lock for the D3D device.

    // COM interfaces
    IDirect3D9Ex                *m_pD3D9;
    IDirect3DDevice9Ex          *m_pDevice;
    IDirect3DDeviceManager9     *m_pDeviceManager;        // Direct3D device manager.
    IDirect3DSurface9           *m_pSurfaceRepaint;       // Surface for repaint requests.

	int m_DroppedFrames;
	int m_GoodFrames;
	int m_FramesInQueue;
	double m_AvgTimeDelta;

	// various structures for DXVA2 calls
    //DXVA2_VideoDesc                 m_VideoDesc;
    //DXVA2_VideoProcessBltParams     m_BltParams; 
    //DXVA2_VideoSample               m_Sample;

	//IDirectXVideoProcessorService   *m_pDXVAVPS;            // Service required to create video processors
    //IDirectXVideoProcessor          *m_pDXVAVP;
	//IDirect3DSurface9               *m_pRenderSurface;      // The surface which is passed to render
    //IDirect3DSurface9               *m_pMixerSurfaces[PRESENTER_BUFFER_COUNT]; // The surfaces, which are used by mixer
	
private: // disallow copy and assign
    D3DPresentEngine(const D3DPresentEngine&);              
    void operator=(const D3DPresentEngine&);

	//ID3DXFont* pFont;
};