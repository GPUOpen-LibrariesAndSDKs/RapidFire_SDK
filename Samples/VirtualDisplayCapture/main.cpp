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
// VirtualDesktopCapture shows how to create a virtual display and capture it 
// with RapidFire.
//
// A virtual display is created with ADL if the adapter has a free connector.
// Then a RapidFire session is created, that will capture the virtual display.
// The session will use the identity encoder since we only want to capture the desktop.
//
// Notes: - RapidFire currently only supports one GPU and capturing of displays,
//          that are created on the GPU with the primary display.
//        - If the virtual display capture shows black borders around the captured desktop
//          the registry key "KMD_Wddm2DisplayVirtualModeSupport" has to be set to 0.
// 
/////////////////////////////////////////////////////////////////////////////////////////

#include <vector>

#include <adl_sdk.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <windows.h>

#include "GLWindow.h"
#include "GLTexRenderer.h"
#include "RFWrapper.hpp"

typedef int(*ADL2_MAIN_CONTROL_CREATE) (ADL_MAIN_MALLOC_CALLBACK, int, ADL_CONTEXT_HANDLE*);
typedef int(*ADL2_MAIN_CONTROL_DESTROY) (ADL_CONTEXT_HANDLE);
typedef int(*ADL2_DISPLAY_POSSIBLEMODE_GET) (ADL_CONTEXT_HANDLE, int, int*, ADLMode**);
typedef int(*ADL2_DISPLAY_MODES_GET) (ADL_CONTEXT_HANDLE, int, int, int*, ADLMode**);
typedef int(*ADL2_DISPLAY_MODES_SET) (ADL_CONTEXT_HANDLE, int, int, int, ADLMode*);
typedef int(*ADL2_ADAPTER_NUMBEROFADAPTERS_GET) (ADL_CONTEXT_HANDLE, int*);
typedef int(*ADL2_ADAPTER_ADAPTERINFO_GET) (ADL_CONTEXT_HANDLE, LPAdapterInfo, int);
typedef int(*ADL2_ADAPTERX2_CAPS) (ADL_CONTEXT_HANDLE, int, int*);
typedef int(*ADL2_ADAPTER_CONNECTIONSTATE_GET) (ADL_CONTEXT_HANDLE, int, ADLDevicePort, ADLConnectionState*);
typedef int(*ADL2_ADAPTER_SUPPORTEDCONNECTIONS_GET) (ADL_CONTEXT_HANDLE, int, ADLDevicePort, ADLSupportedConnections*);
typedef int(*ADL2_ADAPTER_CONNECTIONDATA_SET) (ADL_CONTEXT_HANDLE, int, ADLDevicePort, ADLConnectionData);
typedef int(*ADL2_ADAPTER_CONNECTIONDATA_REMOVE) (ADL_CONTEXT_HANDLE, int, ADLDevicePort);
typedef int(*ADL2_ADAPTER_EMULATIONMODE_SET) (ADL_CONTEXT_HANDLE, int, ADLDevicePort, int);
typedef int(*ADL2_DISPLAY_MODETIMINGOVERRIDEX2_GET) (ADL_CONTEXT_HANDLE, int, ADLDisplayID, ADLDisplayModeX2*, ADLDisplayModeInfo*);
typedef int(*ADL2_DISPLAY_MODETIMINGOVERRIDE_SET) (ADL_CONTEXT_HANDLE, int, int, ADLDisplayModeInfo*, int);
typedef int(*ADL2_ADAPTER_MODETIMINGOVERRIDE_CAPS) (ADL_CONTEXT_HANDLE, int, int*);
typedef int(*ADL2_DISPLAY_MODETIMINGOVERRIDE_DELETE) (ADL_CONTEXT_HANDLE, int, ADLDisplayID, ADLDisplayModeX2*, int);

HINSTANCE hADLDll;

ADL2_MAIN_CONTROL_CREATE                 ADL2_Main_Control_Create = NULL;
ADL2_MAIN_CONTROL_DESTROY                ADL2_Main_Control_Destroy = NULL;
ADL2_DISPLAY_POSSIBLEMODE_GET            ADL2_Display_PossibleMode_Get = NULL;
ADL2_DISPLAY_MODES_SET                   ADL2_Display_Modes_Set = NULL;
ADL2_DISPLAY_MODES_GET                   ADL2_Display_Modes_Get = NULL;
ADL2_ADAPTER_NUMBEROFADAPTERS_GET        ADL2_Adapter_NumberOfAdapters_Get = NULL;
ADL2_ADAPTER_ADAPTERINFO_GET             ADL2_Adapter_AdapterInfo_Get = NULL;
ADL2_ADAPTERX2_CAPS                      ADL2_AdapterX2_Caps = NULL;
ADL2_ADAPTER_CONNECTIONSTATE_GET         ADL2_Adapter_ConnectionState_Get = NULL;
ADL2_ADAPTER_SUPPORTEDCONNECTIONS_GET    ADL2_Adapter_SupportedConnections_Get = NULL;
ADL2_ADAPTER_CONNECTIONDATA_SET          ADL2_Adapter_ConnectionData_Set = NULL;
ADL2_ADAPTER_CONNECTIONDATA_REMOVE       ADL2_Adapter_ConnectionData_Remove = NULL;
ADL2_ADAPTER_EMULATIONMODE_SET           ADL2_Adapter_EmulationMode_Set = NULL;
ADL2_DISPLAY_MODETIMINGOVERRIDEX2_GET    ADL2_Display_ModeTimingOverrideX2_Get = NULL;
ADL2_DISPLAY_MODETIMINGOVERRIDE_SET      ADL2_Display_ModeTimingOverride_Set = NULL;
ADL2_ADAPTER_MODETIMINGOVERRIDE_CAPS     ADL2_Adapter_ModeTimingOverride_Caps = NULL;
ADL2_DISPLAY_MODETIMINGOVERRIDE_DELETE   ADL2_Display_ModeTimingOverride_Delete = NULL;


void* __stdcall ADL_Main_Memory_Alloc(int iSize)
{
    void* lpBuffer = malloc(iSize);
    return lpBuffer;
}

void __stdcall ADL_Main_Memory_Free(void** lpBuffer)
{
    if (nullptr != *lpBuffer)
    {
        free(*lpBuffer);
        *lpBuffer = nullptr;
    }
}

bool InitializeADL();
bool setConnectionData(int iAdapterIndex, const ADLDevicePort& devicePort, int iConnectionType, const char* FileName);
bool removeConnectionData(int iAdapterIndex, const ADLDevicePort& devicePort, int iPreviousEmulationMode);

const unsigned int g_uiVirtualDisplayWidth = 1028;
const unsigned int g_uiVirtualDisplayHeight = 1028;
const unsigned int g_uiRefreshRate = 60;
const RFWrapper& g_rfDll = RFWrapper::getInstance();
bool g_bAddedCustomResolution = false;
ADL_CONTEXT_HANDLE g_adlContext = NULL;
ADLDisplayID g_customDisplayID;
ADLDisplayModeX2 g_customDisplayMode;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    if (!g_rfDll)
    {
        MessageBox(NULL, "Failed to load RapidFire library!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    if (!InitializeADL())
    {
        return -1;
    }

    int iNumberAdapters = 0;
    if (ADL_OK != ADL2_Adapter_NumberOfAdapters_Get(g_adlContext, &iNumberAdapters))
    {
        MessageBox(NULL, "Failed to get number of adapters!", "ADL Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    if (iNumberAdapters < 1)
    {
        MessageBox(NULL, "No adapter found!", "ADL Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    // Find first AMD adapter with a free connector and the connector id
    std::vector<AdapterInfo> adapterInfo(iNumberAdapters);
    ADL2_Adapter_AdapterInfo_Get(g_adlContext, adapterInfo.data(), sizeof(AdapterInfo) * iNumberAdapters);

    int iAmdAdapterIndex = 0;
    ADLDevicePort devicePort;
    ADLConnectionState connectionState;
    bool bAdapterFound = false;

    while (!bAdapterFound)
    {
        for (; iAmdAdapterIndex < iNumberAdapters && adapterInfo[iAmdAdapterIndex].iVendorID != 1002; ++iAmdAdapterIndex);

        if (iAmdAdapterIndex == iNumberAdapters)
        {
            break;
        }

        // Find the first free connector of the adapter
        ADLAdapterCapsX2 adapterCapability;
        if (ADL_OK != ADL2_AdapterX2_Caps(g_adlContext, iAmdAdapterIndex, reinterpret_cast<int*>(&adapterCapability)))
        {
            MessageBox(NULL, "ADL_AdapterX2_Caps failed!", "ADL Error", MB_ICONERROR | MB_OK);
            return -1;
        }

        for (int iConnectorIndex = 0; iConnectorIndex < adapterCapability.iNumConnectors; ++iConnectorIndex)
        {
            devicePort.iConnectorIndex = iConnectorIndex;
            devicePort.aMSTRad.iLinkNumber = 1;
            devicePort.aMSTRad.rad[0] = 0;

            if (ADL_OK != ADL2_Adapter_ConnectionState_Get(g_adlContext, iAmdAdapterIndex, devicePort, &connectionState))
            {
                MessageBox(NULL, "ADL_Adapter_ConnectionState_Get failed!", "ADL Error", MB_ICONERROR | MB_OK);
                return -1;
            }

            if (connectionState.iDisplayIndex == -1)
            {
                bAdapterFound = true;
                break;
            }
        }

        if (bAdapterFound)
        {
            break;
        }

        ++iAmdAdapterIndex;
    }

    if (!bAdapterFound)
    {
        MessageBox(NULL, "No AMD adapter with a free connector found!", "ADL Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    // Create a virtual display
    if (!setConnectionData(iAmdAdapterIndex, devicePort, ADL_CONNECTION_TYPE_VIRTUAL, "EDID_Emulation.bin"))
    {
        if (!setConnectionData(iAmdAdapterIndex, devicePort, ADL_CONNECTION_TYPE_DISPLAY_PORT, "EDID_Emulation.bin"))
        {
            MessageBox(NULL, "Provided connection type is not supported on this port!\n", "ADL Error", MB_ICONERROR | MB_OK);
            return -1;
        }
    }

    int iPreviousEmulationMode = connectionState.iEmulationMode;
    if (ADL_OK != ADL2_Adapter_EmulationMode_Set(g_adlContext, iAmdAdapterIndex, devicePort, ADL_EMUL_MODE_ALWAYS))
    {
        MessageBox(NULL, "ADL_Adapter_EmulationMode_Set failed!", "ADL Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    if (ADL_OK != ADL2_Adapter_ConnectionState_Get(g_adlContext, iAmdAdapterIndex, devicePort, &connectionState))
    {
        MessageBox(NULL, "ADL_Adapter_ConnectionState_Get failed!", "ADL Error", MB_ICONERROR | MB_OK);
        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
        return -1;
    }

    int iNumModes;
    LPADLMode lpADLModes;
    if (ADL_OK != ADL2_Display_Modes_Get(g_adlContext, -1, -1, &iNumModes, &lpADLModes))
    {
        MessageBox(NULL, "ADL_Display_Modes_Get failed!", "ADL Error", MB_ICONERROR | MB_OK);
        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
        return -1;
    }

    // Get the display id of the virtual display
    unsigned int uiWindowsDisplayId = 0;
    for (int iMode = 0; iMode < iNumModes; ++iMode)
    {
        if (lpADLModes[iMode].displayID.iDisplayLogicalIndex == connectionState.iDisplayIndex && lpADLModes[iMode].displayID.iDisplayLogicalAdapterIndex < iNumberAdapters
            && adapterInfo[iAmdAdapterIndex].iBusNumber == adapterInfo[lpADLModes[iMode].displayID.iDisplayLogicalAdapterIndex].iBusNumber)
        {
            int iAdapterIndex = lpADLModes[iMode].displayID.iDisplayLogicalAdapterIndex;
            int iDisplayIndex = lpADLModes[iMode].displayID.iDisplayLogicalIndex;
            std::string strDisplName(adapterInfo[iAdapterIndex].strDisplayName);
            std::string strKey("DISPLAY");
            size_t pos = strDisplName.find(strKey);
            if (pos != std::string::npos && (pos + strKey.length()) < strDisplName.length())
            {
                pos += strKey.length();
                uiWindowsDisplayId = atoi(&strDisplName[pos]);

                // Set the resolution and refresh rate of the virtual display
                int iNumModes;
                LPADLMode lpADLSupportedModes;

                if (ADL_OK != ADL2_Display_PossibleMode_Get(g_adlContext, iAdapterIndex, &iNumModes, &lpADLSupportedModes))
                {
                    MessageBox(NULL, "ADL_Display_PossibleMode_Get failed!", "ADL Error", MB_ICONERROR | MB_OK);
                    removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
                    return -1;
                }

                int iSelectedMode = 0;
                for (; iSelectedMode < iNumModes; ++iSelectedMode)
                {
                    if (lpADLSupportedModes[iSelectedMode].iXRes == g_uiVirtualDisplayWidth && 
                        lpADLSupportedModes[iSelectedMode].iYRes == g_uiVirtualDisplayHeight &&
                        lpADLSupportedModes[iSelectedMode].fRefreshRate == g_uiRefreshRate &&
                        lpADLSupportedModes[iSelectedMode].iColourDepth == 32)
                    {
                        break;
                    }
                }
                
                // Display mode not available so we have to add a custom display mode if the display supports it
                if (iSelectedMode == iNumModes)
                {
                    int iOverrideSupported = 0;
                    if (ADL_OK != ADL2_Adapter_ModeTimingOverride_Caps(g_adlContext, iAdapterIndex, &iOverrideSupported))
                    {
                        MessageBox(NULL, "ADL_Adapter_ModeTimingOverride_Caps failed!", "ADL Error", MB_ICONERROR | MB_OK);
                        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
                        return -1;
                    }

                    if (ADL_FALSE == iOverrideSupported)
                    {
                        MessageBox(NULL, "Requested display mode is not supported and adapter does not support adding custom resolutions!", "ADL Error", MB_ICONERROR | MB_OK);
                        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
                        return -1;
                    }

                    g_customDisplayMode.iWidth = g_uiVirtualDisplayWidth;
                    g_customDisplayMode.iHeight = g_uiVirtualDisplayHeight;
                    g_customDisplayMode.iRefreshRate = g_uiRefreshRate;
                    g_customDisplayMode.iScanType = 0; // 0 = progressive / ADL_DL_TIMINGFLAG_INTERLACED = interlaced
                    g_customDisplayMode.iTimingStandard = ADL_DL_MODETIMING_STANDARD_CVT;
                    g_customDisplayID = lpADLModes[iMode].displayID;

                    ADLDisplayModeInfo dspModeInfo = {};

                    if (ADL_OK != ADL2_Display_ModeTimingOverrideX2_Get(g_adlContext, iAdapterIndex, lpADLModes[iMode].displayID, &g_customDisplayMode, &dspModeInfo))
                    {
                        MessageBox(NULL, "ADL_Display_ModeTimingOverrideX2_Get failed!", "ADL Error", MB_ICONERROR | MB_OK);
                        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
                        return -1;
                    }

                    if (ADL_OK != ADL2_Display_ModeTimingOverride_Set(g_adlContext, iAdapterIndex, iDisplayIndex, &dspModeInfo, 1))
                    {
                        MessageBox(NULL, "ADL_Display_ModeTimingOverride_Set failed!", "ADL Error", MB_ICONERROR | MB_OK);
                        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
                        return -1;
                    }

                    g_bAddedCustomResolution = true;
                }

                lpADLModes[iMode].iXRes = g_uiVirtualDisplayWidth;
                lpADLModes[iMode].iYRes = g_uiVirtualDisplayHeight;
                lpADLModes[iMode].fRefreshRate = g_uiRefreshRate;
                lpADLModes[iMode].iColourDepth = 32;

                if (ADL_OK != ADL2_Display_Modes_Set(g_adlContext, iAdapterIndex, iDisplayIndex, 1, &lpADLModes[iMode]))
                {
                    MessageBox(NULL, "ADL_Display_Modes_Set failed!", "ADL Error", MB_ICONERROR | MB_OK);
                    removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
                    return -1;
                }

                ADL_Main_Memory_Free(reinterpret_cast<void**>(&lpADLSupportedModes));
            }

            break;
        }
    }

    ADL_Main_Memory_Free(reinterpret_cast<void**>(&lpADLModes));

    if (uiWindowsDisplayId == 0)
    {
        MessageBox(NULL, "Virtual display index not found!", "ADL Error", MB_ICONERROR | MB_OK);
        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
        return -1;
    }

    RFProperties props[] = { RF_ENCODER,                  static_cast<RFProperties>(RF_IDENTITY),
                             RF_DESKTOP_DSP_ID,           static_cast<RFProperties>(uiWindowsDisplayId),
                             RF_FLIP_SOURCE,              static_cast<RFProperties>(1),
                             RF_ASYNC_SOURCE_COPY,        static_cast<RFProperties>(1),
                             0 };

    rfStatus = g_rfDll.rfFunc.rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
        return -1;
    }

    RFProperties encoderProps[] = { RF_ENCODER_FORMAT, static_cast<RFProperties>(RF_RGBA8),
                                    0 };

    rfStatus = g_rfDll.rfFunc.rfCreateEncoder2(rfSession, g_uiVirtualDisplayWidth, g_uiVirtualDisplayHeight, encoderProps);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF encoder!", "RF Error", MB_ICONERROR | MB_OK);
        g_rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
        return -1;
    }

    GLWindow win("Diff Desktop Encoding", g_uiVirtualDisplayWidth / 2, g_uiVirtualDisplayHeight / 2, CW_USEDEFAULT, CW_USEDEFAULT, false);

    if (!win)
    {
        MessageBox(NULL, "Failed to create output window!", "RF Error", MB_ICONERROR | MB_OK);
        g_rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
        return -1;
    }

    win.open();

    GLTexRenderer renderer(g_uiVirtualDisplayWidth, g_uiVirtualDisplayHeight);

    if (!renderer.init())
    {
        MessageBox(NULL, "Failed to init GLDesktopRenderer!", "RF Error", MB_ICONERROR | MB_OK);
        g_rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode);
        return -1;
    }

    unsigned int uiTexSize = 0;
    void*        pPixels = nullptr;

    g_rfDll.rfFunc.rfEncodeFrame(rfSession, 0);

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

        g_rfDll.rfFunc.rfEncodeFrame(rfSession, 0);

        rfStatus = g_rfDll.rfFunc.rfGetEncodedFrame(rfSession, &uiTexSize, &pPixels);

        if (rfStatus == RF_STATUS_OK)
        {
            renderer.updateTexture(static_cast<char*>(pPixels));
        }
        else
        {
            Sleep(0);
        }

        renderer.draw();
        SwapBuffers(win.getDC());
    }

    g_rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    if (!removeConnectionData(iAmdAdapterIndex, devicePort, iPreviousEmulationMode))
    {
        return -1;
    }

    ADL2_Main_Control_Destroy(g_adlContext);

    FreeLibrary(hADLDll);

    return static_cast<int>(msg.wParam);
}

bool InitializeADL()
{
    // Load the ADL dll
    {
        hADLDll = LoadLibrary(TEXT("atiadlxx.dll"));
        if (hADLDll == NULL)
        {
            // A 32 bit calling application on 64 bit OS will fail to LoadLibrary.
            // Try to load the 32 bit library (atiadlxy.dll) instead
            hADLDll = LoadLibrary(TEXT("atiadlxy.dll"));
        }

        if (NULL == hADLDll)
        {
            MessageBox(NULL, "Failed to load ADL library\n", "ADL Error", MB_ICONERROR | MB_OK);
            return false;
        }
    }
    {
        ADL2_Main_Control_Create = (ADL2_MAIN_CONTROL_CREATE)GetProcAddress(hADLDll, "ADL2_Main_Control_Create");
        ADL2_Main_Control_Destroy = (ADL2_MAIN_CONTROL_DESTROY)GetProcAddress(hADLDll, "ADL2_Main_Control_Destroy");
        ADL2_Display_PossibleMode_Get = (ADL2_DISPLAY_POSSIBLEMODE_GET)GetProcAddress(hADLDll, "ADL2_Display_PossibleMode_Get");
        ADL2_Display_Modes_Get = (ADL2_DISPLAY_MODES_GET)GetProcAddress(hADLDll, "ADL2_Display_Modes_Get");
        ADL2_Display_Modes_Set = (ADL2_DISPLAY_MODES_SET)GetProcAddress(hADLDll, "ADL2_Display_Modes_Set");
        ADL2_Adapter_NumberOfAdapters_Get = (ADL2_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(hADLDll, "ADL2_Adapter_NumberOfAdapters_Get");
        ADL2_Adapter_AdapterInfo_Get = (ADL2_ADAPTER_ADAPTERINFO_GET)GetProcAddress(hADLDll, "ADL2_Adapter_AdapterInfo_Get");
        ADL2_AdapterX2_Caps = (ADL2_ADAPTERX2_CAPS)GetProcAddress(hADLDll, "ADL2_AdapterX2_Caps");
        ADL2_Adapter_ConnectionState_Get = (ADL2_ADAPTER_CONNECTIONSTATE_GET)GetProcAddress(hADLDll, "ADL2_Adapter_ConnectionState_Get");
        ADL2_Adapter_SupportedConnections_Get = (ADL2_ADAPTER_SUPPORTEDCONNECTIONS_GET)GetProcAddress(hADLDll, "ADL2_Adapter_SupportedConnections_Get");
        ADL2_Adapter_ConnectionData_Set = (ADL2_ADAPTER_CONNECTIONDATA_SET)GetProcAddress(hADLDll, "ADL2_Adapter_ConnectionData_Set");
        ADL2_Adapter_EmulationMode_Set = (ADL2_ADAPTER_EMULATIONMODE_SET)GetProcAddress(hADLDll, "ADL2_Adapter_EmulationMode_Set");
        ADL2_Adapter_ConnectionData_Remove = (ADL2_ADAPTER_CONNECTIONDATA_REMOVE)GetProcAddress(hADLDll, "ADL2_Adapter_ConnectionData_Remove");
        ADL2_Display_ModeTimingOverrideX2_Get = (ADL2_DISPLAY_MODETIMINGOVERRIDEX2_GET)GetProcAddress(hADLDll, "ADL2_Display_ModeTimingOverrideX2_Get");
        ADL2_Display_ModeTimingOverride_Set = (ADL2_DISPLAY_MODETIMINGOVERRIDE_SET)GetProcAddress(hADLDll, "ADL2_Display_ModeTimingOverride_Set");
        ADL2_Adapter_ModeTimingOverride_Caps = (ADL2_ADAPTER_MODETIMINGOVERRIDE_CAPS)GetProcAddress(hADLDll, "ADL2_Adapter_ModeTimingOverride_Caps");
        ADL2_Display_ModeTimingOverride_Delete = (ADL2_DISPLAY_MODETIMINGOVERRIDE_DELETE)GetProcAddress(hADLDll, "ADL2_Display_ModeTimingOverride_Delete");
        if (NULL == ADL2_Main_Control_Create ||
            NULL == ADL2_Main_Control_Destroy||
            NULL == ADL2_Display_PossibleMode_Get ||
            NULL == ADL2_Display_Modes_Get ||
            NULL == ADL2_Display_Modes_Set ||
            NULL == ADL2_Adapter_NumberOfAdapters_Get ||
            NULL == ADL2_Adapter_AdapterInfo_Get ||
            NULL == ADL2_AdapterX2_Caps ||
            NULL == ADL2_Adapter_ConnectionState_Get ||
            NULL == ADL2_Adapter_SupportedConnections_Get ||
            NULL == ADL2_Adapter_ConnectionData_Set ||
            NULL == ADL2_Adapter_ConnectionData_Remove ||
            NULL == ADL2_Adapter_EmulationMode_Set ||
            NULL == ADL2_Display_ModeTimingOverrideX2_Get ||
            NULL == ADL2_Display_ModeTimingOverride_Set ||
            NULL == ADL2_Adapter_ModeTimingOverride_Caps ||
            NULL == ADL2_Display_ModeTimingOverride_Delete)
        {
            MessageBox(NULL, "Failed to get ADL function pointers\n", "ADL Error", MB_ICONERROR | MB_OK);
            return false;
        }
    }
    if (ADL_OK != ADL2_Main_Control_Create(ADL_Main_Memory_Alloc, 1, &g_adlContext))
    {
        MessageBox(NULL, "ADL_Main_Control_Create() failed\n", "ADL Error", MB_ICONERROR | MB_OK);
        return false;
    }

    return true;
}

bool setConnectionData(int iAdapterIndex, const ADLDevicePort& devicePort, int iConnectionType, const char* FileName)
{
    ADLConnectionData connectionData;
    ADLSupportedConnections supportedConnections;
    FILE* ptr_myfile;

    ptr_myfile = fopen(FileName, "rb");
    if (!ptr_myfile)
    {
        MessageBox(NULL, "Opening EDID binary failed!\n", "ADL Error", MB_ICONERROR | MB_OK);
        return false;
    }
    else
    {
        fseek(ptr_myfile, 0L, SEEK_END);
        connectionData.iDataSize = ftell(ptr_myfile);
        if (connectionData.iDataSize > ADL_MAX_DISPLAY_EDID_DATA_SIZE)
        {
            fclose(ptr_myfile);
            MessageBox(NULL, "EDID binary file is too large!\n", "ADL Error", MB_ICONERROR | MB_OK);
            return false;
        }
        fseek(ptr_myfile, 0L, SEEK_SET);
        fread(connectionData.EdidData, connectionData.iDataSize, 1, ptr_myfile);
    }

    fclose(ptr_myfile);

    if (ADL_OK != ADL2_Adapter_SupportedConnections_Get(g_adlContext, iAdapterIndex, devicePort, &supportedConnections))
    {
        MessageBox(NULL, "ADL_Adapter_SupportedConnections_Get failed!\n", "ADL Error", MB_ICONERROR | MB_OK);
        return false;
    }

    connectionData.aConnectionProperties.iValidProperties = 0;
    connectionData.aConnectionProperties.iBitrate = 0;
    connectionData.aConnectionProperties.iColorDepth = 0;
    connectionData.aConnectionProperties.iNumberOfLanes = 0;
    connectionData.aConnectionProperties.iOutputBandwidth = 0;
    connectionData.aConnectionProperties.iStereo3DCaps = 0;
    if ((supportedConnections.iSupportedConnections & (1 << iConnectionType)) == (1 << iConnectionType))
    {
        connectionData.iConnectionType = iConnectionType;

        if ((supportedConnections.iSupportedProperties[iConnectionType] & ADL_CONNECTION_PROPERTY_BITRATE) == ADL_CONNECTION_PROPERTY_BITRATE)
        {
            connectionData.aConnectionProperties.iBitrate = ADL_LINK_BITRATE_DEF;
            connectionData.aConnectionProperties.iValidProperties |= ADL_CONNECTION_PROPERTY_BITRATE;
        }
        if ((supportedConnections.iSupportedProperties[iConnectionType] & ADL_CONNECTION_PROPERTY_COLORDEPTH) == ADL_CONNECTION_PROPERTY_COLORDEPTH)
        {
            connectionData.aConnectionProperties.iColorDepth = ADL_COLOR_DEPTH_DEF;
            connectionData.aConnectionProperties.iValidProperties |= ADL_CONNECTION_PROPERTY_COLORDEPTH;
        }
        if ((supportedConnections.iSupportedProperties[iConnectionType] & ADL_CONNECTION_PROPERTY_NUMBER_OF_LANES) == ADL_CONNECTION_PROPERTY_NUMBER_OF_LANES)
        {
            connectionData.aConnectionProperties.iNumberOfLanes = ADL_LANECOUNT_DEF;
            connectionData.aConnectionProperties.iValidProperties |= ADL_CONNECTION_PROPERTY_NUMBER_OF_LANES;
        }
        if ((supportedConnections.iSupportedProperties[iConnectionType] & ADL_CONNECTION_PROPERTY_OUTPUT_BANDWIDTH) == ADL_CONNECTION_PROPERTY_OUTPUT_BANDWIDTH)
        {
            connectionData.aConnectionProperties.iOutputBandwidth = ADL_LINK_BITRATE_DEF;
            connectionData.aConnectionProperties.iValidProperties |= ADL_CONNECTION_PROPERTY_OUTPUT_BANDWIDTH;
        }
        if ((supportedConnections.iSupportedProperties[iConnectionType] & ADL_CONNECTION_PROPERTY_3DCAPS) == ADL_CONNECTION_PROPERTY_3DCAPS)
        {
            connectionData.aConnectionProperties.iStereo3DCaps = 0;
            connectionData.aConnectionProperties.iValidProperties |= ADL_CONNECTION_PROPERTY_3DCAPS;
        }

        if (ADL_OK != ADL2_Adapter_ConnectionData_Set(g_adlContext, iAdapterIndex, devicePort, connectionData))
        {
            MessageBox(NULL, "Set Connection Data failed!\n", "ADL Error", MB_ICONERROR | MB_OK);
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool removeConnectionData(int iAdapterIndex, const ADLDevicePort& devicePort, int iPreviousEmulationMode)
{
    bool ret = true;

    // Delete custom display mode if we added one
    if (g_bAddedCustomResolution)
    {
        if (ADL_OK != ADL2_Display_ModeTimingOverride_Delete(g_adlContext, iAdapterIndex, g_customDisplayID, &g_customDisplayMode, 1))
        {
            MessageBox(NULL, "ADL_Display_ModeTimingOverride_Delete failed!", "ADL Error", MB_ICONERROR | MB_OK);
            ret = false;
        }
    }

    if (ADL_OK != ADL2_Adapter_EmulationMode_Set(g_adlContext, iAdapterIndex, devicePort, iPreviousEmulationMode))
    {
        MessageBox(NULL, "ADL_Adapter_EmulationMode_Set failed!", "ADL Error", MB_ICONERROR | MB_OK);
        ret = false;
    }

    if (ADL_OK != ADL2_Adapter_ConnectionData_Remove(g_adlContext, iAdapterIndex, devicePort))
    {
        MessageBox(NULL, "ADL_Adapter_ConnectionData_Remove failed!", "ADL Error", MB_ICONERROR | MB_OK);
        ret = false;
    }

    return ret;
}