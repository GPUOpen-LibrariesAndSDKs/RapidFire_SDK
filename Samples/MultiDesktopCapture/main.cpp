
/////////////////////////////////////////////////////////////////////////////////////////
//
// MultiDesktopCapture shows how to use RapidFire to grab multiple desktops.
//
// For each desktop a RFSession is created. The session will use the identity encoder 
// since we only want to capture the desktop. 
// Each session will do the processing in its own thread.
// 
/////////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include <GL/glew.h>
#include <GL/wglew.h>
#include <windows.h>

#include "GLWindow.h"
#include "GLTexRenderer.h"
#include "RapidFireServer.h"
#include "RFWrapper.hpp"

const RFWrapper& g_rfDll = RFWrapper::getInstance();

bool             g_bDone = false;
std::atomic<int> g_threadsTerminated = 0;

void RapidFireThread(unsigned int uiDisplayId, LONG wndPosX, LONG wndPosY, DWORD screenWidth, DWORD screenHeight)
{
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    RFProperties props[] = { RF_ENCODER,                  static_cast<RFProperties>(RF_IDENTITY),
                             RF_DESKTOP_DSP_ID,           static_cast<RFProperties>(uiDisplayId),
                             RF_DESKTOP_UPDATE_ON_CHANGE, static_cast<RFProperties>(1),
                             RF_FLIP_SOURCE,              static_cast<RFProperties>(1),
                             RF_ASYNC_SOURCE_COPY,        static_cast<RFProperties>(1),
                             0 };

    rfStatus = g_rfDll.rfFunc.rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        g_bDone = true;
        return;
    }

    RFProperties encoderProps[] = { RF_ENCODER_FORMAT, static_cast<RFProperties>(RF_RGBA8),
                                    0 };

    rfStatus = g_rfDll.rfFunc.rfCreateEncoder2(rfSession, screenWidth, screenHeight, encoderProps);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF encoder!", "RF Error", MB_ICONERROR | MB_OK);
        g_rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        g_bDone = true;
        return;
    }

    GLWindow win("Diff Desktop Encoding", screenWidth / 2, screenHeight / 2, wndPosX, wndPosY, false);

    if (!win)
    {
        MessageBox(NULL, "Failed to create output window!", "RF Error", MB_ICONERROR | MB_OK);
        g_rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        g_bDone = true;
        return;
    }

    win.open();

    GLTexRenderer renderer(screenWidth, screenHeight);

    if (!renderer.init())
    {
        MessageBox(NULL, "Failed to init GLDesktopRenderer!", "RF Error", MB_ICONERROR | MB_OK);
        g_rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        g_bDone = true;
        return;
    }

    unsigned int uiBufferIndex = 0;
    unsigned int uiTexSize     = 0;
    void*        pPixels       = nullptr;

    g_rfDll.rfFunc.rfEncodeFrame(rfSession, uiBufferIndex);
    uiBufferIndex = 1 - uiBufferIndex;

    MSG msg = {};

    for (;;)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT || g_bDone)
        {
            g_bDone = true;
            break;
        }

        glViewport(0, 0, win.getWidth(), win.getHeight());

        rfStatus = g_rfDll.rfFunc.rfEncodeFrame(rfSession, uiBufferIndex);

        if (rfStatus == RF_STATUS_OK)
        {
            rfStatus = g_rfDll.rfFunc.rfGetEncodedFrame(rfSession, &uiTexSize, &pPixels);

            if (rfStatus == RF_STATUS_OK)
            {
                renderer.updateTexture(static_cast<char*>(pPixels));
            }
        }

        renderer.draw();
        SwapBuffers(win.getDC());

        uiBufferIndex = 1 - uiBufferIndex;
    }

    g_rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
}
 

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    if (!g_rfDll)
    {
        MessageBox(NULL, "Failed to load RapidFire library!", "RF Error", MB_ICONERROR | MB_OK);
        return - 1;
    }

    std::vector<std::thread> threadList;

    DISPLAY_DEVICE	DisplayDevice = {};
    DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
        
    // Enumerate displays and create a window if the display is mapped.
    for (int nDevice = 0; EnumDisplayDevices(NULL, nDevice, &DisplayDevice, 0) != 0; ++nDevice)
    {
        if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {
            HDC hdc = CreateDC(NULL, DisplayDevice.DeviceName, NULL, nullptr);

            if (hdc)
            {
                DEVMODE DevMode = {};
                DevMode.dmSize = sizeof(DEVMODE);

                // Get display settings
                if (EnumDisplaySettings(DisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &DevMode))
                {
                    // Extract Win display id from display name
                    const std::string strKey("DISPLAY");
                    const std::string deviceName(DisplayDevice.DeviceName);

                    size_t pos = deviceName.find(strKey);
                    if (pos == std::string::npos)
                    {
                        continue;
                    }

                    std::stringstream convert(deviceName.substr(pos + strKey.length()));
                    unsigned int uiDisplayId = 0;
                    convert >> uiDisplayId;

                    threadList.push_back(std::thread(RapidFireThread, uiDisplayId, DevMode.dmPosition.x, DevMode.dmPosition.y, DevMode.dmPelsWidth, DevMode.dmPelsHeight));
                }

                DeleteDC(hdc);
            }
        }
    }
    
    // Wait for threads to terminate
    for (auto& t : threadList)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    threadList.clear();
    
    return 0;
}