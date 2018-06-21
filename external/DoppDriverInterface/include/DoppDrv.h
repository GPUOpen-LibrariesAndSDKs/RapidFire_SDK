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

#include <string>
#include <vector>

#include <Windows.h>

#include "..\..\RapidFire\include\RapidFire.h"

class  ADLWrapper;
class  GPU_LIST_ENTRY;

typedef enum _DOPPEventType
{
    DOPP_CURSORHIDE_EVENT = 0,
    DOPP_MOUSE_EVENT,
    DOPP_CURSORSHOW_EVENT,
    DOPP_DESKOTOP_EVENT,
    DOPP_MAX_EVENT,
} DOPPEventType;

// DOPPDrvInterface can be used by applications to query the DOPP state and to register
// DOPP specific events like DOPP_DESKOTOP_EVENT or DOPP_MOUSE_EVENT.
// The application can create user events and should delete them by calling deleteDOPPEvent.
// It is allowed to register multiple events. Signaling is done based on GPU events.
// If a GPU event of a certain type occurs all user events of the same type will get signaled.
class DOPPDrvInterface
{
public:

    DOPPDrvInterface(const std::string& strDisplayName, unsigned int uiBusId);
    DOPPDrvInterface();

    // The destructor will un-register all kmd events.
    // If DOPP was enabled by calling enableDOPP, the destructor will disable DOPP.
    ~DOPPDrvInterface();

    unsigned int getNumGpus() const { return m_numAMDGpus; }

    // Gets current DOPP state.
    bool getDoppState();

    // Tries to enable DOPP.
    // This will only work on drivers that have DOPP support enabled.
    bool enableDopp();

    // Disbales DOPP, should only be called if DOPP was enabled by calling enableDopp.
    bool disableDopp();

    bool getPrimarySurfacePixelFormat(RFFormat& format) const;

    bool isRunningInVirtualEnvironmant() const;

    bool getCursorVisibility() const;

    // Creates and registers a user event that gets signaled by the driver.
    HANDLE createDOPPEvent(DOPPEventType eventType);

    // Removes a user event.
    void deleteDOPPEvent(HANDLE hEvent);

private:

    void init(const std::string& strDisplayName, unsigned int uiBusId);

    struct GPU_USER_EVENT
    {
        HANDLE          hEvent;
        DOPPEventType   EventType;;
    };

    // Builds a static list containing information on all GPUs in the system.
    bool buildGpuList();

    // The device context used for this intance.
    // The DC is required to register events.
    HDC m_hDC;

    unsigned int m_uiDisplayOutputId;

    // List of user events that were registered with createDOPPEvent.
    std::vector<GPU_USER_EVENT> m_UserEventList;

    // Pointer to the entry of the GPU on which this intance is created.
    GPU_LIST_ENTRY* m_pMyGpu;
    unsigned int m_numAMDGpus;

    const ADLWrapper& m_adl;
};