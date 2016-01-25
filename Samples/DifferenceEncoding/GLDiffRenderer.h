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

