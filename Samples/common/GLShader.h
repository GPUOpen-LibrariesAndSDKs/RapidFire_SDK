#pragma once
#include <string>

class GLShader
{
public:

    GLShader();
    virtual ~GLShader();

    bool    createVertexShaderFromFile(const char*   pFileName);
    bool    createFragmentShaderFromFile(const char* pFileName);

    bool    createVertexShaderFromString(const char*   pSource);
    bool    createFragmentShaderFromString(const char* pSource);

    bool    buildProgram();

    void    bind() const                    { glUseProgram(m_uiProgram); };
    void    unbind() const                  { glUseProgram(0);           };
                                      
    unsigned int    getProgram() const      { return m_uiProgram; };
    const char*     getErrorMessage() const { return m_strErrorMessage.c_str(); };

private:

    bool            readShaderSource(const char* pFileName, std::string& strSource) const;
    bool            createShader(const char* pSource, unsigned int uiType);

    unsigned int    m_uiVertexShader;
    unsigned int    m_uiFragmentShader;
    unsigned int    m_uiProgram;

    std::string     m_strErrorMessage;
};