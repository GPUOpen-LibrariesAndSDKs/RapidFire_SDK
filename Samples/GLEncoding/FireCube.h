#pragma once

#include <GL/glew.h>
#include "GLShader.h"

class FireCube
{
public:
    FireCube(void);
    ~FireCube(void);

    bool    init();

    void    draw() const;

private:

    bool     initBuffer();
    bool     initVertexArray();
    bool     initProgram();
    bool     initTexture(const char* pFileName);

    GLuint	 m_uiVertexBuffer;
    GLuint   m_uiElementBuffer;
    GLuint   m_uiVertexArray;
    GLuint   m_uiTexture;
    GLint    m_nSampler;

    GLuint   m_MVMLocation;
    GLuint   m_MVPLocation;
    GLuint   m_uiLightBuffer;

    GLShader m_GLShader;
};

