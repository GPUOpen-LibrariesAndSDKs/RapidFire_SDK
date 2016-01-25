#include <fstream>

#include <GL/glew.h>

#include "GLShader.h"

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
    glUseProgram(0);

    glDeleteShader(m_uiVertexShader);
    glDeleteShader(m_uiFragmentShader);

    glDeleteProgram(m_uiProgram);
}


bool GLShader::createVertexShaderFromFile(const char* pFileName)
{
    string strSource;

    if (!readShaderSource(pFileName, strSource))
    {
        return false;
    }

    const char* pSource = strSource.c_str();

    if (!createShader(pSource, GL_VERTEX_SHADER))
    {
        return false;
    }

    return true;
}


bool GLShader::createFragmentShaderFromFile(const char* pFileName)
{
    string strSource;

    if (!readShaderSource(pFileName, strSource))
    {
        return false;
    }

    const char* pSource = strSource.c_str();

    if (!createShader(pSource, GL_FRAGMENT_SHADER))
    {
        return false;
    }

    return true;
}


bool GLShader::createVertexShaderFromString(const char* pSource)
{
    if (!createShader(pSource, GL_VERTEX_SHADER))
    {
        return false;
    }

    return true;
}


bool GLShader::createFragmentShaderFromString(const char* pSource)
{
    if (!createShader(pSource, GL_FRAGMENT_SHADER))
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


bool GLShader::createShader(const char* pSource, unsigned int uiType)
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


bool GLShader::readShaderSource(const char* pFileName, std::string& strSource) const
{
    string	 strLine;
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