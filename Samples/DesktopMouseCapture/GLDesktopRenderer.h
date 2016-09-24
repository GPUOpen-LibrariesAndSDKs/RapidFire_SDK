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

#include <vector>
#include <stdint.h>

#include <GL/glew.h>

class GLShader;

class GLDesktopRenderer
{
public:

    GLDesktopRenderer(unsigned int uiDesktopTexWidth, unsigned int uiDesktopTexHeight, unsigned int uiMouseTexeWidth, unsigned int uiMouseTexHeight);
    
    virtual ~GLDesktopRenderer();

    bool    init();

    void    updateDesktopTexture(const char* pPixels);
    void    updateMouseTexture(const unsigned char* pPixels, unsigned int pixelsWidth, unsigned int pixelsHeight, 
                               const unsigned char* pMask, unsigned int maskWidth, unsigned int maskHeight, unsigned int maskPitch);

    void    draw() const;

private:

    void                 initVBO();
    void                 initPBO();
    bool                 initShader();
                         
    unsigned int                  m_uiDesktopTexDim[2];
    unsigned int                  m_uiMouseTexDim[2];
                                  
    unsigned int                  m_uiDesktopTexSize;
    unsigned int                  m_uiMouseTexSize;
                                  
    GLuint                        m_uiFBOTexture;
    GLuint                        m_uiFBODepthBuffer;
                                  
    GLuint                        m_uiPBO;
    GLuint                        m_uiDesktopTex;
    GLuint                        m_uiMouseTex;
    std::vector<unsigned char>    m_mouseTex;

    GLuint                        m_uiQuad;
    GLuint                        m_uiVertexArray;
                                  
    GLShader*                     m_pShader;
    GLint                         m_nTexLocation;
};

