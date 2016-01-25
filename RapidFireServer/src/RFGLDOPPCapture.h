/*****************************************************************************
* Copyright (C) 2013 Advanced Micro Devices, Inc.
* All rights reserved.
*
* This software is provided by the copyright holders and contributors "As is"
* And any express or implied warranties, including, but not limited to, the
* implied warranties of merchantability, non-infringement, and fitness for a
* particular purpose are disclaimed. In no event shall the copyright holder or
* contributors be liable for any direct, indirect, incidental, special,
* exemplary, or consequential damages (including, but not limited to,
* procurement of substitute goods or services; loss of use, data, or profits;
* or business interruption) however caused and on any theory of liability,
* whether in contract, strict liability, or tort (including negligence or
* otherwise) arising in any way out of the use of this software, even if
* advised of the possibility of such damage.
*****************************************************************************/
#pragma once

#include <atomic>
#include <thread>

#include "DoppDrv.h"
#include <GL/glew.h>

#include "RapidFireServer.h"

class GLShader;

class GLDOPPCapture
{
public:

    GLDOPPCapture(unsigned int uiDesktop, DOPPDrvInterface* pDrv);
    virtual ~GLDOPPCapture();

    RFStatus            initDOPP(unsigned int uiPresentWidth, unsigned int uiPresentHeight, float fRotation, bool bTrackDesktopChanges, bool bBlocking);

    RFStatus            resizeDesktopTexture();
    RFStatus            resizePresentTexture(unsigned int uiPresentWidth, unsigned int uiPresentHeight);

    bool                releaseEvent();

    // Render desktop to rendertarget with id idx.
    bool                processDesktop(unsigned int idx);

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
    void                createQuad(float fRotation);

    void                notificationLoop();   

    GLuint                      m_uiDesktopTexture;
    GLuint						m_uiBackupDesktopTexture;

    const unsigned int          m_uiDesktopId;
    const unsigned int          m_uiNumTargets;

    unsigned int                m_uiDesktopWidth;
    unsigned int                m_uiDesktopHeight;
    unsigned int                m_uiPresentWidth;
    unsigned int                m_uiPresentHeight;
    
    GLShader*                   m_pShader;
    GLuint                      m_uiBaseMap;

    GLuint                      m_uiVertexBuffer;
    GLuint                      m_uiVertexArray;
    
    GLuint*                     m_pFBO;
    GLuint*                     m_pTexture;

    bool                        m_bTrackDesktopChanges;
    bool                        m_bBlocking;

    std::atomic_bool            m_bDesktopChanged;

    HANDLE                      m_hDesktopEvent[2];
    std::thread                 m_NotificationThread;
    DOPPDrvInterface*           m_pDOPPDrvInterface;
};