//////////////////////////////////////////////////////////////////////////
//
// Presenter.h : Defines the presenter object.
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

#include "SubRenderOptionsImpl.h"
#include "IEVRCPSettings.h"

#pragma once

inline int GCD(int a, int b)
{
  if (a == 0 || b == 0) {
    return 1;
  }
  while (a != b) {
    if (a < b) {
      b -= a;
    }
    else if (a > b) {
      a -= b;
    }
  }
  return a;
}

// RENDER_STATE: Defines the state of the presenter. 
enum RENDER_STATE
{
  RENDER_STATE_STARTED = 1,
  RENDER_STATE_STOPPED,
  RENDER_STATE_PAUSED,
  RENDER_STATE_SHUTDOWN,  // Initial state. 

  // State transitions:

  // InitServicePointers                  -> STOPPED
  // ReleaseServicePointers               -> SHUTDOWN
  // IMFClockStateSink::OnClockStart      -> STARTED
  // IMFClockStateSink::OnClockRestart    -> STARTED
  // IMFClockStateSink::OnClockPause      -> PAUSED
  // IMFClockStateSink::OnClockStop       -> STOPPED
};

// FRAMESTEP_STATE: Defines the presenter's state with respect to frame-stepping.
enum FRAMESTEP_STATE
{
  FRAMESTEP_NONE,             // Not frame stepping.
  FRAMESTEP_WAITING_START,    // Frame stepping, but the clock is not started.
  FRAMESTEP_PENDING,          // Clock is started. Waiting for samples.
  FRAMESTEP_SCHEDULED,        // Submitted a sample for rendering.
  FRAMESTEP_COMPLETE          // Sample was rendered. 

  // State transitions:

  // MFVP_MESSAGE_STEP                -> WAITING_START
  // OnClockStart/OnClockRestart      -> PENDING
  // MFVP_MESSAGE_PROCESSINPUTNOTIFY  -> SUBMITTED
  // OnSampleFree                     -> COMPLETE
  // MFVP_MESSAGE_CANCEL              -> NONE
  // OnClockStop                      -> NONE
  // OnClockSetRate( non-zero )       -> NONE
};

#define BLEND_FUNC_PARAMS (BYTE* video[4], int videoStride[4], RECT vidRect, BYTE* subData[4], int subStride[4], POINT position, SIZE size, LAVPixelFormat pixFmt, int bpp)

#define DECLARE_BLEND_FUNC(name) \
  HRESULT name BLEND_FUNC_PARAMS

#define DECLARE_BLEND_FUNC_IMPL(name) \
  DECLARE_BLEND_FUNC(EVRCustomPresenter::name)

typedef struct EVRSubtitleConsumerContext {
  LPWSTR name;                    ///< name of the Consumer
  LPWSTR version;                 ///< Version of the Consumer
  SIZE   originalVideoSize;       ///< Size of the original video
  SIZE   arAdjustedVideoSize;       ///< Size of the video
  RECT   videoOutputRect;
  RECT	 subtitleTargetRect;
  int	supportedLevels;
  ULONGLONG frameRate;
  double refreshRate;
  LPWSTR yuvMatrix;
} EVRSubtitleConsumerContext;

//-----------------------------------------------------------------------------
//  EVRCustomPresenter class
//  Description: Implements the custom presenter.
//-----------------------------------------------------------------------------

class EVRCustomPresenter :
  BaseObject,
  RefCountedObject,
  // COM interfaces:
  public IMFVideoDeviceID,
  public IMFVideoPresenter, // Inherits IMFClockStateSink
  public IMFRateSupport,
  public IMFGetService,
  public IMFTopologyServiceLookupClient,
  public IMFVideoDisplayControl,
  public CSubRenderOptionsImpl,
  public IEVRCPSettings,
  public ISubRenderConsumer2,
  public IEVRTrustedVideoPlugin,
  public IEVRCPConfig
{

public:
  static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppv);

  // IUnknown methods
  STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  // IMFGetService methods
  STDMETHOD(GetService)(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

  // IMFVideoPresenter methods
  STDMETHOD(ProcessMessage)(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
  STDMETHOD(GetCurrentMediaType)(IMFVideoMediaType** ppMediaType);

  // IMFClockStateSink methods
  STDMETHOD(OnClockStart)(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);
  STDMETHOD(OnClockStop)(MFTIME hnsSystemTime);
  STDMETHOD(OnClockPause)(MFTIME hnsSystemTime);
  STDMETHOD(OnClockRestart)(MFTIME hnsSystemTime);
  STDMETHOD(OnClockSetRate)(MFTIME hnsSystemTime, float flRate);

  // IMFRateSupport methods
  STDMETHOD(GetSlowestRate)(MFRATE_DIRECTION eDirection, BOOL bThin, float *pflRate);
  STDMETHOD(GetFastestRate)(MFRATE_DIRECTION eDirection, BOOL bThin, float *pflRate);
  STDMETHOD(IsRateSupported)(BOOL bThin, float flRate, float *pflNearestSupportedRate);

  // IMFVideoDeviceID methods
  STDMETHOD(GetDeviceID)(IID* pDeviceID);

  // IMFTopologyServiceLookupClient methods
  STDMETHOD(InitServicePointers)(IMFTopologyServiceLookup *pLookup);
  STDMETHOD(ReleaseServicePointers)();

  // IMFVideoDisplayControl methods
  STDMETHOD(GetNativeVideoSize)(SIZE* pszVideo, SIZE* pszARVideo) {
    SIZE r;
    r.cy = m_VideoSize.cy;

    if (m_bCorrectAR && m_VideoAR.cx > 0 && m_VideoAR.cy > 0)
    {
      r.cx = (LONGLONG(m_VideoSize.cy) * LONGLONG(m_VideoAR.cx)) / LONGLONG(m_VideoAR.cy);
    }
    else
    {
      r.cx = m_VideoSize.cx;
    }
    *pszVideo = r;
    SIZE ar;
    /*ar.cx = m_VideoSize.cx*m_VideoAR.cx;
    ar.cy = m_VideoSize.cy*m_VideoAR.cy;*/
    ar.cx = m_VideoAR.cx;
    ar.cy = m_VideoAR.cy;
    *pszARVideo = ar;
    return S_OK;
  }
  STDMETHOD(GetIdealVideoSize)(SIZE* pszMin, SIZE* pszMax) {
    if (pszMin) {
      pszMin->cx = 1;
      pszMin->cy = 1;
    }

    if (pszMax) {
      if (m_pD3DPresentEngine) {
        pszMax->cx = m_pD3DPresentEngine->Width();
        pszMax->cy = m_pD3DPresentEngine->Height();
      }
    }

    return S_OK;
  }
  STDMETHOD(SetVideoPosition)(const MFVideoNormalizedRect* pnrcSource, const LPRECT prcDest);
  STDMETHOD(GetVideoPosition)(MFVideoNormalizedRect* pnrcSource, LPRECT prcDest);
  STDMETHOD(SetAspectRatioMode)(DWORD dwAspectRatioMode);
  STDMETHOD(GetAspectRatioMode)(DWORD* pdwAspectRatioMode);
  STDMETHOD(SetVideoWindow)(HWND hwndVideo);
  STDMETHOD(GetVideoWindow)(HWND* phwndVideo);
  STDMETHOD(RepaintVideo)();
  STDMETHOD(GetCurrentImage)(BITMAPINFOHEADER* pBih, BYTE** pDib, DWORD* pcbDib, LONGLONG* pTimeStamp) { return E_NOTIMPL; }
  STDMETHOD(SetBorderColor)(COLORREF Clr) {
    m_BorderColor = Clr;
    return S_OK;
  }
  STDMETHOD(GetBorderColor)(COLORREF* pClr) {
    CheckPointer(pClr, E_POINTER);
    *pClr = m_BorderColor;
    return S_OK;
  }
  STDMETHOD(SetRenderingPrefs)(DWORD dwRenderFlags) {
    m_dwVideoRenderPrefs = (MFVideoRenderPrefs)dwRenderFlags;
    return S_OK;
  }
  STDMETHOD(GetRenderingPrefs)(DWORD* pdwRenderFlags) {
    CheckPointer(pdwRenderFlags, E_POINTER);
    *pdwRenderFlags = m_dwVideoRenderPrefs;
    return S_OK;
  }
  STDMETHOD(SetFullscreen)(BOOL bFullscreen) {
    m_bIsFullscreen = !!bFullscreen;
    return S_OK;
  }
  STDMETHOD(GetFullscreen)(BOOL* pbFullscreen) {
    CheckPointer(pbFullscreen, E_POINTER);
    *pbFullscreen = m_bIsFullscreen;
    return S_OK;
  }

  // IEVRTrustedVideoPlugin
  STDMETHODIMP IsInTrustedVideoMode(BOOL* pYes)
  {
    CheckPointer(pYes, E_POINTER);
    *pYes = TRUE;
    return S_OK;
  }

  STDMETHODIMP CanConstrict(BOOL* pYes)
  {
    CheckPointer(pYes, E_POINTER);
    *pYes = TRUE;
    return S_OK;
  }

  STDMETHODIMP SetConstriction(DWORD dwKPix)
  {
    return S_OK;
  }

  STDMETHODIMP DisableImageExport(BOOL bDisable)
  {
    return S_OK;
  }

  //IDirectXVideoMemoryConfiguration
  /*STDMETHOD(GetAvailableSurfaceTypeByIndex(DWORD dwTypeIndex, DXVA2_SurfaceType* pdwType));
  STDMETHOD(SetSurfaceType(DXVA2_SurfaceType dwType));*/

  // ISubRenderConsumer
  STDMETHODIMP GetMerit(ULONG *merit) { CheckPointer(merit, E_POINTER); *merit = 0x00040000; return S_OK; }
  STDMETHODIMP Connect(ISubRenderProvider *subtitleRenderer);
  STDMETHODIMP Disconnect(void);
  STDMETHODIMP DeliverFrame(REFERENCE_TIME start, REFERENCE_TIME stop, LPVOID context, ISubRenderFrame *subtitleFrame);

  // ISubRenderConsumer2
  STDMETHODIMP Clear(REFERENCE_TIME clearNewerThan);

  // ISubRenderOptions
  STDMETHODIMP GetBool(LPCSTR field, bool      *value);
  STDMETHODIMP GetInt(LPCSTR field, int       *value);
  STDMETHODIMP GetSize(LPCSTR field, SIZE      *value);
  STDMETHODIMP GetRect(LPCSTR field, RECT      *value);
  STDMETHODIMP GetUlonglong(LPCSTR field, ULONGLONG *value);
  STDMETHODIMP GetDouble(LPCSTR field, double    *value);
  STDMETHODIMP GetString(LPCSTR field, LPWSTR    *value, int *chars);
  STDMETHODIMP GetBin(LPCSTR field, LPVOID    *value, int *size);

  STDMETHODIMP SetBool(LPCSTR field, bool      value);
  STDMETHODIMP SetInt(LPCSTR field, int       value);
  STDMETHODIMP SetSize(LPCSTR field, SIZE      value);
  STDMETHODIMP SetRect(LPCSTR field, RECT      value);
  STDMETHODIMP SetUlonglong(LPCSTR field, ULONGLONG value);
  STDMETHODIMP SetDouble(LPCSTR field, double    value);
  STDMETHODIMP SetString(LPCSTR field, LPWSTR    value, int chars);
  STDMETHODIMP SetBin(LPCSTR field, LPVOID    value, int size);

  //IEVRCPSettings
  STDMETHODIMP SetNominalRange(MFNominalRange range);
  STDMETHODIMP_(MFNominalRange) GetNominalRange()
  {
    AutoLock lock(m_subCritSec);
    return m_outputRange;
  }
  STDMETHODIMP SetSubtitleAlpha(float fAlpha)
  {
    AutoLock lock(m_subCritSec);
    m_fBitmapAlpha = fAlpha;
    return S_OK;
  }
  STDMETHODIMP_(float) GetSubtitleAlpha()
  {
    AutoLock lock(m_subCritSec);
    return m_fBitmapAlpha;
  }

  //IEVRCPConfig
  STDMETHODIMP SetInt(EVRCPSetting setting, int value) {
    HRESULT hr = S_OK;
    AutoLock lock(m_subCritSec);

    switch (setting)
    {
    case EVRCP_SETTING_NOMINAL_RANGE:
      m_outputRange = (MFNominalRange)value;
      break;
    case EVRCP_SETTING_FRAME_DROP_THRESHOLD:
      m_scheduler.SetFrameDropThreshold(value);
      break;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP GetInt(EVRCPSetting setting, int* value) {
    HRESULT hr = S_OK;
    CheckPointer(value, E_POINTER);
    AutoLock lock(m_subCritSec);

    switch (setting)
    {
    case EVRCP_SETTING_NOMINAL_RANGE:
      *value = m_outputRange;
      break;
    case EVRCP_SETTING_FRAME_DROP_THRESHOLD:
      *value = m_scheduler.GetFrameDropThreshold();
      break;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP SetFloat(EVRCPSetting setting, float value) {
    HRESULT hr = S_OK;
    AutoLock lock(m_subCritSec);

    switch (setting)
    {
    case EVRCP_SETTING_SUBTITLE_ALPHA:
      m_fBitmapAlpha = value;
      break;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP GetFloat(EVRCPSetting setting, float* value) {
    HRESULT hr = S_OK;
    CheckPointer(value, E_POINTER);
    AutoLock lock(m_subCritSec);

    switch (setting)
    {
    case EVRCP_SETTING_SUBTITLE_ALPHA:
      *value = m_fBitmapAlpha;
      break;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP SetBool(EVRCPSetting setting, bool value) {
    HRESULT hr = S_OK;
    AutoLock lock(m_subCritSec);

    switch (setting)
    {
    case EVRCP_SETTING_USE_MF_TIME_CALC:
      m_scheduler.SetUseMfTimeCalc(value);
      break;
    case EVRCP_SETTING_CORRECT_AR:
      m_bCorrectAR = value;
      break;
    case EVRCP_SETTING_REQUEST_OVERLAY:
      hr = m_pD3DPresentEngine->SetBool(setting, value);
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
    AutoLock lock(m_subCritSec);

    switch (setting)
    {
    case EVRCP_SETTING_USE_MF_TIME_CALC:
      *value = m_scheduler.GetUseMfTimeCalc();
      break;
    case EVRCP_SETTING_CORRECT_AR:
      *value = m_bCorrectAR;
      break;
    case EVRCP_SETTING_REQUEST_OVERLAY:
      m_pD3DPresentEngine->GetBool(setting, value);
      break;
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP SetString(EVRCPSetting setting, LPWSTR value) {
    HRESULT hr = S_OK;
    AutoLock lock(m_subCritSec);

    switch (setting)
    {
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }
  STDMETHODIMP GetString(EVRCPSetting setting, LPWSTR value) {
    HRESULT hr = S_OK;
    CheckPointer(value, E_POINTER);
    AutoLock lock(m_subCritSec);

    switch (setting)
    {
    default:
      hr = E_NOTIMPL;
      break;
    }
    return hr;
  }

protected:
  EVRCustomPresenter(HRESULT& hr);
  virtual ~EVRCustomPresenter();

  STDMETHODIMP HookEVR(IBaseFilter *evr);
  STDMETHODIMP ProcessSubtitles(DWORD waitfor);

  // CheckShutdown: 
  //     Returns MF_E_SHUTDOWN if the presenter is shutdown.
  //     Call this at the start of any methods that should fail after shutdown.
  inline HRESULT CheckShutdown() const
  {
    if (m_RenderState == RENDER_STATE_SHUTDOWN)
    {
      return MF_E_SHUTDOWN;
    }
    else
    {
      return S_OK;
    }
  }

  // IsActive: The "active" state is started or paused.
  inline BOOL IsActive() const
  {
    return ((m_RenderState == RENDER_STATE_STARTED) || (m_RenderState == RENDER_STATE_PAUSED));
  }

  // IsScrubbing: Scrubbing occurs when the frame rate is 0.
  inline BOOL IsScrubbing() const { return m_fRate == 0.0f; }

  // NotifyEvent: Send an event to the EVR through its IMediaEventSink interface.
  void NotifyEvent(long EventCode, LONG_PTR Param1, LONG_PTR Param2)
  {
    if (m_pMediaEventSink)
    {
      m_pMediaEventSink->Notify(EventCode, Param1, Param2);
    }
  }

  float GetMaxRate(BOOL bThin);

  // Mixer operations
  HRESULT ConfigureMixer(IMFTransform *pMixer);

  // Formats
  HRESULT CreateOptimalVideoType(IMFMediaType* pProposed, IMFMediaType **ppOptimal);
  HRESULT CalculateOutputRectangle(IMFMediaType *pProposed, RECT *prcOutput);
  HRESULT SetMediaType(IMFMediaType *pMediaType);
  HRESULT IsMediaTypeSupported(IMFMediaType *pMediaType);

  // Message handlers
  HRESULT Flush();
  HRESULT RenegotiateMediaType();
  HRESULT ProcessInputNotify();
  HRESULT BeginStreaming();
  HRESULT EndStreaming();
  HRESULT CheckEndOfStream();

  // Managing samples
  void    ProcessOutputLoop();
  HRESULT ProcessOutput();
  HRESULT DeliverSample(IMFSample *pSample, BOOL bRepaint);
  HRESULT TrackSample(IMFSample *pSample);
  void    ReleaseResources();

  // Frame-stepping
  HRESULT PrepareFrameStep(DWORD cSteps);
  HRESULT StartFrameStep();
  HRESULT DeliverFrameStepSample(IMFSample *pSample);
  HRESULT CompleteFrameStep(IMFSample *pSample);
  HRESULT CancelFrameStep();

  // Callbacks

  // Callback when a video sample is released.
  HRESULT OnSampleFree(IMFAsyncResult *pResult);
  AsyncCallback<EVRCustomPresenter>   m_SampleFreeCB;

protected:

  // FrameStep: Holds information related to frame-stepping. 
  // Note: The purpose of this structure is simply to organize the data in one variable.
  struct FrameStep
  {
    FrameStep() : state(FRAMESTEP_NONE), steps(0), pSampleNoRef(NULL)
    {
    }

    FRAMESTEP_STATE     state;          // Current frame-step state.
    VideoSampleList     samples;        // List of pending samples for frame-stepping.
    DWORD               steps;          // Number of steps left.
    DWORD_PTR           pSampleNoRef;   // Identifies the frame-step sample.
  };

protected:

  RENDER_STATE                m_RenderState;          // Rendering state.
  FrameStep                   m_FrameStep;            // Frame-stepping information.

  CritSec                     m_ObjectLock;           // Serializes our public methods.  
//int							m_SampleWidth;
//int							m_SampleHeight;
//int							m_SampleArX;
//int							m_SampleArY;
  MFVideoAspectRatioMode		  m_dwAspectRatioMode;
  MFVideoRenderPrefs          m_dwVideoRenderPrefs;
  COLORREF                    m_BorderColor;
  bool						            m_bIsFullscreen;

  // Samples and scheduling
  Scheduler                   m_scheduler;            // Manages scheduling of samples.
  SamplePool                  m_SamplePool;           // Pool of allocated samples.
  DWORD                       m_TokenCounter;         // Counter. Incremented whenever we create new samples.

  // Rendering state
  BOOL                        m_bSampleNotify;        // Did the mixer signal it has an input sample?
  BOOL                        m_bRepaint;             // Do we need to repaint the last sample?
  BOOL                        m_bPrerolled;           // Have we presented at least one sample?
  BOOL                        m_bEndStreaming;        // Did we reach the end of the stream (EOS)?

  MFVideoNormalizedRect       m_nrcSource;            // Source rectangle.
  float                       m_fRate;                // Playback rate.

  // Deletable objects.
  D3DPresentEngine            *m_pD3DPresentEngine;    // Rendering engine. (Never null if the constructor succeeds.)

  // COM interfaces.
  IMFClock                    *m_pClock;               // The EVR's clock.
  IMFTransform                *m_pMixer;               // The mixer.
  IMediaEventSink             *m_pMediaEventSink;      // The EVR's event-sink interface.
  IMFMediaType                *m_pMediaType;           // Output media type
  IBaseFilter					        *m_pEvr;

  ISubRenderProvider          *m_pProvider;
  ISubRenderFrame             *m_SubtitleFrame;
  EVRSubtitleConsumerContext  context;
  LONGLONG                    m_llFrame;
  bool                        m_bEvrPinHooked;
  //IPin* m_pPin;
 //   IMemInputPin* m_pMemInputPin;
  REFERENCE_TIME              m_rtStart;
  REFERENCE_TIME              m_rtStop;

  CritSec				              m_subCritSec;
  HANDLE				              m_hEvtDelivered;
  //IMFVideoMixerBitmap * m_pMixerBitmap;
  ULONGLONG			              m_lastSubtitleId;
  bool				                m_bSubtitleSet;
  MFNominalRange		          m_outputRange;
  SIZE				                m_VideoSize;
  SIZE				                m_VideoAR;
  float				                m_fBitmapAlpha;
  //IPin *				m_pEvrPin;
  REFERENCE_TIME		          m_rtTimePerFrame;
  bool				                m_bCorrectAR;
};


