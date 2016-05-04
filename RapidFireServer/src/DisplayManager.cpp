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

#include "DisplayManager.h"

#include <tchar.h>

#include <string>

#include <windows.h>

#include "ADLWrapper.h"


struct DisplayData
{
    bool                bPrimary;
    unsigned int        uiGPUId;
    unsigned int        uiBusNumber;
    unsigned int        uiDisplayId;
    unsigned int        uiDesktopId;
    unsigned int        uiDisplayLogicalId;
    unsigned int        uiWindowsDisplayId;
    int                 nOriginX;
    int                 nOriginY;
    unsigned int        uiWidth;
    unsigned int        uiHeight;
    DWORD               dwOrientation;
    std::string         strDisplayname;
    std::string         strMonitorName;
};


DisplayManager::DisplayManager()
    : m_uiNumGPU(0)
    , m_adlCalls(ADLWrapper::getInstance())
{
    m_Displays.clear();
}


DisplayManager::~DisplayManager()
{
    deleteDisplays();
}


unsigned int DisplayManager::enumDisplays()
{
    int				nNumDisplays       = 0;
    int				nNumAdapters       = 0;
    int             nCurrentBusNumber  = 0;
    int             nCurrentAdapter    = -1;
    unsigned int    uiCurrentDesktopId = 0;
    unsigned int    uiCurrentGPUId     = 0;
    unsigned int    uiCurrentDisplayId = 0;

    // check if ADL was loaded.
    if (!m_adlCalls)
    {
        return 0;
    }

    if (m_Displays.size() > 0)
    {
        deleteDisplays();
    }

    // Make sure we get the latest topology. If a session is deleted and re-created the topology might
    // have changed but ADL2_Main_ControlX2_Create won't get called since ADLWrapper is a singelton.
    m_adlCalls.ADL2_Main_Control_Refresh(m_adlCalls.getHandle());

    // Determine how many adapters and displays are in the system.
    m_adlCalls.ADL2_Adapter_NumberOfAdapters_Get(m_adlCalls.getHandle(), &nNumAdapters);

    if (nNumAdapters <= 0)
    {
        return false;
    }

    int nPrimary = -1;

    m_adlCalls.ADL2_Adapter_Primary_Get(m_adlCalls.getHandle(), &nPrimary);

    std::vector<AdapterInfo> adlAdapterInfo(nNumAdapters);

    m_adlCalls.ADL2_Adapter_AdapterInfo_Get(m_adlCalls.getHandle(), &(adlAdapterInfo[0]), sizeof(AdapterInfo) * nNumAdapters);

    // Loop through all adapters.
    for (int i = 0; i < nNumAdapters; ++i)
    {
        int nAdapterIdx = adlAdapterInfo[i].iAdapterIndex;

        int nAdapterStatus;
        m_adlCalls.ADL2_Adapter_Active_Get(m_adlCalls.getHandle(), nAdapterIdx, &nAdapterStatus);

        if (nAdapterStatus)
        {
            LPADLDisplayInfo pDisplayInfo = NULL;

            // ADL allocates memory to store DisplayInfo but we need to free it later.
            m_adlCalls.ADL2_Display_DisplayInfo_Get(m_adlCalls.getHandle(), nAdapterIdx, &nNumDisplays, &pDisplayInfo, 0);

            for (int j = 0; j < nNumDisplays; ++j)
            {
                // Check if the display is connected.
                if (pDisplayInfo[j].iDisplayInfoValue & ADL_DISPLAY_DISPLAYINFO_DISPLAYCONNECTED)
                {
                    // Check if the display is mapped on adapter.
                    if (pDisplayInfo[j].iDisplayInfoValue & ADL_DISPLAY_DISPLAYINFO_DISPLAYMAPPED && pDisplayInfo[j].displayID.iDisplayLogicalAdapterIndex == nAdapterIdx)
                    {
                        if (nCurrentAdapter != nAdapterIdx)
                        {
                            // NEW Desktop: If we see independant displays and each display represents a desktop
                            // then each display has its own adapter id
                            // If the displays belong to a group that builds a single desktop, they will have the same
                            // Adapter id but different display ids.
                            //
                            // The list of displays is the same for all adapters. Hence in the first iteration all displays belonging
                            // to an adapter are found.
                            ++uiCurrentDesktopId;

                            nCurrentAdapter = nAdapterIdx;
                        }

                        if (nCurrentBusNumber == 0)
                        {
                            // Found the first GPU in the system.
                            ++m_uiNumGPU;
                            nCurrentBusNumber = adlAdapterInfo[nAdapterIdx].iBusNumber;
                        }
                        else if (nCurrentBusNumber != adlAdapterInfo[nAdapterIdx].iBusNumber)
                        {
                            // Found a new GPU.
                            ++m_uiNumGPU;
                            ++uiCurrentGPUId;
                            nCurrentBusNumber = adlAdapterInfo[nAdapterIdx].iBusNumber;
                        }

                        // Found mapped display, store relevant information.
                        DisplayData* pDsp = new DisplayData;

                        pDsp->bPrimary           = (nPrimary == nAdapterIdx);
                        pDsp->uiGPUId            = uiCurrentGPUId;
                        pDsp->uiBusNumber        = static_cast<unsigned int>(nCurrentBusNumber);
                        pDsp->uiDisplayId        = uiCurrentDisplayId;
                        pDsp->uiDesktopId        = uiCurrentDesktopId;
                        pDsp->uiDisplayLogicalId = pDisplayInfo[j].displayID.iDisplayLogicalIndex;
                        pDsp->uiWindowsDisplayId = 0;
                        pDsp->strDisplayname     = std::string(adlAdapterInfo[nAdapterIdx].strDisplayName);
                        pDsp->strMonitorName     = std::string(pDisplayInfo[j].strDisplayName);
                        pDsp->nOriginX           = 0;
                        pDsp->nOriginY           = 0;
                        pDsp->uiWidth            = 0;
                        pDsp->uiHeight           = 0;
                        pDsp->dwOrientation      = 0;

                        DEVMODEA DevMode = {};
                        EnumDisplaySettingsA(pDsp->strDisplayname.c_str(), ENUM_CURRENT_SETTINGS, &DevMode);

                        pDsp->nOriginX = DevMode.dmPosition.x;
                        pDsp->nOriginY = DevMode.dmPosition.y;
                        pDsp->uiWidth  = DevMode.dmPelsWidth;
                        pDsp->uiHeight = DevMode.dmPelsHeight;

                        if ((DevMode.dmFields & DM_DISPLAYORIENTATION) == DM_DISPLAYORIENTATION)
                        {
                            pDsp->dwOrientation = DevMode.dmDisplayOrientation;
                        }

                        // Get the windows display id from the display name.
                        std::string strDisplName(pDsp->strDisplayname);
                        std::string strKey("DISPLAY");

                        size_t pos = strDisplName.find(strKey);

                        if (pos != std::string::npos && (pos + strKey.length()) < strDisplName.length())
                        {
                            pos += strKey.length();

                            pDsp->uiWindowsDisplayId = atoi(&strDisplName[pos]);
                        }

                        m_Displays.push_back(pDsp);
                        ++uiCurrentDisplayId;
                    }
                }
            }

            if (nNumDisplays && pDisplayInfo)
            {
                free(pDisplayInfo);
            }
        }
    }

    return static_cast<unsigned int>(m_Displays.size());
}


void DisplayManager::deleteDisplays()
{
    for (const auto& d : m_Displays)
    {
        if (d)
        {
            delete (d);
        }
    }

    m_Displays.clear();
    m_uiNumGPU = 0;
}


unsigned int DisplayManager::getNumGPUs() const
{
    return m_uiNumGPU;
}


unsigned int DisplayManager::getNumDisplays() const
{
    return static_cast<unsigned int>(m_Displays.size());
}


unsigned int DisplayManager::getNumDisplaysOnGPU(unsigned int uiGPU) const
{
    unsigned int uiNumDsp = 0;

    // Loop through all displays and check if they are on the requested GPU.
    for (const auto& d : m_Displays)
    {
        if (d->uiGPUId == uiGPU)
        {
            ++uiNumDsp;
        }
    }

    return uiNumDsp;
}


unsigned int DisplayManager::getDisplayOnGPU(unsigned int uiGPU, unsigned int n) const
{
    unsigned int uiCurrentDisplayOnGpu = 0;

    // Loop through all displays and return the n-th display on the GPU uiGPU.
    for (const auto& d : m_Displays)
    {
        if (d->uiGPUId == uiGPU)
        {
            if (uiCurrentDisplayOnGpu == n)
            {
                return d->uiDisplayId;
            }

            ++uiCurrentDisplayOnGpu;
        }
    }

    return 0;
}


bool DisplayManager::getDisplayIdFromWinID(unsigned int uiWindowsDisplayId, unsigned int& uiDspId) const
{
    // Loop through all displays and return the desktop id of the first display
    // belonging to this desktop. The ID is the same for all displays, thus taking the first is ok.
    for (const auto& d : m_Displays)
    {
        if (d->uiWindowsDisplayId == uiWindowsDisplayId)
        {
            uiDspId = d->uiDisplayId;

            return true;
        }
    }

    return false;
}


bool DisplayManager::getDisplayIdFromCCCID(unsigned int uiCCCDspIdId, unsigned int& uiDspId) const
{
    // Loop through all displays and return the desktop id of the first display
    // belonging to this desktop. The ID is the same for all displays, thus taking the first is ok.
    for (const auto& d : m_Displays)
    {
        if (d->uiDesktopId == uiCCCDspIdId)
        {
            uiDspId = d->uiDisplayId;

            return true;
        }
    }

    return false;
}


bool DisplayManager::checkInternalDisplayID(unsigned int uiInternalDspId) const
{
    for (const auto& d : m_Displays)
    {
        if (d->uiDisplayId == uiInternalDspId)
        {
            return true;
        }
    }

    return false;
}


unsigned int DisplayManager::getPrimaryDisplay() const
{
    for (const auto& d : m_Displays)
    {
        if (d->bPrimary)
        {
            return d->uiDisplayId;
        }
    }

    // We nned to have a primary display. We should never end up here.
    return 0xffffffff;
}


unsigned int DisplayManager::getWindowsDisplayId(unsigned int uiDspId) const
{
    if (uiDspId < m_Displays.size())
    {
        return m_Displays[uiDspId]->uiWindowsDisplayId;
    }

    // Windows Display IDs start with 1. 0 indicates an error.
    return 0;
}

unsigned int DisplayManager::getDesktopId(unsigned int uiDspId) const
{
    if (uiDspId < m_Displays.size())
    {
        return m_Displays[uiDspId]->uiDesktopId;
    }

    // Desktop IDs start with 1. 0 indicates an error.
    return 0;
}


unsigned int DisplayManager::getGpuId(unsigned int uiDisplay) const
{
    if (uiDisplay < m_Displays.size())
    {
        return m_Displays[uiDisplay]->uiGPUId;
    }

    return 0;
}


unsigned int DisplayManager::getBusNumber(unsigned int uiDisplayId) const
{
    if (uiDisplayId < m_Displays.size())
    {
        return m_Displays[uiDisplayId]->uiBusNumber;;
    }

    return 0;
}


std::string DisplayManager::getDisplayName(unsigned int uiDisplayId) const
{
    if (uiDisplayId < m_Displays.size())
    {
        return m_Displays[uiDisplayId]->strDisplayname;;
    }

    return std::string();
}


std::string DisplayManager::getMonitorName(unsigned int uiDisplayId) const
{
    if (uiDisplayId < m_Displays.size())
    {
        return m_Displays[uiDisplayId]->strMonitorName;;
    }

    return std::string();
}



int DisplayManager::getOriginX(unsigned int uiDspId) const
{
    if (uiDspId < m_Displays.size())
    {
        return m_Displays[uiDspId]->nOriginX;
    }

    return 0;
}


int DisplayManager::getOriginY(unsigned int uiDspId) const
{
    if (uiDspId < m_Displays.size())
    {
        return m_Displays[uiDspId]->nOriginY;
    }

    return 0;
}

unsigned int DisplayManager::getWidth(unsigned int uiDspId) const
{
    if (uiDspId < m_Displays.size())
    {
        return m_Displays[uiDspId]->uiWidth;
    }

    return 0;
}


unsigned int DisplayManager::getHeight(unsigned int uiDspId) const
{
    if (uiDspId < m_Displays.size())
    {
        return m_Displays[uiDspId]->uiHeight;
    }

    return 0;
}

float DisplayManager::getDesktopRotation(unsigned int uiDspId) const
{
    if (uiDspId < m_Displays.size())
    {
        return m_Displays[uiDspId]->dwOrientation * 90.0f;
    }

    return 0.0f;
}