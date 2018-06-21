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

#include "RFMouseGrab.h"

#include <sstream>
#include <stdexcept>

#include "DoppDrv.h"

#include "RFError.h"

#define ONE_SECOND 1000

RFMouseGrab::RFMouseGrab(DOPPDrvInterface* pDrv, unsigned int uiDisplayId)
    : m_pDrv(pDrv)
    , m_bRunning(false)
    , m_bShapeUpdated(false)
    , m_bVisibilityUpdated(true)
    , m_hNewCursorStateEvent(NULL)
    , m_uiDisplayId(uiDisplayId)
    , m_hCursorEventsThread(NULL)
    , m_dwThreadId(0)
    , m_iVisible(1)
{
    HMODULE libUser32 = LoadLibraryA("user32.dll");
    if (!libUser32)
    {
        throw std::runtime_error("Failed to load module user32.dll");
    }

    m_fnGetCursorFrameInfo = reinterpret_cast<GET_CURSOR_FRAME_INFO>(GetProcAddress(libUser32, "GetCursorFrameInfo"));
    if (!m_fnGetCursorFrameInfo)
    {
        throw std::runtime_error("Failed to get function address for GetCursorFrameInfo");
    }

    m_animatedCursorInfo.hAnimatedCursor = NULL;
    m_animatedCursorInfo.dwFrameIndex = 0;
    m_animatedCursorInfo.dwUpdateCounter = 0;

    m_hcFallBackCursor = LoadCursor(NULL, IDC_ARROW);
    if (!m_hcFallBackCursor)
    {
        throw std::runtime_error("Failed to load IDC_ARROW cursor");
    }

    m_renderedMouseData.mouseData.iVisible = 0;
    m_renderedMouseData.mouseData.mask.pPixels = nullptr;
    m_renderedMouseData.mouseData.color.pPixels = nullptr;
    m_changedMouseData = m_renderedMouseData;

    memset(&m_renderedMouseData.maskBuffer, 0, sizeof(BitmapBuffer));
    memset(&m_renderedMouseData.colorBuffer, 0, sizeof(BitmapBuffer));
    memset(&m_changedMouseData.maskBuffer, 0, sizeof(BitmapBuffer));
    memset(&m_changedMouseData.colorBuffer, 0, sizeof(BitmapBuffer));

    memset(m_hShapeChangedEvents, NULL, sizeof(HANDLE) * MAX_CURSOR_SHAPECHANGE_TYPES);

    if (!createEvents())
    {
        throw std::runtime_error("Failed to craete events");
    }

    if (!createThreads())
    {
        throw std::runtime_error("Failed to start threads");
    }

    RFReadWriteAccess mutex(&m_MouseDataMutex);
    updateMouseShapeData(false, false);
}


RFMouseGrab::~RFMouseGrab()
{
    // Terminate threads.
    m_bRunning = false;

    // Signal event to unblock waiting threads.
    SetEvent(m_hShapeChangedEvents[CURSOR_SHAPE_CHANGED]);

    if (WaitForSingleObject(m_hCursorEventsThread, ONE_SECOND) == WAIT_TIMEOUT)
    {
        // Should never enter this path.
        DWORD dwExit;

        GetExitCodeThread(m_hCursorEventsThread, &dwExit);
        TerminateThread(m_hCursorEventsThread, dwExit);
    }

    CloseHandle(m_hNewCursorStateEvent);
    m_hNewCursorStateEvent = NULL;

    if (m_hCursorEventsThread)
    {
        CloseHandle(m_hCursorEventsThread);
        m_hCursorEventsThread = NULL;
    }

    m_renderedMouseData.mouseData.iVisible = 0;
    m_changedMouseData.mouseData.iVisible = 0;

    if (m_renderedMouseData.colorBuffer.pBuffer)
    {
        delete[] m_renderedMouseData.colorBuffer.pBuffer;
        m_renderedMouseData.colorBuffer.pBuffer = nullptr;
        m_renderedMouseData.colorBuffer.uiBufferSize = 0;
    }

    if (m_renderedMouseData.maskBuffer.pBuffer)
    {
        delete[] m_renderedMouseData.maskBuffer.pBuffer;
        m_renderedMouseData.maskBuffer.pBuffer = nullptr;
        m_renderedMouseData.maskBuffer.uiBufferSize = 0;
    }

    if (m_changedMouseData.colorBuffer.pBuffer)
    {
        delete[] m_changedMouseData.colorBuffer.pBuffer;
        m_changedMouseData.colorBuffer.pBuffer = nullptr;
        m_changedMouseData.colorBuffer.uiBufferSize = 0;
    }

    if (m_changedMouseData.maskBuffer.pBuffer)
    {
        delete[] m_changedMouseData.maskBuffer.pBuffer;
        m_changedMouseData.maskBuffer.pBuffer = nullptr;
        m_changedMouseData.maskBuffer.uiBufferSize = 0;
    }
}


bool RFMouseGrab::getShapeData(int iBlocking, RFMouseData& md)
{
    bool bNewData = false;

    if (iBlocking)
    {
        ResetEvent(m_hNewCursorStateEvent);

        WaitForSingleObject(m_hNewCursorStateEvent, INFINITE);
    }

    {
        RFReadWriteAccess mutex(&m_MouseDataMutex);

        bNewData = m_bShapeUpdated | m_bVisibilityUpdated;

        m_changedMouseData.mouseData.iVisible = m_iVisible;

        if (m_iVisible && m_changedMouseData.mouseData.mask.pPixels)
        {
            md = m_changedMouseData.mouseData;
        }
        else if (!m_iVisible && m_changedMouseData.mouseData.mask.pPixels)
        {
            memset(m_changedMouseData.mouseData.mask.pPixels, 0, m_changedMouseData.mouseData.mask.uiPitch * m_changedMouseData.mouseData.mask.uiHeight);

            if (m_changedMouseData.mouseData.color.pPixels)
            {
                memset(m_changedMouseData.mouseData.color.pPixels, 0, m_changedMouseData.mouseData.color.uiPitch * m_changedMouseData.mouseData.color.uiHeight);
            }

            md = m_changedMouseData.mouseData;
        }
        else
        {
            memset(&md, 0, sizeof(RFMouseData));
        }

        if (m_bShapeUpdated)
        {
            std::swap(m_renderedMouseData, m_changedMouseData);
        }

        m_bShapeUpdated = false;
        m_bVisibilityUpdated = false;
    }

    // Only return true if the mouse shape changed.
    return bNewData;
}


int HasAlphaValues(RFBitmapBuffer* pColor)
{
    if (!pColor || !pColor->pPixels || pColor->uiBitsPerPixel != 32)
    {
        return 0;
    }

    uint8_t* pColorPixels = static_cast<uint8_t*>(pColor->pPixels);

    for (unsigned int y = 0; y < pColor->uiHeight; ++y)
    {
        for (unsigned int x = 0; x < pColor->uiWidth; ++x)
        {
            if (pColorPixels[y * pColor->uiPitch + x * 4 + 3] != 0)
            {
                return 1;
            }
        }
    }

    return 0;
}


int HasMaskValues(RFBitmapBuffer* pMask)
{
    if (!pMask || !pMask->pPixels || pMask->uiBitsPerPixel != 1)
    {
        return 0;
    }

    uint8_t* pMaskPixels = static_cast<uint8_t*>(pMask->pPixels);
    if (pMaskPixels[0] != 0xff && pMaskPixels[0] != 0)
    {
        return 1;
    }

    for (unsigned int y = 0; y < pMask->uiHeight; ++y)
    {
        for (unsigned int x = 0; x < pMask->uiPitch; ++x)
        {
            if (pMaskPixels[y * pMask->uiPitch + x] != pMaskPixels[0])
            {
                return 1;
            }
        }
    }

    return 0;
}


void MaskAlphaAndPremultiplyColors(RFBitmapBuffer* pMask, RFBitmapBuffer* pColor)
{
    if (!pMask || !pColor || !pMask->pPixels || !pColor->pPixels || pMask->uiBitsPerPixel != 1 || pColor->uiBitsPerPixel != 32)
    {
        return;
    }

    int hasMaskValues = HasMaskValues(pMask);
    uint8_t* pColorPixels = static_cast<uint8_t*>(pColor->pPixels);
    uint8_t* pMaskPixels = static_cast<uint8_t*>(pMask->pPixels);

    for (unsigned int y = 0; y < pColor->uiHeight; ++y)
    {
        for (unsigned int x = 0; x < pColor->uiWidth; ++x)
        {
            uint8_t* pColorPixel = &pColorPixels[y * pColor->uiPitch + x * 4];

            if (hasMaskValues && (pMaskPixels[y * pMask->uiPitch + x / 8] >> (7 - (x % 8))) & 1)
            {
                pColorPixel[3] = 0;
            }
            else
            {
                uint8_t alpha  = pColorPixel[3];
                pColorPixel[0] = (pColorPixel[0] * alpha) / 255;
                pColorPixel[1] = (pColorPixel[1] * alpha) / 255;
                pColorPixel[2] = (pColorPixel[2] * alpha) / 255;
            }
        }
    }
}


void ConvertColorToMonochrome(RFMouseData* mouseData)
{
    if (!mouseData || !mouseData->color.pPixels || !mouseData->mask.pPixels || mouseData->mask.uiBitsPerPixel != 1 || mouseData->color.uiBitsPerPixel != 32)
    {
        return;
    }

    uint8_t* pColorPixels = static_cast<uint8_t*>(mouseData->color.pPixels);

    for (unsigned int y = 0; y < mouseData->color.uiHeight; ++y)
    {
        for (unsigned int x = 0; x < mouseData->color.uiWidth; ++x)
        {
            if (*(reinterpret_cast<uint32_t*>(&pColorPixels[y * mouseData->color.uiPitch + x * 4])) != 0)
            {
                pColorPixels[y * mouseData->mask.uiPitch + x / 8] |= 1 << (7 - (x % 8));
            }
        }
    }

    unsigned int maskBuffersSize = mouseData->mask.uiHeight * mouseData->mask.uiPitch;
    memcpy(pColorPixels + maskBuffersSize, pColorPixels, maskBuffersSize);
    memcpy(pColorPixels, mouseData->mask.pPixels, maskBuffersSize);

    mouseData->mask.pPixels = mouseData->color.pPixels;
    mouseData->mask.uiHeight *= 2;
    memset(&mouseData->color, 0, sizeof(mouseData->color));
}

void ConvertRFMouseData(RFMouseData* input, RFMouseData2* output)
{
    if (!input || !input->mask.pPixels || !output)
    {
        return;
    }

    output->iVisible = input->iVisible;
    output->uiXHot = input->uiXHot;
    output->uiYHot = input->uiYHot;

    if (input->color.pPixels && HasAlphaValues(&input->color))
    {
        output->pShape.uiWidth = input->color.uiWidth;
        output->pShape.uiHeight = input->color.uiHeight;
        output->pShape.uiPitch = input->color.uiPitch;
        output->pShape.uiBitsPerPixel = input->color.uiBitsPerPixel;
        MaskAlphaAndPremultiplyColors(&input->mask, &input->color);
        output->pShape.pPixels = input->color.pPixels;
        output->uiFlags = 0x00000004; // RF_MOUSE_MASKED_COLOR
        return;
    }

    if (input->color.pPixels)
    {
        ConvertColorToMonochrome(input);
    }

    output->pShape.uiWidth = input->mask.uiWidth;
    output->pShape.uiHeight = input->mask.uiHeight;
    output->pShape.uiPitch = input->mask.uiPitch;
    output->pShape.uiBitsPerPixel = input->mask.uiBitsPerPixel;
    output->pShape.pPixels = input->mask.pPixels;
    output->uiFlags = 0x00000001; // RF_MOUSE_MONOCHROME
    return;
}

bool RFMouseGrab::getShapeData2(int iBlocking, RFMouseData2& md)
{
    RFMouseData md1 = {};
    if (!getShapeData(iBlocking, md1))
    {
        return false;
    }

    memset(&md, 0, sizeof(md));
    ConvertRFMouseData(&md1, &md);

    return true;
}

bool RFMouseGrab::releaseEvent()
{
    if (m_hNewCursorStateEvent)
    {
        SetEvent(m_hNewCursorStateEvent);
        Sleep(0);

        return true;
    }

    return false;
}


bool RFMouseGrab::createEvents()
{
    if (!m_pDrv)
    {
        return false;
    }

    // Create user event that gets signaled if the mouse shape changes.
    m_hShapeChangedEvents[CURSOR_SHAPE_CHANGED] = m_pDrv->createDOPPEvent(DOPPEventType::DOPP_MOUSE_EVENT);
    if (!m_hShapeChangedEvents[CURSOR_SHAPE_CHANGED])
    {
        return false;
    }

    m_hShapeChangedEvents[CURSOR_SHAPE_SHOW] = m_pDrv->createDOPPEvent(DOPPEventType::DOPP_CURSORSHOW_EVENT);
    if (!m_hShapeChangedEvents[CURSOR_SHAPE_SHOW])
    {
        return false;
    }

    m_hShapeChangedEvents[CURSOR_SHAPE_HIDE] = m_pDrv->createDOPPEvent(DOPPEventType::DOPP_CURSORHIDE_EVENT);
    if (!m_hShapeChangedEvents[CURSOR_SHAPE_HIDE])
    {
        return false;
    }

    // Create event to signal getShapeData it is called with blocking option.
    m_hNewCursorStateEvent = CreateEvent(NULL, TRUE, FALSE, "NewCursorStateEvent");

    if (!m_hNewCursorStateEvent)
    {
        return false;
    }

    return true;
}


bool RFMouseGrab::createThreads()
{
    m_bRunning = true;

    m_hCursorEventsThread = CreateThread(NULL, NULL, ShapeNotificationThread, this, 0, &m_dwThreadId);

    if (!m_hCursorEventsThread)
    {
        return false;
    }

    return true;
}


DWORD WINAPI RFMouseGrab::ShapeNotificationThread(void* pThreadData)
{
    RFMouseGrab* pMouse = reinterpret_cast<RFMouseGrab*>(pThreadData);

    if (pMouse)
    {
        pMouse->updateLoop();
    }

    return 0;
}

void RFMouseGrab::updateLoop()
{
    while (m_bRunning)
    {
        DWORD ret = WaitForMultipleObjects(MAX_CURSOR_SHAPECHANGE_TYPES, m_hShapeChangedEvents, FALSE, INFINITE) - WAIT_OBJECT_0;

        if (ret == CURSOR_SHAPE_CHANGED)
        {
            RFReadWriteAccess mutex(&m_MouseDataMutex);
            updateMouseShapeData(true, false);
            m_iVisible = m_pDrv->getCursorVisibility();
        }
        else if (ret == CURSOR_SHAPE_SHOW)
        {
            RFReadWriteAccess mutex(&m_MouseDataMutex);

            m_iVisible = m_pDrv->getCursorVisibility();
            m_bVisibilityUpdated = true;

            updateMouseShapeData(false, false);
        }
        else if (ret == CURSOR_SHAPE_HIDE)
        {
            RFReadWriteAccess mutex(&m_MouseDataMutex);

            m_iVisible = m_pDrv->getCursorVisibility();
            m_bVisibilityUpdated = true;

            updateMouseShapeData(false, false);
        }

        SetEvent(m_hNewCursorStateEvent);
        Sleep(0);
    }
}


bool RFMouseGrab::copyBitmapToBuffer(const HBITMAP hBitmap, BitmapBuffer& buffer)
{
    if (GetObject(hBitmap, sizeof(BITMAP), &buffer.BitMap) > 0)
    {
        unsigned int uiNewSize = buffer.BitMap.bmWidthBytes * buffer.BitMap.bmHeight;

        if (buffer.pBuffer == nullptr)
        {
            buffer.pBuffer = new char[uiNewSize];

            buffer.uiBufferSize = uiNewSize;
        }
        else if (uiNewSize != buffer.uiBufferSize)
        {
            delete[] buffer.pBuffer;

            buffer.pBuffer = new char[uiNewSize];

            buffer.uiBufferSize = uiNewSize;
        }

        unsigned int uiRealSize = GetBitmapBits(hBitmap, uiNewSize, buffer.pBuffer);

        if (uiRealSize == buffer.uiBufferSize)
        {
            return true;
        }
    }

    return false;
}


void RFMouseGrab::updateMouseShapeData(bool bIncrementAnimationIndex, bool bGetMouseVisibility)
{
    CURSORINFO cursorInfo;
    ICONINFO   iconInfo;

    memset(&cursorInfo, 0, sizeof(CURSORINFO));
    memset(&iconInfo, 0, sizeof(ICONINFO));

    cursorInfo.cbSize = sizeof(CURSORINFO);

    // If GetCursorInfo fails because of ERROR_ACCESS_DENIED use the standard IDC_ARROW cursor.
    if (!GetCursorInfo(&cursorInfo))
    {
        if (GetLastError() != ERROR_ACCESS_DENIED)
        {
            return;
        }

        cursorInfo.flags = CURSOR_SHOWING;
        cursorInfo.hCursor = m_hcFallBackCursor;
    }

    if (bGetMouseVisibility)
    {
        int iVisible = (cursorInfo.flags == CURSOR_SHOWING);
        if (m_iVisible != iVisible)
        {
            m_bVisibilityUpdated = true;
        }
        m_iVisible = iVisible;
    }

    m_animatedCursorInfo.hAnimatedCursor = m_fnGetCursorFrameInfo(cursorInfo.hCursor, L"", m_animatedCursorInfo.dwFrameIndex, &m_animatedCursorInfo.dwDisplayRate, &m_animatedCursorInfo.dwTotalFrames);
    if (m_animatedCursorInfo.dwTotalFrames == 1)
    {
        m_animatedCursorInfo.dwFrameIndex = 0;
    }
    else
    {
        cursorInfo.hCursor = m_animatedCursorInfo.hAnimatedCursor;

        if (bIncrementAnimationIndex)
        {
            ++m_animatedCursorInfo.dwFrameIndex;
            if (m_animatedCursorInfo.dwFrameIndex >= m_animatedCursorInfo.dwTotalFrames)
            {
                m_animatedCursorInfo.dwFrameIndex = 0;
            }
        }
    }

    if (GetIconInfo(cursorInfo.hCursor, &iconInfo))
    {
        m_changedMouseData.mouseData.uiXHot = iconInfo.xHotspot;
        m_changedMouseData.mouseData.uiYHot = iconInfo.yHotspot;

        if (copyBitmapToBuffer(iconInfo.hbmMask, m_changedMouseData.maskBuffer))
        {
            StoreBitmapBuffer(m_changedMouseData.maskBuffer, m_changedMouseData.mouseData.mask);
        }

        m_changedMouseData.mouseData.color.pPixels = nullptr;
        if (iconInfo.hbmColor)
        {
            if (copyBitmapToBuffer(iconInfo.hbmColor, m_changedMouseData.colorBuffer))
            {
                StoreBitmapBuffer(m_changedMouseData.colorBuffer, m_changedMouseData.mouseData.color);
            }
        }

        DeleteObject(iconInfo.hbmColor);
        DeleteObject(iconInfo.hbmMask);

        // Indicate that we have new shape data.
        m_bShapeUpdated = true;
    }
}


void RFMouseGrab::StoreBitmapBuffer(const BitmapBuffer& src, RFBitmapBuffer& dest)
{
    dest.uiWidth = src.BitMap.bmWidth;
    dest.uiHeight = src.BitMap.bmHeight;
    dest.uiPitch = src.BitMap.bmWidthBytes;
    dest.uiBitsPerPixel = src.BitMap.bmBitsPixel;
    dest.pPixels = src.pBuffer;
}