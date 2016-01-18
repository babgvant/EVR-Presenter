/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <strmif.h>
#include <videoacc.h>
#include <dxva.h>
#include <dxva2api.h>

interface IPinC;

typedef struct IPinCVtbl {
    BEGIN_INTERFACE
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IPinC* This, /* [in] */ REFIID riid, /* [iid_is][out] */ void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IPinC* This);
    ULONG(STDMETHODCALLTYPE* Release)(IPinC* This);
    HRESULT(STDMETHODCALLTYPE* Connect)(IPinC* This, /* [in] */ IPinC* pReceivePin, /* [in] */ const AM_MEDIA_TYPE* pmt);
    HRESULT(STDMETHODCALLTYPE* ReceiveConnection)(IPinC* This, /* [in] */ IPinC* pConnector, /* [in] */ const AM_MEDIA_TYPE* pmt);
    HRESULT(STDMETHODCALLTYPE* Disconnect)(IPinC* This);
    HRESULT(STDMETHODCALLTYPE* ConnectedTo)(IPinC* This, /* [out] */ IPinC** pPin);
    HRESULT(STDMETHODCALLTYPE* ConnectionMediaType)(IPinC* This, /* [out] */ AM_MEDIA_TYPE* pmt);
    HRESULT(STDMETHODCALLTYPE* QueryPinInfo)(IPinC* This, /* [out] */ PIN_INFO* pInfo);
    HRESULT(STDMETHODCALLTYPE* QueryDirection)(IPinC* This, /* [out] */ PIN_DIRECTION* pPinDir);
    HRESULT(STDMETHODCALLTYPE* QueryId)(IPinC* This, /* [out] */ LPWSTR* Id);
    HRESULT(STDMETHODCALLTYPE* QueryAccept)(IPinC* This, /* [in] */ const AM_MEDIA_TYPE* pmt);
    HRESULT(STDMETHODCALLTYPE* EnumMediaTypes)(IPinC* This, /* [out] */ IEnumMediaTypes** ppEnum);
    HRESULT(STDMETHODCALLTYPE* QueryInternalConnections)(IPinC* This, /* [out] */ IPinC** apPin, /* [out][in] */ ULONG* nPin);
    HRESULT(STDMETHODCALLTYPE* EndOfStream)(IPinC* This);
    HRESULT(STDMETHODCALLTYPE* BeginFlush)(IPinC* This);
    HRESULT(STDMETHODCALLTYPE* EndFlush)(IPinC* This);
    HRESULT(STDMETHODCALLTYPE* NewSegment)(IPinC* This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate);
    END_INTERFACE
} IPinCVtbl;

interface IPinC {
    CONST_VTBL struct IPinCVtbl* lpVtbl;
};

interface IMemInputPinC;

typedef struct IMemInputPinCVtbl {
    BEGIN_INTERFACE
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IPinC* This, /* [in] */ REFIID riid, /* [iid_is][out] */ void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IPinC* This);
    ULONG(STDMETHODCALLTYPE* Release)(IPinC* This);
    HRESULT(STDMETHODCALLTYPE* GetAllocator)(IMemInputPinC* This, IMemAllocator** ppAllocator);
    HRESULT(STDMETHODCALLTYPE* NotifyAllocator)(IMemInputPinC* This, IMemAllocator* pAllocator, BOOL bReadOnly);
    HRESULT(STDMETHODCALLTYPE* GetAllocatorRequirements)(IMemInputPinC* This, ALLOCATOR_PROPERTIES* pProps);
    HRESULT(STDMETHODCALLTYPE* Receive)(IMemInputPinC* This, IMediaSample* pSample);
    HRESULT(STDMETHODCALLTYPE* ReceiveMultiple)(IMemInputPinC* This, IMediaSample** pSamples, long nSamples, long* nSamplesProcessed);
    HRESULT(STDMETHODCALLTYPE* ReceiveCanBlock)(IMemInputPinC* This);
    END_INTERFACE
} IMemInputPinCVtbl;

interface IMemInputPinC {
    CONST_VTBL struct IMemInputPinCVtbl* lpVtbl;
};

extern bool HookNewSegmentAndReceive(IPinC* pPinC, IMemInputPinC* pMemInputPin);
extern void UnhookNewSegmentAndReceive();
extern REFERENCE_TIME g_tSegmentStart, g_tSampleStart, g_rtTimePerFrame;

//

interface IAMVideoAcceleratorC;

typedef struct IAMVideoAcceleratorCVtbl {
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE* QueryInterface)(
        IAMVideoAcceleratorC* This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void** ppvObject);

    ULONG(STDMETHODCALLTYPE* AddRef)(
        IAMVideoAcceleratorC* This);

    ULONG(STDMETHODCALLTYPE* Release)(
        IAMVideoAcceleratorC* This);

    HRESULT(STDMETHODCALLTYPE* GetVideoAcceleratorGUIDs)(
        IAMVideoAcceleratorC* This,
        /* [out][in] */ LPDWORD pdwNumGuidsSupported,
        /* [out][in] */ LPGUID pGuidsSupported);

    HRESULT(STDMETHODCALLTYPE* GetUncompFormatsSupported)(
        IAMVideoAcceleratorC* This,
        /* [in] */ const GUID* pGuid,
        /* [out][in] */ LPDWORD pdwNumFormatsSupported,
        /* [out][in] */ LPDDPIXELFORMAT pFormatsSupported);

    HRESULT(STDMETHODCALLTYPE* GetInternalMemInfo)(
        IAMVideoAcceleratorC* This,
        /* [in] */ const GUID* pGuid,
        /* [in] */ const AMVAUncompDataInfo* pamvaUncompDataInfo,
        /* [out][in] */ LPAMVAInternalMemInfo pamvaInternalMemInfo);

    HRESULT(STDMETHODCALLTYPE* GetCompBufferInfo)(
        IAMVideoAcceleratorC* This,
        /* [in] */ const GUID* pGuid,
        /* [in] */ const AMVAUncompDataInfo* pamvaUncompDataInfo,
        /* [out][in] */ LPDWORD pdwNumTypesCompBuffers,
        /* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo);

    HRESULT(STDMETHODCALLTYPE* GetInternalCompBufferInfo)(
        IAMVideoAcceleratorC* This,
        /* [out][in] */ LPDWORD pdwNumTypesCompBuffers,
        /* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo);

    HRESULT(STDMETHODCALLTYPE* BeginFrame)(
        IAMVideoAcceleratorC* This,
        /* [in] */ const AMVABeginFrameInfo* amvaBeginFrameInfo);

    HRESULT(STDMETHODCALLTYPE* EndFrame)(
        IAMVideoAcceleratorC* This,
        /* [in] */ const AMVAEndFrameInfo* pEndFrameInfo);

    HRESULT(STDMETHODCALLTYPE* GetBuffer)(
        IAMVideoAcceleratorC* This,
        /* [in] */ DWORD dwTypeIndex,
        /* [in] */ DWORD dwBufferIndex,
        /* [in] */ BOOL bReadOnly,
        /* [out] */ LPVOID* ppBuffer,
        /* [out] */ LONG* lpStride);

    HRESULT(STDMETHODCALLTYPE* ReleaseBuffer)(
        IAMVideoAcceleratorC* This,
        /* [in] */ DWORD dwTypeIndex,
        /* [in] */ DWORD dwBufferIndex);

    HRESULT(STDMETHODCALLTYPE* Execute)(
        IAMVideoAcceleratorC* This,
        /* [in] */ DWORD dwFunction,
        /* [in] */ LPVOID lpPrivateInputData,
        /* [in] */ DWORD cbPrivateInputData,
        /* [in] */ LPVOID lpPrivateOutputDat,
        /* [in] */ DWORD cbPrivateOutputData,
        /* [in] */ DWORD dwNumBuffers,
        /* [in] */ const AMVABUFFERINFO* pamvaBufferInfo);

    HRESULT(STDMETHODCALLTYPE* QueryRenderStatus)(
        IAMVideoAcceleratorC* This,
        /* [in] */ DWORD dwTypeIndex,
        /* [in] */ DWORD dwBufferIndex,
        /* [in] */ DWORD dwFlags);

    HRESULT(STDMETHODCALLTYPE* DisplayFrame)(
        IAMVideoAcceleratorC* This,
        /* [in] */ DWORD dwFlipToIndex,
        /* [in] */ IMediaSample* pMediaSample);

    END_INTERFACE
} IAMVideoAcceleratorCVtbl;

interface IAMVideoAcceleratorC {
    CONST_VTBL struct IAMVideoAcceleratorCVtbl* lpVtbl;
};

void HookAMVideoAccelerator(IAMVideoAcceleratorC* pAMVideoAcceleratorC);

// DXVA2 hooks
void HookDirectXVideoDecoderService(void* pIDirectXVideoDecoderService);
LPCTSTR GetDXVADecoderDescription();
LPCTSTR GetDXVAVersion();
void ClearDXVAState();
