//////////////////////////////////////////////////////////////////////////
//
// EVRPresenterUuid.h : Public header for applications using the DLL.
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

// CLSID of the custom presenter.

// {5325DF1C-6F10-4292-B8FB-BE855F99F88A}
DEFINE_GUID(CLSID_CustomEVRPresenter, 
0x5325df1c, 0x6f10, 0x4292, 0xb8, 0xfb, 0xbe, 0x85, 0x5f, 0x99, 0xf8, 0x8a);

// {D54059EF-CA38-46A5-9123-0249770482EE}
DEFINE_GUID(IID_IEVRCPSettings, 
0xd54059ef, 0xca38, 0x46a5, 0x91, 0x23, 0x2, 0x49, 0x77, 0x4, 0x82, 0xee);
