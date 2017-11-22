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
// DesktopMouse shows how to use RapidFire to grab the desktop and mouse shape data.
//
// A RF Session is created that is configured to capture the desktop and to provide
// mouse shape data. Since no encoding is required, the identity encoder is used.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <GL/glew.h>
#include <GL/wglew.h>
#include <windows.h>

#include "GLDesktopRenderer.h"
#include "GLWindow.h"
#include "RFWrapper.hpp"


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    const RFWrapper& rfDll = RFWrapper::getInstance();

    GLWindow win("Desktop and Mouse Capture", 800, 600, CW_USEDEFAULT, CW_USEDEFAULT, false);

    if (!win)
    {
        MessageBox(NULL, "Failed to create output window!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    RFProperties props[] = { RF_ENCODER,                  static_cast<RFProperties>(RF_IDENTITY),
                             RF_DESKTOP,                  static_cast<RFProperties>(1),
                             RF_DESKTOP_UPDATE_ON_CHANGE, static_cast<RFProperties>(1),
                             RF_MOUSE_DATA,               static_cast<RFProperties>(1),
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

    RFProperties encoderProps[] = { RF_ENCODER_FORMAT, RF_RGBA8, 0};

    rfStatus = rfDll.rfFunc.rfCreateEncoder2(rfSession, uiStreamWidth, uiStreamHeight, encoderProps);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    win.open();

    GLDesktopRenderer renderer(uiStreamWidth, uiStreamHeight, 32, 32);

    if (!renderer.init())
    {
        MessageBox(NULL, "Failed to init GLDesktopRenderer!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    void*           pDesktopTexture = nullptr;
    unsigned int    uiDesktopTextureSize = 0;
    RFMouseData     md = {};
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

        if (rfDll.rfFunc.rfGetMouseData(rfSession, 0, &md) == RF_STATUS_OK)
        {
            // Typically the mouse pointer is 32x32. The texture that was created is 32x32 as well.
            // Some shapes have a larger dimensions, those will not be drawn by this sample
            renderer.updateMouseTexture(static_cast<const unsigned char*>(md.color.pPixels), md.color.uiWidth, md.color.uiHeight,
                                        static_cast<const unsigned char*>(md.mask.pPixels), md.mask.uiWidth, md.mask.uiHeight, md.mask.uiPitch);
        }

        rfDll.rfFunc.rfEncodeFrame(rfSession, 0);

        rfStatus = rfDll.rfFunc.rfGetEncodedFrame(rfSession, &uiDesktopTextureSize, &pDesktopTexture);

        if (rfStatus == RF_STATUS_OK)
        {
            renderer.updateDesktopTexture(static_cast<char*>(pDesktopTexture));
        }

        renderer.draw();
        SwapBuffers(win.getDC());
    }

    rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    return static_cast<int>(msg.wParam);
}