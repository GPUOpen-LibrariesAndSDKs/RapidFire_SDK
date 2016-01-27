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

#include <GL/glew.h>

class GLRenderTarget
{
public:
    GLRenderTarget();
    ~GLRenderTarget();

    // Create FBO with specified dimension and format
    bool    createBuffer(unsigned int nWidth, unsigned int nHeight, int nBufferFormat, int nExtFormat, int nType);
    // Delete FBO and storage
    void    deleteBuffer();

    // Bind FBO
    void    bind(GLenum nTarget = GL_FRAMEBUFFER) const;

    // Release FBO
    void    unbind() const;

    // Draws color attachment as texture into a screen aligned quad
    void    draw() const;

    int             getBufferFormat() const;
    unsigned int    getBufferWidth() const;
    unsigned int    getBufferHeight() const;
    unsigned int    getColorTex() const { return m_uiColorTex; };

private:

    GLuint          m_uiBufferId;
    GLuint          m_uiColorTex;
    GLuint          m_uiDepthBuffer;
    unsigned int    m_uiBufferWidth;
    unsigned int    m_uiBufferHeight;
    unsigned int    m_uiQuad;

    int             m_nBufferFormat;
    int             m_nExtFormat;
    int             m_nType;
};
