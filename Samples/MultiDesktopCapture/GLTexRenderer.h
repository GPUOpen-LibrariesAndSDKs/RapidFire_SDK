#pragma once

#include <GL/glew.h>

class GLShader;

class GLTexRenderer
{
public:

    GLTexRenderer(unsigned int uiTextureWidth, unsigned int uiTextureHeight);
    
    virtual ~GLTexRenderer();

    bool    init();
    void    updateTexture(const char* pSource);

    void    draw() const;

private:

    bool           initVBO();
    bool           initPBO();
    bool           initShader();

    unsigned int   m_uiTextureDimension[2];

    unsigned int   m_uiSourceTextureSize;

    GLuint         m_uiPBO;
    GLuint         m_uiSourceTexture;

    GLuint         m_uiQuad;
    GLuint         m_uiVertexArray;

    GLShader*      m_pShader;
    GLint          m_nSourceTexLocation;
};