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

class GLShader;

class GLDiffRenderer
{
public:

    GLDiffRenderer(unsigned int uiTextureWidth, unsigned int uiTextureHeight, unsigned int uiDiffMapWidth, unsigned int uiDiffMapHeight, unsigned int uiDiffMapBlockWidth, unsigned int uiDiffMapBlockHeight);
    
    virtual ~GLDiffRenderer();

    bool    init();
    void    updateTexture(const char* pSource, const char* pDiffMap) const;

    void    draw() const;

private:

    bool                    initVBO();
    bool                    initPBO();
    bool                    initShader();

    unsigned int            m_uiTextureDimension[2];
    unsigned int            m_uiDiffMapDimension[2];
    float					m_fDiffMapScaling[2];

    unsigned int            m_uiSourceTextureSize;
    unsigned int            m_uiDiffTexSize;

    GLuint                  m_uiPBO;
    GLuint                  m_uiSourceTexture;
    GLuint                  m_uiDiffTexture;

    GLuint                  m_uiQuad;
    GLuint                  m_uiVertexArray;

    GLShader*               m_pShader;
    GLint                   m_nDiffMapLocation;
    GLint                   m_nSourceTexLocation;
    GLint					m_nDiffMapScalingLocation;
};

