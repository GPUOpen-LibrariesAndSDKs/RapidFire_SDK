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

#include <mutex>

#include <Windows.h>

#include "RapidFire.h"
#include "RFLock.h"

class DOPPDrvInterface;

class RFMouseGrab
{
public:

    explicit RFMouseGrab(DOPPDrvInterface* pDrv, unsigned int displayId);
    ~RFMouseGrab();

    // Returns mouse shape data.
    // bBlockin [in]: If true the function blocks until a mouse shape change occured.
    // md      [out]: Shape data of the mouse
    //
    // Returns true if the data in md is new, otherwise false. If bBlocking is true, the data in md
    // is always new and the function returns true as well.
    bool    getShapeData(int iBlocking, RFMouseData& md);

    bool    getShapeData2(int iBlocking, RFMouseData2& md);

    // This function will signal m_hNewDataEvent and can be used to unblock a thread
    // that waits for mouse updates.
    bool    releaseEvent();

private:

    struct BitmapBuffer
    {
        unsigned int            uiBufferSize;
        void*                   pBuffer;
        BITMAP                  BitMap;
    };

    static DWORD WINAPI ShapeNotificationThread(void*);

    typedef HCURSOR(WINAPI* GET_CURSOR_FRAME_INFO)(HCURSOR, LPCWSTR, DWORD, DWORD*, DWORD*);

    GET_CURSOR_FRAME_INFO	m_fnGetCursorFrameInfo;

    struct AnimatedCursorInfo
    {
        DWORD	dwFrameIndex;
        DWORD	dwUpdateCounter;
        DWORD	dwDisplayRate;
        DWORD	dwTotalFrames;
        HCURSOR	hAnimatedCursor;
    };

    AnimatedCursorInfo m_animatedCursorInfo;

    HCURSOR m_hcFallBackCursor;

    bool createThreads();
    bool createEvents();

    // Loop that waits until it gets signaled from KMD.
    void updateLoop();
    void updateMouseShapeData(bool bIncrementAnimationIndex, bool bGetMouseVisibility);

    bool copyBitmapToBuffer(const HBITMAP hBitmap, BitmapBuffer& buffer);

    bool m_bRunning;
    bool m_bShapeUpdated;
    bool m_bVisibilityUpdated;

    // Event that gets signaled by updateLoop.
    // If getShapeData is blocking, it needs to wait until m_hNewCursorStateEvent is signaled.
    HANDLE m_hNewCursorStateEvent;

    enum CURSOR_SHAPECHANGE_TYPE
    {
        CURSOR_SHAPE_CHANGED = 0,
        CURSOR_SHAPE_SHOW,
        CURSOR_SHAPE_HIDE,
        MAX_CURSOR_SHAPECHANGE_TYPES,
    };
    // Events that gets signaled by the driver on shape and visibility changes.
    HANDLE m_hShapeChangedEvents[MAX_CURSOR_SHAPECHANGE_TYPES];
    unsigned int m_uiDisplayId;

    // Mutex to safely update mouse data
    RFLock m_MouseDataMutex;

    HANDLE m_hCursorEventsThread;
    DWORD m_dwThreadId;

    DOPPDrvInterface* m_pDrv;

    int m_iVisible;

    struct MouseData
    {
        RFMouseData mouseData;
        BitmapBuffer maskBuffer;
        BitmapBuffer colorBuffer;
    };

    void StoreBitmapBuffer(const BitmapBuffer& src, RFBitmapBuffer& dest);

    MouseData m_renderedMouseData;
    MouseData m_changedMouseData;

    RFMouseGrab(const RFMouseGrab&);
    RFMouseGrab& operator=(const RFMouseGrab& rhs);
};