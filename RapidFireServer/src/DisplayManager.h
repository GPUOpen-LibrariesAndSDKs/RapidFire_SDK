//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <vector>

class  ADLWrapper;
struct DisplayData;

class DisplayManager
{
public:

    DisplayManager();
    ~DisplayManager();

    // Enumerates all active displays. This function has to be called before any other method of this class is called.
    // The displays are stored in the order in which they are enumerated. An index will be assigned to each display.
    // The indexing starts at 0.
    // return: The number of mapped displays.
    unsigned int    enumDisplays();

    // returns the number of GPUs in the system.
    unsigned int    getNumGPUs() const;

    // Returns the number of mapped displays.
    unsigned int    getNumDisplays() const;

    // returns the number of displays mapped on GPU uiGPU.
    unsigned int    getNumDisplaysOnGPU(unsigned int uiGPU) const;

    // returns the DisplayID of the n-th Display on GPU uiGPU.
    unsigned int    getDisplayOnGPU(unsigned int uiGPU, unsigned int n = 0) const;

    // Writes the index of the display that has the windows id uiWindowsDisplayId into uiDspId.
    // The windows ID is obtained by the display name which is e.g. \\.\Display1 \\.\Display2.
    // Those would correspond to windows ID 1 and 2.
    // The uiWindowsDisplayId actually maps only to a Desktop ID since in case of Eyefinity a desktop consits of
    // multiple displays but windows will see only e.g. \\.\Display1
    bool            getDisplayIdFromWinID(unsigned int uiWindowsDisplayId, unsigned int& uiDspId) const;

    // Writes the index of the display that has the CCC ID uiCCCDspId into uiDspId
    bool            getDisplayIdFromCCCID(unsigned int uiCCCDspId, unsigned int& uiDspId) const;

    // Checks that the value passes as internal display ID is a valid display ID
    bool            checkInternalDisplayID(unsigned int uiInternalDspId) const;

    // returns the display id of the primary display.
    unsigned int    getPrimaryDisplay() const;

    // Returns the display Id as seen by windows e.g. in the Screen Resolution dialogue.
    unsigned int    getWindowsDisplayId(unsigned int uiDspId) const;

    // Returns the desktop ID as seen in the Catalyst Control Center.
    unsigned int    getDesktopId(unsigned int uiDisplayId) const;

    // Returns the GPU ID of the display uiDisplay.
    unsigned int    getGpuId(unsigned int uiDisplayId) const;

    unsigned int    getBusNumber(unsigned int uiDisplayId) const;

    // Gets the name of the display.
    std::string     getDisplayName(unsigned int uiDisplayId) const;

    // Gets the name of the connected monitor.
    std::string     getMonitorName(unsigned int uiDisplayId) const;

    float           getDesktopRotation(unsigned int uiDisplayId) const;

    int             getOriginX(unsigned int uiDisplayId) const;

    int             getOriginY(unsigned int uiDisplayId) const;

    unsigned int    getWidth(unsigned int uiDisplayId) const;

    unsigned int    getHeight(unsigned int uiDisplayId) const;

private:

    void            deleteDisplays();

    const ADLWrapper&           m_adlCalls;
    unsigned int                m_uiNumGPU;

    std::vector<DisplayData*>   m_Displays;
};