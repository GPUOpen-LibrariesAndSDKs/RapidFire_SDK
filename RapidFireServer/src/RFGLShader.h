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