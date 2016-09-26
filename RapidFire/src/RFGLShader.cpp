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

#include "RFGLShader.h"

#include <fstream>

using namespace std;


GLShader::GLShader()
    : m_uiVertexShader(0)
    , m_uiFragmentShader(0)
    , m_uiProgram(0)
{
    m_strErrorMessage.clear();
}


GLShader::~GLShader()
{
    if (m_uiProgram)
    {
        int currentProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        if (currentProgram)
        {
            glUseProgram(0);
            glDeleteProgram(m_uiProgram);
        }
    }
}


bool GLShader::createVertexShaderFromFile(const char* pFileName)
{
    if (!createShaderFromFile(pFileName, GL_VERTEX_SHADER))
    {
        return false;
    }

    return true;
}


bool GLShader::createFragmentShaderFromFile(const char* pFileName)
{
    if (!createShaderFromFile(pFileName, GL_FRAGMENT_SHADER))
    {
        return false;
    }

    return true;
}


bool GLShader::buildProgram()
{
    if (!m_uiVertexShader || !m_uiFragmentShader)
    {
        return false;
    }

    if (m_uiProgram)
    {
        glDeleteProgram(m_uiProgram);
    }

    m_uiProgram = glCreateProgram();

    glAttachShader(m_uiProgram, m_uiVertexShader);
    glAttachShader(m_uiProgram, m_uiFragmentShader);

    glLinkProgram(m_uiProgram);

    int  nStatus;
    int  nLength;
    char cMessage[256];

    glGetProgramiv(m_uiProgram, GL_LINK_STATUS, &nStatus);

    if (nStatus != GL_TRUE)
    {
        glGetProgramInfoLog(m_uiProgram, 256, &nLength, cMessage);

        m_strErrorMessage = cMessage;

        return false;
    }

    return true;
}


bool GLShader::createShaderFromString(const char* pSource, unsigned int uiType)
{
    unsigned int uiShader = 0;

    if (uiType == GL_VERTEX_SHADER)
    {
        m_uiVertexShader = glCreateShader(GL_VERTEX_SHADER);

        uiShader = m_uiVertexShader;
    }
    else if (uiType == GL_FRAGMENT_SHADER)
    {
        m_uiFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        uiShader = m_uiFragmentShader;
    }

    glShaderSource(uiShader, 1, &pSource, nullptr);

    glCompileShader(uiShader);

    int  nStatus;
    int  nLength;
    char cMessage[256];

    glGetShaderiv(uiShader, GL_COMPILE_STATUS, &nStatus);

    if (nStatus != GL_TRUE)
    {
        glGetShaderInfoLog(uiShader, 256, &nLength, cMessage);

        m_strErrorMessage = cMessage;

        return false;
    }

    return true;
}


bool GLShader::createShaderFromFile(const char* pFileName, unsigned int uiType)
{
    string strSource;

    if (!readShaderSource(pFileName, strSource))
    {
        return false;
    }

    const char* pSource = strSource.c_str();

    return createShaderFromString(pSource, uiType);
}


bool GLShader::readShaderSource(const char* pFileName, std::string &strSource)
{
    string   strLine;
    ifstream ShaderFile(pFileName);

    if (!ShaderFile.is_open())
    {
        return false;
    }

    while (!ShaderFile.eof())
    {
        getline(ShaderFile, strLine);
        strSource += strLine;
        strSource += "\n";
    }

    return true;
}