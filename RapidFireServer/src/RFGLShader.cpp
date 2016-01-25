/*****************************************************************************
* Copyright (C) 2013 Advanced Micro Devices, Inc.
* All rights reserved.
*
* This software is provided by the copyright holders and contributors "As is"
* And any express or implied warranties, including, but not limited to, the
* implied warranties of merchantability, non-infringement, and fitness for a
* particular purpose are disclaimed. In no event shall the copyright holder or
* contributors be liable for any direct, indirect, incidental, special,
* exemplary, or consequential damages (including, but not limited to,
* procurement of substitute goods or services; loss of use, data, or profits;
* or business interruption) however caused and on any theory of liability,
* whether in contract, strict liability, or tort (including negligence or
* otherwise) arising in any way out of the use of this software, even if
* advised of the possibility of such damage.
*****************************************************************************/
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