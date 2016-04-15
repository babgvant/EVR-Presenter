//////////////////////////////////////////////////////////////////////////
//
// dllmain.cpp : Implements DLL exports and COM class factory
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Note: This source file implements the class factory for the DMO,
//       plus the following DLL functions:
//       - DllMain
//       - DllCanUnloadNow
//       - DllRegisterServer
//       - DllUnregisterServer
//       - DllGetClassObject
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

#include <initguid.h>
#include "EVRPresenterUuid.h"

HMODULE g_hModule;                  // DLL module handle

DEFINE_CLASSFACTORY_SERVER_LOCK;    // Defines the static member variable for the class factory lock.

// Friendly name for COM registration.
WCHAR* g_sFriendlyName =  L"EVR Presenter (babgvant)";

//namespace
//{
//#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
//  const std::string path_to_log_file = "./";
//#else
//  const std::string path_to_log_file = "/tmp/";
//#endif
//}

// g_ClassFactories: Array of class factory data.
// Defines a look-up table of CLSIDs and corresponding creation functions.

ClassFactoryData g_ClassFactories[] =
{
    {   &CLSID_CustomEVRPresenter, EVRCustomPresenter::CreateInstance }
};
      
const DWORD g_numClassFactories = ARRAY_SIZE(g_ClassFactories);

// DllMain: Entry-point for the DLL.
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = (HMODULE)hModule;

        {
          HKEY newKey;
          WCHAR logPath[_MAX_PATH + 1] = { 0 };
          DWORD readType;
          DWORD hsize = sizeof(logPath);
          DWORD regVal = 0;
          bool bGotLogPath = false;

          if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\babgvant\\EVR Presenter\0",
            0, KEY_READ | KEY_QUERY_VALUE, &newKey) == ERROR_SUCCESS)
          {
            if (RegQueryValueExW(newKey, L"LogFile\0", 0, &readType, (LPBYTE)logPath, &hsize) == ERROR_SUCCESS)
            {
              bGotLogPath = true;
            }

            RegCloseKey(newKey);
          }

          if(bGotLogPath)
            TRACE_INIT(logPath);
          else
            TRACE_INIT(NULL);
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        TRACE_CLOSE();
        break;
    }
    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    if (!ClassFactory::IsLocked())
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}


STDAPI DllRegisterServer()
{
    HRESULT hr;
    
    // Register the MFT's CLSID as a COM object.
    hr = RegisterObject(g_hModule, CLSID_CustomEVRPresenter, g_sFriendlyName, TEXT("Both"));

    return hr;
}

STDAPI DllUnregisterServer()
{
    // Unregister the CLSID
    UnregisterObject(CLSID_CustomEVRPresenter);

    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv)
{
    ClassFactory *pFactory = NULL;

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE; // Default to failure

    // Find an entry in our look-up table for the specified CLSID.
    for (DWORD index = 0; index < g_numClassFactories; index++)
    {
        if (*g_ClassFactories[index].pclsid == clsid)
        {
            // Found an entry. Create a new class factory object.
            pFactory = new ClassFactory(g_ClassFactories[index].pfnCreate);
            if (pFactory)
            {
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pFactory->QueryInterface(riid, ppv);
    }
    SAFE_RELEASE(pFactory);

    return hr;
}


