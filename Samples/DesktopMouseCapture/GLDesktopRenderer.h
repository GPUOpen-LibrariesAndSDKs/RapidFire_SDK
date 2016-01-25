#pragma once

#include <vector>

#include <GL/glew.h>

class GLShader;

class GLDesktopRenderer
{
public:

    GLDesktopRenderer(unsigned int uiDesktopTexWidth, unsigned int uiDesktopTexHeight, unsigned int uiMouseTexeWidth, unsigned int uiMouseTexHeight);
    
    virtual ~GLDesktopRenderer();

    bool    init();

    void    updateDesktopTexture(const char* pPixels);
    void    updateMouseTexture(const unsigned char* pPixels, unsigned int pixelsWidth, unsigned int pixelsHeight, const unsigned char* pMask, unsigned int maskWidth, unsigned int maskHeight);

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

