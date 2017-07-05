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

/////////////////////////////////////////////////////////////////////////////////////////
//
// DifferenceEncoding shows how to use RapidFire to compute difference maps of the desktop.
//
// First a RF session is created. The session is configured to capture the desktop only
// if the desktop has changed and to use a difference encoder. Attitudinally it is requested
// to asynchronously copy the source frame to system memory.
// After having received a diff map and the source frame both are rendered into a window
// and the diff regions are highlighted.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <GL/glew.h>
#include <GL/wglew.h>
#include <windows.h>

#include "GLDiffRenderer.h"
#include "GLWindow.h"
#include "RFWrapper.hpp"


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    const RFWrapper& rfDll = RFWrapper::getInstance();

    if (!rfDll)
    {
        MessageBox(NULL, "Failed to load RapidFire library!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    RFProperties props[] = { RF_ENCODER,                  static_cast<RFProperties>(RF_DIFFERENCE),
                             RF_DESKTOP,                  static_cast<RFProperties>(1),
                             RF_DESKTOP_UPDATE_ON_CHANGE, static_cast<RFProperties>(1),
                             RF_FLIP_SOURCE,              static_cast<RFProperties>(1),
                             RF_ASYNC_SOURCE_COPY,        static_cast<RFProperties>(1),
                             0 };

    rfStatus = rfDll.rfFunc.rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    // Get dimension of primary display.
    unsigned int uiStreamWidth  = GetSystemMetrics(SM_CXSCREEN);
    unsigned int uiStreamHeight = GetSystemMetrics(SM_CYSCREEN);

    int BlockX = 128;
    int BlockY = 128;

    RFProperties  encoderProps[] = { RF_ENCODER_FORMAT,       RF_RGBA8,
                                     RF_DIFF_ENCODER_BLOCK_S, BlockX,
                                     RF_DIFF_ENCODER_BLOCK_T, BlockY,
                                     0};

    rfStatus = rfDll.rfFunc.rfCreateEncoder2(rfSession, uiStreamWidth, uiStreamHeight, encoderProps);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF encoder!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    GLWindow win("Diff Desktop Encoding", 800, 600, CW_USEDEFAULT, CW_USEDEFAULT, false);

    if (!win)
    {
        MessageBox(NULL, "Failed to create output window!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    win.open();

    RFProperties diffMapWidth  = 0;
    RFProperties diffMapHeight = 0;

    rfDll.rfFunc.rfGetEncodeParameter(rfSession, RF_ENCODER_OUTPUT_WIDTH,  &diffMapWidth);
    rfDll.rfFunc.rfGetEncodeParameter(rfSession, RF_ENCODER_OUTPUT_HEIGHT, &diffMapHeight);

    GLDiffRenderer renderer(uiStreamWidth, uiStreamHeight, static_cast<unsigned int>(diffMapWidth), static_cast<unsigned int>(diffMapHeight), BlockX, BlockY);

    if (!renderer.init())
    {
        MessageBox(NULL, "Failed to init GLDesktopRenderer!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    void*           pSource         = nullptr;
    void*           pDifference     = nullptr;
    unsigned int    uiDiffSize      = 0;
    unsigned int    uiSourceSize    = 0;

    rfDll.rfFunc.rfEncodeFrame(rfSession, 0);

    MSG msg = {};

    for (;;)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
        {
            break;
        }

        if (win.isMinimized())
        {
            Sleep(100);
            continue;
        }

        RFStatus ret = rfDll.rfFunc.rfEncodeFrame(rfSession, 0);

        if (ret == RF_STATUS_OK)
        {
            bool success = (RF_STATUS_OK == rfDll.rfFunc.rfGetSourceFrame(rfSession, &uiSourceSize, &pSource));
            success &= (RF_STATUS_OK == rfDll.rfFunc.rfGetEncodedFrame(rfSession, &uiDiffSize, &pDifference));

            if (success)
            {
                renderer.updateTexture(static_cast<char*>(pSource), static_cast<char*>(pDifference));
            }
        }

        renderer.draw();
        SwapBuffers(win.getDC());
    }

    rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    return static_cast<int>(msg.wParam);
}