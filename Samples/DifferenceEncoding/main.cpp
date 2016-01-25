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

    unsigned int BlockX = 128;
    unsigned int BlockY = 128;

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
    unsigned int    uiIndex         = 0;

    rfDll.rfFunc.rfEncodeFrame(rfSession, uiIndex);
    uiIndex = 1 - uiIndex;

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

        rfStatus = rfDll.rfFunc.rfEncodeFrame(rfSession, uiIndex);

        if (rfStatus == RF_STATUS_OK)
        {
            rfStatus  = rfDll.rfFunc.rfGetSourceFrame(rfSession, &uiSourceSize, &pSource);
            rfStatus |= rfDll.rfFunc.rfGetEncodedFrame(rfSession, &uiDiffSize, &pDifference);

            if (rfStatus == RF_STATUS_OK)
            {
                renderer.updateTexture(static_cast<char*>(pSource), static_cast<char*>(pDifference));
            }
        }

        renderer.draw();
        SwapBuffers(win.getDC());

        uiIndex = 1 - uiIndex;
    }

    rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    return static_cast<int>(msg.wParam);
}