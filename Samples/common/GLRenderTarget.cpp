#include <GL/glew.h>

#include "GLRenderTarget.h"


GLRenderTarget::GLRenderTarget()
    : m_uiBufferId(0)
    , m_uiBufferWidth(0)
    , m_uiBufferHeight(0)
    , m_nBufferFormat(0)
{}


GLRenderTarget::~GLRenderTarget()
{
    deleteBuffer();
}


bool GLRenderTarget::createBuffer(unsigned int uiWidth, unsigned int uiHeight, int nBufferFormat, int nExtFormat, int nType)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (m_uiBufferId == 0)
    {
        m_uiBufferWidth  = uiWidth;
        m_uiBufferHeight = uiHeight;
        m_nBufferFormat  = nBufferFormat;
        m_nExtFormat     = nExtFormat;
        m_nType          = nType;

        // Setup texture to be used as color attachment
        glGenTextures(1, &m_uiColorTex);

        glBindTexture(GL_TEXTURE_2D, m_uiColorTex);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, m_nBufferFormat, m_uiBufferWidth, m_uiBufferHeight, 0, m_nExtFormat, m_nType, nullptr);

        // Create FBO with color and depth attachment
        glGenFramebuffers(1, &m_uiBufferId);
        glGenRenderbuffers(1, &m_uiDepthBuffer);

        glBindFramebuffer(GL_FRAMEBUFFER, m_uiBufferId);

        glBindRenderbuffer(GL_RENDERBUFFER, m_uiDepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_uiBufferWidth, m_uiBufferHeight);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_uiColorTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_uiDepthBuffer);

        GLenum FBStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (FBStatus == GL_FRAMEBUFFER_COMPLETE)
        {
            return true;
        }
    }

    return false;
}


void GLRenderTarget::deleteBuffer()
{
    if (m_uiColorTex)
    {
        glDeleteTextures(1, &m_uiColorTex);
    }

    if (m_uiDepthBuffer)
    {
        glDeleteRenderbuffers(1, &m_uiDepthBuffer);
    }

    if (m_uiBufferId)
    {
        glDeleteFramebuffers(1, &m_uiBufferId);
    }

    m_uiBufferId = 0;
}


void GLRenderTarget::bind(GLenum nTarget) const
{
    if (m_uiBufferId)
    {
        glBindFramebuffer(nTarget, m_uiBufferId);
    }
}


void GLRenderTarget::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void GLRenderTarget::draw() const
{
    int nViewport[4];

    glGetIntegerv(GL_VIEWPORT, nViewport);

    float left, right, bottom, top;

    left = -static_cast<float>(nViewport[2]) / 2.0f;
    right = -left;
    bottom = -static_cast<float>(nViewport[3]) / 2.0f;
    top = -bottom;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_uiBufferId);

    glBlitFramebuffer(0, 0, m_uiBufferWidth, m_uiBufferHeight, nViewport[0], nViewport[1], nViewport[2], nViewport[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}


unsigned int GLRenderTarget::getBufferHeight() const
{
    if (m_uiBufferId)
    {
        return m_uiBufferHeight;
    }

    return 0;
}


unsigned int GLRenderTarget::getBufferWidth() const
{
    if (m_uiBufferId)
    {
        return m_uiBufferWidth;
    }

    return 0;
}


int GLRenderTarget::getBufferFormat() const
{
    if (m_uiBufferId)
    {
        return m_nBufferFormat;
    }

    return 0;
}