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

#include "GLDiffRenderer.h"
#include "GLShader.h"

#define MULTI_LINE_STRING(a) #a


GLDiffRenderer::GLDiffRenderer(unsigned int const uiTextureWidth, unsigned int const uiTextureHeight, unsigned int const uiDiffMapWidth, unsigned int const uiDiffMapHeight, unsigned int uiDiffMapBlockWidth, unsigned int uiDiffMapBlockHeight)
    : m_uiPBO(0)
    , m_uiQuad(0)
    , m_uiVertexArray(0)
    , m_uiSourceTexture(0)
    , m_uiDiffTexture(0)
    , m_nSourceTexLocation(0)
    , m_nDiffMapLocation(0)
    , m_nDiffMapScalingLocation(0)
    , m_pShader(nullptr)
{
    m_uiTextureDimension[0] = uiTextureWidth;
    m_uiTextureDimension[1] = uiTextureHeight;

    m_uiDiffMapDimension[0] = uiDiffMapWidth;
    m_uiDiffMapDimension[1] = uiDiffMapHeight;

    m_fDiffMapScaling[0] = static_cast<float> (uiTextureWidth) / (uiDiffMapBlockWidth * uiDiffMapWidth);
    m_fDiffMapScaling[1] = static_cast<float> (uiTextureHeight) / (uiDiffMapBlockHeight * uiDiffMapHeight);

    m_uiSourceTextureSize = uiTextureWidth * uiTextureHeight * 4;
    m_uiDiffTexSize       = m_uiDiffMapDimension[0] * m_uiDiffMapDimension[1];

    if (m_uiSourceTextureSize == 0 || m_uiDiffTexSize == 0)
    {
        throw std::runtime_error("Dimension of 0 is not allowed");
    }
}


GLDiffRenderer::~GLDiffRenderer()
{
    if (m_pShader)
    {
        delete m_pShader;
        m_pShader = nullptr;
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

    if (m_uiDiffTexture)
    {
        glDeleteTextures(1, &m_uiDiffTexture);
    }

    if (m_uiSourceTexture)
    {
        glDeleteTextures(1, &m_uiSourceTexture);
    }
}


void GLDiffRenderer::draw() const
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_uiSourceTexture);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_uiDiffTexture);

    m_pShader->bind();

    glBindVertexArray(m_uiVertexArray);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);

    m_pShader->unbind();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);
}


void GLDiffRenderer::updateTexture(const char* pSource, const char* pDiffMap) const
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_uiPBO);

    char* pPixels = static_cast<char*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, m_uiSourceTextureSize + m_uiDiffTexSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    if (pPixels)
    {
        memcpy(pPixels, pSource, m_uiSourceTextureSize);
        memcpy(&pPixels[m_uiSourceTextureSize], pDiffMap, m_uiDiffTexSize);
    }

    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glBindTexture(GL_TEXTURE_2D, m_uiSourceTexture);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_uiTextureDimension[0], m_uiTextureDimension[1], GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, m_uiDiffTexture);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_uiDiffMapDimension[0], m_uiDiffMapDimension[1], GL_RED, GL_UNSIGNED_BYTE, reinterpret_cast<void*>(static_cast<uintptr_t>(m_uiSourceTextureSize)));

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}


bool GLDiffRenderer::init()
{
    // Create PBOs to download the textures
    if (!initPBO())
    {
        return false;
    }

    if (!initVBO())
    {
        return false;
    }

    if (!initShader())
    {
        return false;
    }

    // check alignment of diff map textures. The diff map texture uses 1 byte per pixel
    if (m_uiDiffMapDimension[0] % 4 != 0)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    // Set swap interval to 1 to avoid that some frames don't get displayed due to DWM
    wglSwapIntervalEXT(1);

    return true;
}


bool GLDiffRenderer::initPBO()
{
    // Build PBOs for data transfer
    glGenBuffers(1, &m_uiPBO);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_uiPBO);

    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_uiSourceTextureSize + m_uiDiffTexSize, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Gen texture to store source image
    glGenTextures(1, &m_uiSourceTexture);

    glBindTexture(GL_TEXTURE_2D, m_uiSourceTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_uiTextureDimension[0], m_uiTextureDimension[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Gen texture to store diff image
    glGenTextures(1, &m_uiDiffTexture);

    glBindTexture(GL_TEXTURE_2D, m_uiDiffTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_uiDiffMapDimension[0], m_uiDiffMapDimension[1], 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}


bool GLDiffRenderer::initVBO()
{
    const float pVertexData[] = {  1.0f, -1.0f,  0.0f,    1.0f, 0.0f,
                                   1.0f,  1.0f,  0.0f,    1.0f, 1.0f,
                                  -1.0f, -1.0f,  0.0f,    0.0f, 0.0f,
                                  -1.0f,  1.0f,  0.0f,    0.0f, 1.0f };

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

    return true;
}


bool GLDiffRenderer::initShader()
{
    const char pVertexShader[] = MULTI_LINE_STRING( #version 420 core   \n

                                                    layout(location = 0) in vec4 inVertex;
                                                    layout(location = 1) in vec2 inTexCoord;

                                                    out vec2 TexCoord;

                                                    void main()
                                                    {
                                                        TexCoord   = inTexCoord;
                                                        gl_Position = inVertex;
                                                    }
                                                  );

    const char pFragShader[] = MULTI_LINE_STRING(   #version 420 core \n
                        
                                                    uniform sampler2D  SourceMap;
                                                    uniform sampler2D  DiffMap;

                                                    uniform vec2 DiffMapScaling;

                                                    in vec2 TexCoord;

                                                    void main()
                                                    {
                                                        vec4 scolor = texture(SourceMap, TexCoord);
                                                        vec4 dcolor = texture(DiffMap,   TexCoord * DiffMapScaling) * 255.0f;

                                                        gl_FragColor = mix(scolor, vec4(dcolor.x, 0.0f, 0.0f, 1.0f), dcolor.x * 0.5f);
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

    m_nSourceTexLocation = glGetUniformLocation(m_pShader->getProgram(), "SourceMap");
    m_nDiffMapLocation   = glGetUniformLocation(m_pShader->getProgram(), "DiffMap");
    m_nDiffMapScalingLocation = glGetUniformLocation(m_pShader->getProgram(), "DiffMapScaling");

    if (m_nSourceTexLocation < 0 || m_nDiffMapLocation < 0 || m_nDiffMapScalingLocation < 0)
    {
        return false;
    }

    m_pShader->bind();

    glUniform1i(m_nSourceTexLocation, 1);
    glUniform1i(m_nDiffMapLocation,   2);
    glUniform2f(m_nDiffMapScalingLocation, m_fDiffMapScaling[0], m_fDiffMapScaling[1]);

    m_pShader->unbind();

    return true;
}