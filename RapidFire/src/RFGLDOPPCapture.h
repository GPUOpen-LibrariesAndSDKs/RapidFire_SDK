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

#pragma once

#include <atomic>
#include <thread>

#include "DoppDrv.h"
#include <GL/glew.h>

#include "RapidFire.h"

class GLShader;

class GLDOPPCapture
{
public:

    GLDOPPCapture(unsigned int uiDesktop, unsigned int uiNumFrameBuffers, DOPPDrvInterface* pDrv);
    virtual ~GLDOPPCapture();

    RFStatus            initDOPP(unsigned int uiPresentWidth, unsigned int uiPresentHeight, RFFormat outputFormat, bool bTrackDesktopChanges, bool bBlocking);

    RFStatus            resizeDesktopTexture();
    RFStatus            resizePresentTexture(unsigned int uiPresentWidth, unsigned int uiPresentHeight);

    bool                releaseEvent();

    // Render desktop to rendertarget with id idx.
    bool                processDesktop(bool bInvert, unsigned int idx);

    // Returns the texture name of the texture that is used with render targt idx.
    unsigned int        getFramebufferTex(unsigned int idx) const;

    GLuint              getDesktopTexture()     const   { return m_uiDesktopTexture;    };

    unsigned int        getNumFramebufferTex()  const   { return m_uiNumTargets;        };

    unsigned int        getDesktopWidth()       const   { return m_uiDesktopWidth;      };

    unsigned int        getDesktopHeight()      const   { return m_uiDesktopHeight;     };

    unsigned int        getPresentWidth()       const   { return m_uiPresentWidth;      };

    unsigned int        getPresentHeight()      const   { return m_uiPresentHeight;     };

private:

    bool                setupDOPPExtension();
    bool                createRenderTargets();
    bool                initEffect();

    void                notificationLoop();   

    GLuint                      m_uiDesktopTexture;
    GLuint						m_uiBackupDesktopTexture;

    const unsigned int          m_uiDesktopId;
    const unsigned int          m_uiNumTargets;

    unsigned int                m_uiDesktopWidth;
    unsigned int                m_uiDesktopHeight;
    unsigned int                m_uiPresentWidth;
    unsigned int                m_uiPresentHeight;
    GLint                       m_iSamplerSwizzle[4];
    GLint                       m_iResetSwizzle[4];
    
    GLShader*                   m_pShader;
    GLShader*                   m_pShaderInvert;
    GLuint                      m_uiBaseMap;

    GLuint                      m_uiVertexArray;
    
    GLuint*                     m_pFBO;
    GLuint*                     m_pTexture;

    bool                        m_bTrackDesktopChanges;
    bool                        m_bBlocking;
    int                         m_iNumRemainingFrames;

    std::atomic_bool            m_bDesktopChanged;

    HANDLE                      m_hDesktopEvent[2];
    std::thread                 m_NotificationThread;
    DOPPDrvInterface*           m_pDOPPDrvInterface;
};