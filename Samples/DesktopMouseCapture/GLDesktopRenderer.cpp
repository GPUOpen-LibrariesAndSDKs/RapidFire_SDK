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

#include <GL/glew.h>
#include <GL/wglew.h>
#include <Windows.h>

#include "GLDesktopRenderer.h"
#include "GLShader.h"

#define MULTI_LINE_STRING(a) #a

GLDesktopRenderer::GLDesktopRenderer(unsigned int uiDesktopTexWidth, unsigned int uiDesktopTexHeight, unsigned int uiMouseTexeWidth, unsigned int uiMouseTexHeight)
    : m_uiFBOTexture(0)
    , m_uiFBODepthBuffer(0)
    , m_uiPBO(0)
    , m_uiQuad(0)
    , m_uiVertexArray(0)
    , m_uiDesktopTex(0)
    , m_uiMouseTex(0)
    , m_nTexLocation(0)
    , m_pShader(nullptr)
{
    m_uiDesktopTexDim[0] = uiDesktopTexWidth;
    m_uiDesktopTexDim[1] = uiDesktopTexHeight;

    m_uiMouseTexDim[0] = uiMouseTexeWidth;
    m_uiMouseTexDim[1] = uiMouseTexHeight;

    m_mouseTex.resize (m_uiMouseTexDim[0] * m_uiMouseTexDim[1] * 4);

    m_uiDesktopTexSize = m_uiDesktopTexDim[0] * m_uiDesktopTexDim[1] * 4;
    m_uiMouseTexSize   = m_uiMouseTexDim[0]   * m_uiMouseTexDim[1]   * 4;

    if (m_uiDesktopTexSize * m_uiMouseTexSize == 0)
    {
        throw std::runtime_error("Dimension of 0 is not allowed");
    }
}


GLDesktopRenderer::~GLDesktopRenderer()
{
    if (m_pShader)
    {
        delete m_pShader;
        m_pShader = nullptr;
    }

    if (m_uiFBOTexture)
    {
        glDeleteTextures(1, &m_uiFBOTexture);
        m_uiFBOTexture = 0;
    }

    if (m_uiFBODepthBuffer)
    {
        glDeleteRenderbuffers(1, &m_uiFBODepthBuffer);
        m_uiFBODepthBuffer = 0;
    }

    if (m_uiPBO)
    {
        glDeleteBuffers(1, &m_uiPBO);
        m_uiPBO = 0;
    }

    if (m_uiQuad)
    {
        glDeleteBuffers(1, &m_uiQuad);
        m_uiQuad = 0;
    }

    if (m_uiVertexArray)
    {
        glDeleteVertexArrays(1, &m_uiVertexArray);
        m_uiVertexArray = 0;
    }

    if (m_uiMouseTex)
    {
        glDeleteTextures(1, &m_uiMouseTex);
    }

    if (m_uiDesktopTex)
    {
        glDeleteTextures(1, &m_uiDesktopTex);
    }
}


void GLDesktopRenderer::draw() const
{
    int nViewport[4];
    glGetIntegerv(GL_VIEWPORT, nViewport);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE1);

    glBindTexture(GL_TEXTURE_2D, m_uiDesktopTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_uiMouseTex);

    m_pShader->bind();

    // Draw desktop
    glUniform1i(m_nTexLocation, 1);

    glBindVertexArray(m_uiVertexArray);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // draw mouse
    glViewport(nViewport[2] - (m_uiMouseTexDim[0] * 2), nViewport[3] - (m_uiMouseTexDim[1] * 2), m_uiMouseTexDim[0] * 2, m_uiMouseTexDim[1] * 2);

    glUniform1i(m_nTexLocation, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);

    m_pShader->unbind();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);

    glViewport(nViewport[0], nViewport[1], nViewport[2], nViewport[3]);
}


void GLDesktopRenderer::updateDesktopTexture(const char* pNewTexture)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_uiPBO);

    char* pPixels = static_cast<char*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, m_uiDesktopTexSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    if (pPixels)
    {
        memcpy(pPixels, pNewTexture, m_uiDesktopTexSize);
    }

    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glBindTexture(GL_TEXTURE_2D, m_uiDesktopTex);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_uiDesktopTexDim[0], m_uiDesktopTexDim[1], GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}


void GLDesktopRenderer::updateMouseTexture(const unsigned char* pPixels, unsigned int pixelsWidth, unsigned int pixelsHeight, 
                                           const unsigned char* pMask, unsigned int maskWidth, unsigned int maskHeight, unsigned int maskPitch)
{
    if (pPixels)
    {
        if (pixelsWidth <= m_uiMouseTexDim[0] && pixelsHeight <= m_uiMouseTexDim[1])
        {
            const unsigned char* pTex = pPixels;

            for (unsigned int i = 0; i < pixelsWidth * pixelsHeight * 4; i += 4)
            {
                unsigned char alpha = pTex[i + 3];
                m_mouseTex[i    ] =  (pTex[i    ] * alpha) / 255;
                m_mouseTex[i + 1] =  (pTex[i + 1] * alpha) / 255;
                m_mouseTex[i + 2] =  (pTex[i + 2] * alpha) / 255;
                m_mouseTex[i + 3] = alpha;
            }

            glBindTexture(GL_TEXTURE_2D, m_uiMouseTex);

            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pixelsWidth, pixelsHeight, GL_BGRA, GL_UNSIGNED_BYTE, m_mouseTex.data());

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    else if (pMask)
    {
        memset(m_mouseTex.data(), 0, m_mouseTex.size());

        maskHeight /= 2;
        if (maskWidth <= m_uiMouseTexDim[0] && maskHeight <= m_uiMouseTexDim[1])
        {
            const unsigned char* pTex = pMask + maskPitch * maskHeight;

            for (unsigned int y = 0; y < maskWidth; ++y)
            {
                for (unsigned int x = 0; x < maskHeight; ++x)
                {
                    unsigned char* pColorPixel = &m_mouseTex[(y * maskWidth + x) * 4];
                    
                    if ((pTex[y * maskPitch + x / 8] >> (7 - x % 8)) & 1)
                    {
                        pColorPixel[3] = 255;
                    }
                    else
                    {
                        pColorPixel[3] = 0;
                    }
                }
            }

            glBindTexture(GL_TEXTURE_2D, m_uiMouseTex);

            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, maskWidth, maskHeight, GL_BGRA, GL_UNSIGNED_BYTE, m_mouseTex.data());

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}


bool GLDesktopRenderer::init()
{
    // Create PBOs to download the textures
    initPBO();

    initVBO();

    if (!initShader())
    {
        return false;
    }

    glEnable(GL_BLEND);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Set swap interval to 1 to avoid that some frames don't get displayed due to DWM
    wglSwapIntervalEXT(1);

    return true;
}


void GLDesktopRenderer::initPBO()
{
    // Build PBOs for data transfer of desktop 
    glGenBuffers(1, &m_uiPBO);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_uiPBO);

    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_uiDesktopTexSize, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Gen texture to store desktop image
    glGenTextures(1, &m_uiDesktopTex);

    glBindTexture(GL_TEXTURE_2D, m_uiDesktopTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_uiDesktopTexDim[0], m_uiDesktopTexDim[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Gen texture to store mouse image
    glGenTextures(1, &m_uiMouseTex);

    glBindTexture(GL_TEXTURE_2D, m_uiMouseTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_uiMouseTexDim[0], m_uiMouseTexDim[1], 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}


void GLDesktopRenderer::initVBO()
{
    const float pVertexData[] = {  1.0f, -1.0f,  0.0f,    1.0f, 1.0f,
                                   1.0f,  1.0f,  0.0f,    1.0f, 0.0f,
                                  -1.0f, -1.0f,  0.0f,    0.0f, 1.0f,
                                  -1.0f,  1.0f,  0.0f,    0.0f, 0.0f };

    // Create Vertex Buffer object
    glGenBuffers(1, &m_uiQuad);

    glBindBuffer(GL_ARRAY_BUFFER, m_uiQuad);

    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), pVertexData, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &m_uiVertexArray);

    // Create Vertex array
    glBindVertexArray(m_uiVertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, m_uiQuad);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


bool GLDesktopRenderer::initShader()
{
    const char pVertexShader[] = MULTI_LINE_STRING( #version 420 core   \n

                                                    layout(location = 0) in vec4 inVertex;
                                                    layout(location = 1) in vec2 inTexCoord;

                                                    out vec2 TexCoord;

                                                    void main()
                                                    {
                                                        TexCoord    = inTexCoord;
                                                        gl_Position = inVertex;
                                                    }
                                                  );

    const char pFragShader[] = MULTI_LINE_STRING(   #version 420 core \n
                        
                                                    uniform sampler2D  baseMap;
                                                    
                                                    in vec2 TexCoord;

                                                    void main()
                                                    {
                                                        vec4 color = texture(baseMap, TexCoord);
                                                      
                                                        gl_FragColor = vec4(color.r, color.g, color.b, color.a + 0.3);
                                                    }
                                                );
    
    if (m_pShader)
    {
        delete m_pShader;
    }

    m_pShader = new GLShader;

    if (!m_pShader->createVertexShaderFromString(pVertexShader))
    {
        return false;
    }

    if (!m_pShader->createFragmentShaderFromString(pFragShader))
    {
        return false;
    }

    if (!m_pShader->buildProgram())
    {
        return false;
    }

    m_nTexLocation = glGetUniformLocation(m_pShader->getProgram(), "baseMap");

    if (m_nTexLocation < 0)
    {
        return false;
    }

    m_pShader->bind();

    glUniform1i(m_nTexLocation, 1);

    m_pShader->unbind();

    return true;
}