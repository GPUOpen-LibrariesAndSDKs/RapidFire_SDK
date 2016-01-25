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
