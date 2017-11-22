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

#include "RFGLDOPPCapture.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <GL/glew.h>

#include "RFError.h"
#include "RFGLShader.h"
#include "RFLock.h"

#define GL_WAIT_FOR_PREVIOUS_VSYNC 0x931C

typedef GLuint(APIENTRY* PFNWGLGETDESKTOPTEXTUREAMD)(void);
typedef void   (APIENTRY* PFNWGLENABLEPOSTPROCESSAMD)(bool enable);
typedef GLuint(APIENTRY* WGLGENPRESENTTEXTUREAMD)(void);
typedef GLboolean(APIENTRY* WGLDESKTOPTARGETAMD)(GLuint);
typedef GLuint(APIENTRY* PFNWGLPRESENTTEXTURETOVIDEOAMD)(GLuint presentTexture, const GLuint* attrib_list);

PFNWGLGETDESKTOPTEXTUREAMD      wglGetDesktopTextureAMD;
PFNWGLENABLEPOSTPROCESSAMD      wglEnablePostProcessAMD;
PFNWGLPRESENTTEXTURETOVIDEOAMD  wglPresentTextureToVideoAMD;
WGLDESKTOPTARGETAMD             wglDesktopTargetAMD;
WGLGENPRESENTTEXTUREAMD         wglGenPresentTextureAMD;

#define GET_PROC(xx)                                        \
    {                                                       \
        void **x = reinterpret_cast<void**>(&xx);           \
        *x = static_cast<void*>(wglGetProcAddress(#xx));    \
        if (*x == nullptr) {                                \
            return false;                                   \
        }                                                   \
    }

// Global lock that is used to make sure the GL operations after wglDesktopTarget don't get interrupted.
static RFLock g_GlobalDOPPLock;

GLDOPPCapture::GLDOPPCapture(unsigned int uiDesktop, DOPPDrvInterface* pDrv)
    : m_uiDesktopTexture(0)
    , m_rfDesktopFormat(RF_FORMAT_UNKNOWN)
    , m_uiDesktopId(uiDesktop)
    , m_uiDesktopWidth(0)
    , m_uiDesktopHeight(0)
    , m_bTrackDesktopChanges(false)
    , m_bBlocking(false)
    , m_iNumRemainingFrames(3)
    , m_pDOPPDrvInterface(pDrv)
{
    if (!m_pDOPPDrvInterface)
    {
        throw std::runtime_error("DOPP no driver ineterface");
    }

    if (m_pDOPPDrvInterface->getDoppState() == false)
    {
        // Try to enable DOPP. If succeeded DOPP will be disabled when the DOPPDrvInterface instance
        // is deleted. No explicit disabling is required.
        m_pDOPPDrvInterface->enableDopp();

        if (!m_pDOPPDrvInterface->getDoppState())
        {
            throw std::runtime_error("DOPP not enabled");
        }
    }

    // Event 0 is signaled by DOPP, Event 1 is used to unblock notification loop.
    m_hDesktopEvent[0] = NULL;
    m_hDesktopEvent[1] = NULL;

    m_bDesktopChanged = false;
}


GLDOPPCapture::~GLDOPPCapture()
{
    HGLRC glrc = wglGetCurrentContext();

    if (glrc)
    {
        if (m_uiDesktopTexture)
        {
            glDeleteTextures(1, &m_uiDesktopTexture);
        }
    }
    else
    {
        RF_Error(RF_STATUS_OPENGL_FAIL, "No more valid context when deleting DOPP Capture");
    }

    m_bTrackDesktopChanges = false;

    if (m_hDesktopEvent[0])
    {
        SetEvent(m_hDesktopEvent[0]);
        Sleep(0);
    }

    if (m_hDesktopEvent[1])
    {
        CloseHandle(m_hDesktopEvent[1]);
        m_hDesktopEvent[1] = NULL;
    }

    if (m_NotificationThread.joinable())
    {
        m_NotificationThread.join();
    }

    if (m_hDesktopEvent[0])
    {
        m_pDOPPDrvInterface->deleteDOPPEvent(m_hDesktopEvent[0]);

        m_hDesktopEvent[0] = NULL;
    }
}


RFStatus GLDOPPCapture::initDOPP(bool bTrackDesktopChanges, bool bBlocking)
{
    RFReadWriteAccess doppLock(&g_GlobalDOPPLock);

    HGLRC glrc = wglGetCurrentContext();

    if (!glrc)
    {
        return RF_STATUS_OPENGL_FAIL;
    }

    if (!setupDOPPExtension())
    {
        return RF_STATUS_DOPP_FAIL;
    }

    // Select the Desktop to be processed. ID is the same as seen in CCC.
    if (!wglDesktopTargetAMD(m_uiDesktopId))
    {
        return RF_STATUS_INVALID_DESKTOP_ID;
    }

    m_uiDesktopTexture = wglGetDesktopTextureAMD();
    glBindTexture(GL_TEXTURE_2D, m_uiDesktopTexture);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, reinterpret_cast<GLint*>(&m_uiDesktopWidth));
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, reinterpret_cast<GLint*>(&m_uiDesktopHeight));

    glBindTexture(GL_TEXTURE_2D, 0);

    m_bTrackDesktopChanges = bTrackDesktopChanges;
    m_bBlocking = bBlocking;


    if (m_bBlocking && !m_bTrackDesktopChanges)
    {
        m_bTrackDesktopChanges = true;
    }

    if (m_bTrackDesktopChanges)
    {
        // Create the event to get notifications on Desktop changes.
        m_hDesktopEvent[0] = m_pDOPPDrvInterface->createDOPPEvent(DOPPEventType::DOPP_DESKOTOP_EVENT);

        if (!m_hDesktopEvent[0])
        {
            // If registration fails, indicate that no changes are tracked, desktop capturing is still functinal.
            m_bTrackDesktopChanges = false;
        }
        else
        {
            // Create the event that can be signaled to unblock processDesktop if the blocking option is set.
            // A separate event is used to differentiate between a notifaction which triggers the rendering
            // of the desktop texture and a release call which will only unblock but won't generate a new
            // desktop image.
            m_hDesktopEvent[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
        }
    }

    // If changes are tracked and processDesktop is non-blcking we need a notificatin thread.
    if (m_bTrackDesktopChanges && !m_bBlocking)
    {
        m_NotificationThread = std::thread(&GLDOPPCapture::notificationLoop, this);
    }

    return RF_STATUS_OK;
}


RFStatus GLDOPPCapture::resizeDesktopTexture()
{
    if (m_uiDesktopId > 0)
    {
        RFReadWriteAccess doppLock(&g_GlobalDOPPLock);

        if (m_uiDesktopTexture)
        {
            glDeleteTextures(1, &m_uiDesktopTexture);
        }

        // The resize might happen after a display topology change -> we could fail getting
        // a desktop texture for this m_uiDesktopId.
        if (!wglDesktopTargetAMD(m_uiDesktopId))
        {
            return RF_STATUS_INVALID_DESKTOP_ID;
        }

        m_uiDesktopTexture = wglGetDesktopTextureAMD();
        glBindTexture(GL_TEXTURE_2D, m_uiDesktopTexture);

        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, reinterpret_cast<GLint*>(&m_uiDesktopWidth));
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, reinterpret_cast<GLint*>(&m_uiDesktopHeight));

        glBindTexture(GL_TEXTURE_2D, 0);


        return RF_STATUS_OK;
    }

    return RF_STATUS_INVALID_DESKTOP_ID;
}


bool GLDOPPCapture::releaseEvent()
{
    if (m_bBlocking)
    {
        SetEvent(m_hDesktopEvent[1]);
        Sleep(0);

        return true;
    }

    return false;
}


bool GLDOPPCapture::processDesktop()
{
    if (m_bTrackDesktopChanges)
    {
        if (m_iNumRemainingFrames <= 0)
        {
            if (m_bBlocking)
            {
                DWORD dwResult = WaitForMultipleObjects(2, m_hDesktopEvent, FALSE, INFINITE);

                if ((dwResult - WAIT_OBJECT_0) == 1)
                {
                    // Thread was unblocked by internal event not by DOPP.
                    return false;
                }
            }
            else if (!m_bDesktopChanged.load())
            {
                return false;
            }

            m_iNumRemainingFrames = m_iNumContinuedFrames;
        }

        --m_iNumRemainingFrames;
    }

    m_bDesktopChanged = false;

    return true;
}


RFFormat GLDOPPCapture::getDesktopTextureFormat() const
{
    RFFormat format;
    bool ret = m_pDOPPDrvInterface->getPrimarySurfacePixelFormat(format);

    if (ret && format != RF_FORMAT_UNKNOWN)
    {
        m_rfDesktopFormat = format;
    }

    return m_rfDesktopFormat;
}

bool GLDOPPCapture::setupDOPPExtension()
{
    GET_PROC(wglGetDesktopTextureAMD);
    GET_PROC(wglEnablePostProcessAMD);
    GET_PROC(wglPresentTextureToVideoAMD);
    GET_PROC(wglDesktopTargetAMD);
    GET_PROC(wglGenPresentTextureAMD);

    return true;
}


void GLDOPPCapture::notificationLoop()
{
    while (m_bTrackDesktopChanges)
    {
        DWORD dwResult = WaitForMultipleObjects(2, m_hDesktopEvent, FALSE, INFINITE);

        if ((dwResult - WAIT_OBJECT_0) == 0)
        {
            m_bDesktopChanged.store(true);
        }
    }
}