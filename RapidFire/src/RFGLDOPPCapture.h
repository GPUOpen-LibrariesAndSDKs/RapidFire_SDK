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

class GLDOPPCapture
{
public:

    GLDOPPCapture(unsigned int uiDesktop, DOPPDrvInterface* pDrv);
    virtual ~GLDOPPCapture();

    RFStatus            initDOPP(bool bTrackDesktopChanges, bool bBlocking);

    RFStatus            resizeDesktopTexture();

    bool                releaseEvent();

    // Render desktop to rendertarget with id idx.
    bool                processDesktop();

    GLuint              getDesktopTexture() const { return m_uiDesktopTexture; };

    unsigned int        getDesktopWidth()   const { return m_uiDesktopWidth;   };

    unsigned int        getDesktopHeight()  const { return m_uiDesktopHeight;  };

    RFFormat            getDesktopTextureFormat() const;

private:

    bool                setupDOPPExtension();
    void                notificationLoop();

    GLuint                      m_uiDesktopTexture;
    mutable RFFormat            m_rfDesktopFormat;
    const unsigned int          m_uiDesktopId;
    unsigned int                m_uiDesktopWidth;
    unsigned int                m_uiDesktopHeight;

    bool                        m_bTrackDesktopChanges;
    bool                        m_bBlocking;
    static const int            m_iNumContinuedFrames = 3;
    int                         m_iNumRemainingFrames;

    std::atomic_bool            m_bDesktopChanged;

    HANDLE                      m_hDesktopEvent[2];
    std::thread                 m_NotificationThread;
    DOPPDrvInterface*           m_pDOPPDrvInterface;
};