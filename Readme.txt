/*	
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

Overview
--------

This sample implements a custom presenter for the Enhanced Video 
Renderer (EVR) which supports SubRenderIntf.

The sample presenter can be used with the DirectShow EVR filter or 
the Media Foundation EVR sink.

This sample requires Windows Vista or later.


To use this sample (DirectShow):
--------------------------------

1. Build the sample.
2. Regsvr32 EvrPresenter.dll.
3. Build and run the EVRPlayer sample.
4. From the File menu, select EVR Presenter.
5. Select a file for playback.


To use this sample (Media Foundation):
--------------------------------------

1. Build the sample.
2. Regsvr32 EvrPresenter.dll.
3. Build and run the MFPlayer sample.
4. From the File menu, select Open File
5. In the Open File dialog, check "Custom EVR Presenter."
6. Select a file for playback.



Classes
--------

The main classes in this sample are the following:

D3DPresentEngine: 
    Creates the Direct3D device, allocates Direct3D surfaces for 
    rendering, and presents the surfaces.

EVRCustomPresenter: 
    Implements the custom presenter.

Scheduler:
    Schedules when a sample should be displayed.
