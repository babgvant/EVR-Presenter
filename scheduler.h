//////////////////////////////////////////////////////////////////////////
//
// Scheduler.h: Schedules when video frames are presented.
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


struct SchedulerCallback;

//-----------------------------------------------------------------------------
// Scheduler class
//
// Schedules when a sample should be displayed.
//
// Note: Presentation of each sample is performed by another object which
// must implement SchedulerCallback::PresentSample.
//
// General design:
// The scheduler generally receives samples before their presentation time. It
// puts the samples on a queue and presents them in FIFO order on a worker 
// thread. The scheduler communicates with the worker thread by posting thread
// messages.
//
// The caller has the option of presenting samples immediately (for example,
// for repaints). 
//-----------------------------------------------------------------------------

class Scheduler
{
public:
  Scheduler();
  virtual ~Scheduler();

  void SetCallback(SchedulerCallback *pCB)
  {
    m_pCB = pCB;
  }

  void SetFrameRate(const MFRatio& fps);

  float GetClockRate() {
    AutoLock lock(m_schedCritSec);
    return m_fRate;
  }
  void SetClockRate(float fRate) {
    AutoLock lock(m_schedCritSec);
    m_fRate = fRate;
  }

  int GetFrameDropThreshold() {
    AutoLock lock(m_schedCritSec);
    return m_iFrameDropThreshold;
  }
  void SetFrameDropThreshold(int iThreshold) {
    AutoLock lock(m_schedCritSec);
    m_iFrameDropThreshold = iThreshold;
  }

  const LONGLONG& LastSampleTime() const { return m_LastSampleTime; }
  const LONGLONG& FrameDuration() const { return m_PerFrameInterval; }

  bool GetUseMfTimeCalc()
  {
    AutoLock lock(m_schedCritSec);
    return m_bUseMfTimeCalc;
  }

  void SetUseMfTimeCalc(bool useMfTimeCalc)
  {
    AutoLock lock(m_schedCritSec);
    m_bUseMfTimeCalc = useMfTimeCalc;
  }

  HRESULT StartScheduler(IMFClock *pClock);
  HRESULT StopScheduler();

  HRESULT ScheduleSample(IMFSample *pSample, BOOL bPresentNow);
  HRESULT ProcessSamplesInQueue(LONG *plNextSleep);
  HRESULT ProcessSample(IMFSample *pSample, LONG *plNextSleep);
  HRESULT Flush();

  // ThreadProc for the scheduler thread.
  static DWORD WINAPI SchedulerThreadProc(LPVOID lpParameter);

private:
  // non-static version of SchedulerThreadProc.
  DWORD SchedulerThreadProcPrivate();


private:
  ThreadSafeQueue<IMFSample>  m_ScheduledSamples;     // Samples waiting to be presented.

  IMFClock            *m_pClock;  // Presentation clock. Can be NULL.
  SchedulerCallback   *m_pCB;     // Weak reference; do not delete.

  DWORD               m_dwThreadID;
  HANDLE              m_hSchedulerThread;
  HANDLE              m_hThreadReadyEvent;
  HANDLE              m_hFlushEvent;

  float               m_fRate;                // Playback rate.
  MFTIME              m_PerFrameInterval;     // Duration of each frame.
  LONGLONG            m_PerFrame_1_4th;       // 1/4th of the frame duration.
  MFTIME              m_LastSampleTime;       // Most recent sample time.
  bool				m_bUseMfTimeCalc;
  int					m_iFrameDropThreshold;
  CritSec				m_schedCritSec;
};


//-----------------------------------------------------------------------------
// SchedulerCallback
//
// Defines the callback method to present samples. 
//-----------------------------------------------------------------------------

struct SchedulerCallback
{
  virtual HRESULT PresentSample(IMFSample *pSample, LONGLONG llTarget, LONGLONG timeDelta, LONGLONG remainingInQueue, LONGLONG frameDurationDiv4) = 0;
};
