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
#include <Windows.h>

#include "FireCube.h"

#define MULTI_STRING(a) #a

extern char* readRGBimage(int &xsize, int &ysize, int &channels, const char* fname);

namespace
{
struct VERTEX_ELEMENT
{
    float   position[3];
    float   normal[3];
    float   texcoord[2];
};


struct LIGHT_DATA
{
    float   LightDirection[4];
    float   DiffuseColor[4];
    float   AmbientColor[4];
    float   SpecularColor[4];
};
}



FireCube::FireCube()
    : m_uiVertexBuffer(0)
    , m_uiElementBuffer(0)
    , m_uiVertexArray(0)
    , m_uiTexture(0)
    , m_nSampler(0)
    , m_uiLightBuffer(0)
{}


FireCube::~FireCube()
{
    if (m_uiVertexBuffer)
    {
        glDeleteBuffers(1, &m_uiVertexBuffer);
    }

    if (m_uiElementBuffer)
    {
        glDeleteBuffers(1, &m_uiElementBuffer);
    }

    if (m_uiLightBuffer)
    {
        glDeleteBuffers(1, &m_uiLightBuffer);
    }

    if (m_uiVertexArray)
    {
        glDeleteVertexArrays(1, &m_uiVertexArray);
    }

    if (m_uiTexture)
    {
        glDeleteTextures(1, &m_uiTexture);
    }
}


bool FireCube::initProgram()
{
    const char* pVShader = MULTI_STRING(    #version 420 compatibility \n
                                            
                                            layout(location = 0) in vec4 inVertex;
                                            layout(location = 1) in vec4 inNormal;
                                            layout(location = 2) in vec2 inTexCoord;

                                            out vec3 vNormal;
                                            out vec3 vPosition;
                                            out vec2 vTextureCoord;

                                            void main()
                                            {
                                                // transform vertex position into eye space
                                                vPosition = (gl_ModelViewMatrix * inVertex).xyz;

                                                // Transform normal into eye space
                                                vNormal   = (gl_ModelViewMatrix * vec4(inNormal.x, inNormal.y, inNormal.z, 0.0f)).xyz;

                                                vTextureCoord = inTexCoord;

                                                gl_Position = gl_ModelViewProjectionMatrix * inVertex;
                                            }
                                       );

    const char* pFShader = MULTI_STRING(    #version 420 compatibility \n

                                            layout(shared, binding = 1) uniform LightColor
                                            {
                                               vec4 LightDir;
                                               vec4 DiffuseColor;
                                               vec4 AmbientColor;
                                               vec4 SpecularColor;
                                            };

                                            layout(binding = 1) uniform sampler2D  BaseMap;

                                            in vec2 vTextureCoord;
                                            in vec3 vNormal;
                                            in vec3 vPosition;


                                            void main()
                                            {
                                               vec4 vTexColor  = texture2D(BaseMap, vTextureCoord);
                                               vec4 vBaseColor = mix(vec4(0.4f, 0.4f, 0.4f, 1.0f), vTexColor, vTexColor.a); 

                                               vec3 vN = normalize(vNormal);
                                                
                                               float NdotL = max(dot(vN, -LightDir.xyz), 0.0);

                                               
                                               vec3 vR    = reflect(LightDir.xyz, vN);  
                                               float Spec = pow(max(dot(normalize(vR), normalize(-vPosition)), 0.0), 6.0);
                                               
                                               gl_FragColor = vBaseColor * DiffuseColor * NdotL + vBaseColor * AmbientColor + Spec * SpecularColor;

                                            }
                                         );

    if (!m_GLShader.createVertexShaderFromString(pVShader))
    {
        return false;
    }

    if (!m_GLShader.createFragmentShaderFromString(pFShader))
    {
        return false;
    }

    if (!m_GLShader.buildProgram())
    {
        return false;
    }

    return true;
}


bool FireCube::initTexture(const char* pFileName)
{
    int nTexWidth    = 0;
    int nTexHeight   = 0;
    int nTexChannels = 0;

    char* pPixels = readRGBimage(nTexWidth, nTexHeight, nTexChannels, pFileName);

    if (!pPixels)
    {
        return false;
    }

    glGenTextures(1, &m_uiTexture);

    glBindTexture(GL_TEXTURE_2D, m_uiTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nTexWidth, nTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);

    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] pPixels;

    return true;
}


bool FireCube::initBuffer()
{
    const VERTEX_ELEMENT vertexBuffer[] = { -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,       0.0f, 0.0f,      // Front Tris 0,1,2 0,2,3
                                             0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
                                             0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
                                            -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,       0.0f, 1.0f,

                                             0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,       0.0f, 0.0f,      // Right Tris 4,5,6 4,6,7
                                             0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
                                             0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
                                             0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,       0.0f, 1.0f,

                                             0.5f, -0.5f, -0.5f,    0.0f, 0.0f,-1.0f,       0.0f, 0.0f,      // Back Tris 8,9,10 8,10,11
                                            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,-1.0f,       1.0f, 0.0f,
                                            -0.5f,  0.5f, -0.5f,    0.0f, 0.0f,-1.0f,       1.0f, 1.0f,
                                             0.5f,  0.5f, -0.5f,    0.0f, 0.0f,-1.0f,       0.0f, 1.0f,

                                            -0.5f, -0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,       0.0f, 0.0f,      // Left Tris 12,13,14 12,14,15
                                            -0.5f, -0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
                                            -0.5f,  0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
                                            -0.5f,  0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,       0.0f, 1.0f,

                                            -0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,       0.0f, 0.0f,      // Top Tris 16,17,18 16,18,19
                                             0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,       1.0f, 0.0f,
                                             0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,       1.0f, 1.0f,
                                            -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,       0.0f, 1.0f,

                                            -0.5f, -0.5f, -0.5f,    0.0f,-1.0f, 0.0f,       0.0f, 0.0f,      // Bottom Tris 20,21,22 20,22,23
                                             0.5f, -0.5f, -0.5f,    0.0f,-1.0f, 0.0f,       1.0f, 0.0f,
                                             0.5f, -0.5f,  0.5f,    0.0f,-1.0f, 0.0f,       1.0f, 1.0f,
                                            -0.5f, -0.5f,  0.5f,    0.0f,-1.0f, 0.0f,       0.0f, 1.0f };

    const GLushort indexBuffer[] = {  0, 1,  2,    0,  2,  3,           // Front
                                      4, 5,  6,    4,  6,  7,           // Right
                                      8, 9, 10,    8, 10, 11,           // Back
                                     12, 13, 14,  12, 14, 15,           // Left
                                     16, 17, 18,  16, 18, 19,           // Top
                                     20, 21, 22,  20, 22, 23  };        // Bottom

    // Create vertex buffer
    glGenBuffers(1, &m_uiVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVertexBuffer);

    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(VERTEX_ELEMENT), vertexBuffer, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create element buffer
    glGenBuffers(1, &m_uiElementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiElementBuffer);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLushort), indexBuffer, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
 
    // Create UBO for light data
    glGenBuffers(1, &m_uiLightBuffer);
 
    glBindBuffer(GL_UNIFORM_BUFFER, m_uiLightBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(LIGHT_DATA), nullptr, GL_DYNAMIC_DRAW);

    LIGHT_DATA* ptr = static_cast<LIGHT_DATA*>(glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(LIGHT_DATA), GL_MAP_WRITE_BIT));

    if (!ptr)
    {
        return false;
    }

    ptr->LightDirection[0] =  0.0f;
    ptr->LightDirection[1] = -0.707107f;
    ptr->LightDirection[2] = -0.707107f;
    ptr->LightDirection[3] =  0.0f;

    ptr->AmbientColor[0] = 0.2f;
    ptr->AmbientColor[1] = 0.2f;
    ptr->AmbientColor[2] = 0.2f;
    ptr->AmbientColor[3] = 1.0f;

    ptr->DiffuseColor[0] = 1.0f;
    ptr->DiffuseColor[1] = 1.0f;
    ptr->DiffuseColor[2] = 1.0f;
    ptr->DiffuseColor[3] = 1.0f;

    ptr->SpecularColor[0] = 1.0f; 
    ptr->SpecularColor[1] = 1.0f;
    ptr->SpecularColor[2] = 1.0f;
    ptr->SpecularColor[3] = 1.0f;

    glUnmapBuffer(GL_UNIFORM_BUFFER);

    glBindBuffer(GL_UNIFORM_BUFFER,  0);

    return true;
}


bool FireCube::initVertexArray()
{
    // Create vertex arrays
    glGenVertexArrays(1, &m_uiVertexArray);
    glBindVertexArray(m_uiVertexArray);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_uiVertexBuffer);

    // Enable vertex array to pass vertex data to the shader. 
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VERTEX_ELEMENT), nullptr);

    // Enable vertex array to pass normal data to the shader.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(VERTEX_ELEMENT), reinterpret_cast<void*>(3 * sizeof(float)));

    // Enable vertex array to pass texture data to the shader. 
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VERTEX_ELEMENT), reinterpret_cast<void*>(6 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiElementBuffer);

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return true;
}


bool FireCube::init()
{
    glClearColor(0.3f, 0.3f, 0.5f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    if (!initTexture("AMD_FirePro.rgb"))
    {
        return false;
    }

    if (!initProgram())
    {
        return false;
    }

    if (!initBuffer())
    {
        return false;
    }

    if (!initVertexArray())
    {
        return false;
    }

    return true;
}


void FireCube::draw() const
{    
    m_GLShader.bind();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_uiTexture);

    glBindVertexArray(m_uiVertexArray);

    // Bind UBO containing light data to binding point 1
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_uiLightBuffer);
    
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, NULL);
    
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_GLShader.unbind();
}