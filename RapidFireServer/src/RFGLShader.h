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
#pragma once

#include <string>

#include <GL/glew.h>

class GLShader
{
public:

    GLShader();
    virtual ~GLShader();

    bool            createVertexShaderFromFile(const char* pFileName);
    bool            createFragmentShaderFromFile(const char* pFileName);
                    
    bool            createShaderFromString(const char* pSource, unsigned int uiType);
                    
    bool            buildProgram();
                    
    void            bind()              { glUseProgram(m_uiProgram);        }
                    
    void            unbind()            { glUseProgram(0);                  }

    unsigned int    getProgram()        { return m_uiProgram;               }

    const char*     getErrorMessage()   { return m_strErrorMessage.c_str(); }

private:

    bool            readShaderSource(const char* pFileName, std::string &strSource);
    bool            createShaderFromFile(const char* pFileName, unsigned int uiType);

    unsigned int    m_uiVertexShader;
    unsigned int    m_uiFragmentShader;
    unsigned int    m_uiProgram;

    std::string     m_strErrorMessage;
};