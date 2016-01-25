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
#include "RapidFireServer.h"


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    RFProperties props[] = { RF_ENCODER,                  static_cast<RFProperties>(RF_IDENTITY),
                             RF_DESKTOP,                  static_cast<RFProperties>(1),
                             RF_DESKTOP_UPDATE_ON_CHANGE, static_cast<RFProperties>(1),
                             RF_MOUSE_DATA,               static_cast<RFProperties>(1),
                             0 };

    rfStatus = rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    // Get dimension of primary display. 
    unsigned int uiStreamWidth  = GetSystemMetrics(SM_CXSCREEN);
    unsigned int uiStreamHeight = GetSystemMetrics(SM_CYSCREEN);

    RFProperties encoderProps[] = { RF_ENCODER_FORMAT, RF_RGBA8, 0};

    rfStatus = rfCreateEncoder2(rfSession, uiStreamWidth, uiStreamHeight, encoderProps);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    GLWindow win("Desktop and Mouse Capture", 800, 600, CW_USEDEFAULT, CW_USEDEFAULT, false);

    if (!win)
    {
        MessageBox(NULL, "Failed to create output window!", "RF Error", MB_ICONERROR | MB_OK);
        rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    win.open();

    GLDesktopRenderer renderer(uiStreamWidth, uiStreamHeight, 32, 32);

    if (!renderer.init())
    {
        MessageBox(NULL, "Failed to init GLDesktopRenderer!", "RF Error", MB_ICONERROR | MB_OK);
        rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    RFMouseData     md         = {};
    void*           pDesktop   = nullptr;
    unsigned int    uiSize     = 0;
    unsigned int    uiIndex    = 0;

    rfEncodeFrame(rfSession, uiIndex);
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

        if (rfGetMouseData(rfSession, false, &md) == RF_STATUS_OK)
        {
            // Typically the mouse pointer is 32x32. The texture that was created is 32x32 as well.
            // Some shapes have a larger dimensions, those will not be drawn by this sample
            renderer.updateMouseTexture(static_cast<const unsigned char*>(md.color.pPixels), md.color.uiWidth, md.color.uiHeight, static_cast<const unsigned char*>(md.mask.pPixels), md.mask.uiWidth, md.mask.uiHeight);
        }

        rfStatus = rfEncodeFrame(rfSession, uiIndex);

        if (rfStatus == RF_STATUS_OK)
        {
            rfStatus = rfGetEncodedFrame(rfSession, &uiSize, &pDesktop);

            if (rfStatus == RF_STATUS_OK)
            {
                renderer.updateDesktopTexture(static_cast<char*>(pDesktop));
            }
        }

        renderer.draw();
        SwapBuffers(win.getDC());

        uiIndex = 1 - uiIndex;
    }

    rfDeleteEncodeSession(&rfSession);

    return static_cast<int>(msg.wParam);
}