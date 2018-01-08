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
// A second thread could call wglDesktopTarget and this would lead to artifacts. The desktop texture
// needs to be rendered into the FBO without beeing interrupted by another desktop session.
static RFLock g_GlobalDOPPLock;

GLDOPPCapture::GLDOPPCapture(unsigned int uiDesktop, unsigned int uiNumFrameBuffers, DOPPDrvInterface* pDrv)
    : m_uiDesktopTexture(0)
    , m_uiDesktopId(uiDesktop)
    , m_uiNumTargets(uiNumFrameBuffers)
    , m_uiDesktopWidth(0)
    , m_uiDesktopHeight(0)
    , m_uiPresentWidth(0)
    , m_uiPresentHeight(0)
    , m_pShader(nullptr)
    , m_pShaderInvert(nullptr)
    , m_uiBaseMap(0)
    , m_uiVertexArray(0)
    , m_pFBO(nullptr)
    , m_pTexture(nullptr)
    , m_bTrackDesktopChanges(false)
    , m_bBlocking(false)
    , m_iNumRemainingFrames(uiNumFrameBuffers)
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

    m_iSamplerSwizzle[0] = GL_RED; m_iSamplerSwizzle[1] = GL_GREEN; m_iSamplerSwizzle[2] = GL_BLUE; m_iSamplerSwizzle[3] = GL_ALPHA;
    m_iResetSwizzle[0] = GL_RED; m_iResetSwizzle[1] = GL_GREEN; m_iResetSwizzle[2] = GL_BLUE; m_iResetSwizzle[3] = GL_ALPHA;
}


GLDOPPCapture::~GLDOPPCapture()
{
    HGLRC glrc = wglGetCurrentContext();

    // Make sure we still have a valid context.
    if (glrc)
    {
        if (m_pShader)
        {
            delete m_pShader;
        }

        if (m_pShaderInvert)
        {
            delete m_pShaderInvert;
        }

        if (m_uiDesktopTexture)
        {
            glDeleteTextures(1, &m_uiDesktopTexture);
        }

        if (m_pFBO)
        {
            glDeleteFramebuffers(m_uiNumTargets, m_pFBO);
        }

        if (m_pTexture)
        {
            glDeleteTextures(m_uiNumTargets, m_pTexture);
        }

        if (m_uiVertexArray)
        {
            glDeleteVertexArrays(1, &m_uiVertexArray);
        }
    }
    else
    {
        RF_Error(RF_STATUS_OPENGL_FAIL, "No more valid context whe ndeleting DOPP Capture");
    }

    if (m_pFBO)
    {
        delete[] m_pFBO;
        m_pFBO = nullptr;
    }

    if (m_pTexture)
    {
        delete[] m_pTexture;
        m_pTexture = nullptr;
    }

    // Set changes tracking to false. This will cause the notifcation thread to stop.
    m_bTrackDesktopChanges = false;

    // Release desktop change notification event to unblock the notification thread.
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

    // Terminate the notification thread.
    if (m_NotificationThread.joinable())
    {
        m_NotificationThread.join();
    }

    // Delete the desktop notification event.
    if (m_hDesktopEvent[0])
    {
        m_pDOPPDrvInterface->deleteDOPPEvent(m_hDesktopEvent[0]);

        m_hDesktopEvent[0] = NULL;
    }
}


RFStatus GLDOPPCapture::initDOPP(unsigned int uiPresentWidth, unsigned int uiPresentHeight, RFFormat outputFormat, bool bTrackDesktopChanges, bool bBlocking)
{
    RFReadWriteAccess doppLock(&g_GlobalDOPPLock);

    HGLRC glrc = wglGetCurrentContext();

    if (!glrc)
    {
        return RF_STATUS_OPENGL_FAIL;
    }

    if (uiPresentWidth <= 0 || uiPresentHeight <= 0)
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    m_uiPresentWidth = uiPresentWidth;
    m_uiPresentHeight = uiPresentHeight;

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

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindTexture(GL_TEXTURE_2D, 0);

    if (!initEffect())
    {
        return RF_STATUS_DOPP_FAIL;
    }

    if (!createRenderTargets())
    {
        return RF_STATUS_DOPP_FAIL;
    }

    glGenVertexArrays(1, &m_uiVertexArray);

    m_iResetSwizzle[0] = GL_RED; m_iResetSwizzle[1] = GL_GREEN; m_iResetSwizzle[2] = GL_BLUE; m_iResetSwizzle[3] = GL_ALPHA;

    if (outputFormat == RF_ARGB8)
    {
        m_iSamplerSwizzle[0] = GL_ALPHA; m_iSamplerSwizzle[1] = GL_RED; m_iSamplerSwizzle[2] = GL_GREEN; m_iSamplerSwizzle[3] = GL_BLUE;
    }
    else if (outputFormat == RF_BGRA8)
    {
        m_iSamplerSwizzle[0] = GL_BLUE; m_iSamplerSwizzle[1] = GL_GREEN; m_iSamplerSwizzle[2] = GL_RED; m_iSamplerSwizzle[3] = GL_ALPHA;
    }
    else if (outputFormat == RF_RGBA8)
    {
        m_iSamplerSwizzle[0] = GL_RED; m_iSamplerSwizzle[1] = GL_GREEN; m_iSamplerSwizzle[2] = GL_BLUE; m_iSamplerSwizzle[3] = GL_ALPHA;
    }

    m_bTrackDesktopChanges = bTrackDesktopChanges;
    m_bBlocking = bBlocking;


    if (m_bBlocking && !m_bTrackDesktopChanges)
    {
        // If a blocking call is requested, we need to track desktop changes.
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


bool GLDOPPCapture::createRenderTargets()
{
    if (m_pFBO != nullptr || m_pTexture != nullptr)
    {
        return false;
    }

    m_pFBO = new GLuint[m_uiNumTargets];
    m_pTexture = new GLuint[m_uiNumTargets];

    glGenFramebuffers(m_uiNumTargets, m_pFBO);
    glGenTextures(m_uiNumTargets, m_pTexture);

    bool bFBStatus = true;

    for (unsigned int i = 0; i < m_uiNumTargets; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, m_pTexture[i]);

        // WORKAROUND to avoid conflicst with AMF avoid using GL_RGBA8.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_uiPresentWidth, m_uiPresentHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        glBindFramebuffer(GL_FRAMEBUFFER, m_pFBO[i]);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTexture[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            bFBStatus = false;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return bFBStatus;
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

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        // Get the size of the desktop. Usually these are the same values as returned by GetSystemMetrics(SM_CXSCREEN)
        // and GetSystemMetrics(SM_CYSCREEN). In some cases they might differ, e.g. if a rotated desktop is used.
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, reinterpret_cast<GLint*>(&m_uiDesktopWidth));
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, reinterpret_cast<GLint*>(&m_uiDesktopHeight));

        glBindTexture(GL_TEXTURE_2D, 0);

        return RF_STATUS_OK;
    }

    return RF_STATUS_INVALID_DESKTOP_ID;
}


RFStatus GLDOPPCapture::resizePresentTexture(unsigned int uiPresentWidth, unsigned int uiPresentHeight)
{
    if (m_pTexture)
    {
        glDeleteTextures(m_uiNumTargets, m_pTexture);

        delete[] m_pTexture;
        m_pTexture = nullptr;
    }

    if (m_pFBO)
    {
        glDeleteFramebuffers(m_uiNumTargets, m_pFBO);

        delete[] m_pFBO;
        m_pFBO = nullptr;
    }


    m_uiPresentWidth = uiPresentWidth;
    m_uiPresentHeight = uiPresentHeight;

    if (!createRenderTargets())
    {
        return RF_STATUS_OPENGL_FAIL;
    }

    return RF_STATUS_OK;
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


bool GLDOPPCapture::initEffect()
{
    if (m_pShader)
    {
        delete m_pShader;
    }

    if (m_pShaderInvert)
    {
        delete m_pShaderInvert;
    }

    const char* strVertexShader =
    {
        "#version 420                                                                     \n"
        "                                                                                 \n"
        "out vec2 Texcoord;                                                               \n"
        "                                                                                 \n"
        "void main( void )                                                                \n"
        "{                                                                                \n"
        "   switch(gl_VertexID)                                                           \n"
        "   {                                                                             \n"
        "      case 0: gl_Position = vec4(-1, -1, 0, 1); Texcoord = vec2(0, 0); break;    \n"
        "      case 1: gl_Position = vec4(-1, 3, 0, 1); Texcoord = vec2(0, 2); break;     \n"
        "      case 2: gl_Position = vec4(3, -1, 0, 1); Texcoord = vec2(2, 0); break;     \n"
        "   }                                                                             \n"
        "}                                                                                \n"
    };

    const char* strVertexShaderInvert =
    {
        "#version 420                                                                     \n"
        "                                                                                 \n"
        "out vec2 Texcoord;                                                               \n"
        "                                                                                 \n"
        "void main( void )                                                                \n"
        "{                                                                                \n"
        "   switch(gl_VertexID)                                                           \n"
        "   {                                                                             \n"
        "      case 0: gl_Position = vec4(-1, -1, 0, 1); Texcoord = vec2(0, 1); break;    \n"
        "      case 1: gl_Position = vec4(-1, 3, 0, 1); Texcoord = vec2(0, -1); break;     \n"
        "      case 2: gl_Position = vec4(3, -1, 0, 1); Texcoord = vec2(2, 1); break;     \n"
        "   }                                                                             \n"
        "}                                                                                \n"
    };

    const char* strFragmentShader =
    {
        "#version 420                                                        \n"
        "                                                                    \n"
        "uniform sampler2D baseMap;                                          \n"
        "                                                                    \n"
        "varying vec2 Texcoord;                                              \n"
        "                                                                    \n"
        "void main(void)                                                     \n"
        "{                                                                   \n"
        "    vec4 texColor = texture2D(baseMap, Texcoord);                   \n"
        "                                                                    \n"
        "    gl_FragColor = vec4(texColor.r, texColor.g, texColor.b, 1.0f);  \n"
        "}                                                                   \n"
    };

    m_pShader = new (std::nothrow)GLShader;

    if (!m_pShader)
    {
        return false;
    }

    if (!m_pShader->createShaderFromString(strVertexShader, GL_VERTEX_SHADER))
    {
        return false;
    }

    if (!m_pShader->createShaderFromString(strFragmentShader, GL_FRAGMENT_SHADER))
    {
        return false;
    }

    if (!m_pShader->buildProgram())
    {
        return false;
    }

    m_pShader->bind();

    m_uiBaseMap = glGetUniformLocation(m_pShader->getProgram(), "baseMap");

    m_pShader->unbind();

    m_pShaderInvert = new (std::nothrow)GLShader;

    if (!m_pShaderInvert)
    {
        return false;
    }

    if (!m_pShaderInvert->createShaderFromString(strVertexShaderInvert, GL_VERTEX_SHADER))
    {
        return false;
    }

    if (!m_pShaderInvert->createShaderFromString(strFragmentShader, GL_FRAGMENT_SHADER))
    {
        return false;
    }

    if (!m_pShaderInvert->buildProgram())
    {
        return false;
    }

    m_pShaderInvert->bind();

    m_uiBaseMap = glGetUniformLocation(m_pShaderInvert->getProgram(), "baseMap");

    m_pShaderInvert->unbind();


    return true;
}


bool GLDOPPCapture::processDesktop(bool bInvert, unsigned int idx)
{
    if (idx >= m_uiNumTargets)
    {
        idx = 0;
    }

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

            m_iNumRemainingFrames = m_uiNumTargets;
        }

        --m_iNumRemainingFrames;
    }

    {
        // GLOBAL LOCK: The operations of selecting the desktop and rendering the desktop texture
        // into the FBO must not be interrupted. Otherwise another thread may select another
        // Desktop by calling wglDesktopTarget while the previous one was not completely processed.
        RFReadWriteAccess doppLock(&g_GlobalDOPPLock);

        glBindFramebuffer(GL_FRAMEBUFFER, m_pFBO[idx]);

        int pVP[4];

        // Store old VP just in case the calling app used OpenGL as well.
        glGetIntegerv(GL_VIEWPORT, pVP);

        glViewport(0, 0, m_uiPresentWidth, m_uiPresentHeight);

        wglDesktopTargetAMD(m_uiDesktopId);

        if (bInvert)
        {
            m_pShaderInvert->bind();
        }
        else
        {
            m_pShader->bind();
        }

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_uiDesktopTexture);

        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, m_iSamplerSwizzle);

        glUniform1i(m_uiBaseMap, 1);

        glBindVertexArray(m_uiVertexArray);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        if (bInvert)
        {
            m_pShaderInvert->unbind();
        }
        else
        {
            m_pShader->unbind();
        }

        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, m_iResetSwizzle);

        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Restore original viewport.
        glViewport(pVP[0], pVP[1], pVP[2], pVP[3]);

        m_bDesktopChanged = false;

        glFinish();
    }

    return true;
}


unsigned int GLDOPPCapture::getFramebufferTex(unsigned int idx) const
{
    if (m_uiNumTargets > 0 && idx < m_uiNumTargets && m_pTexture)
    {
        return m_pTexture[idx];
    }

    return 0;
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