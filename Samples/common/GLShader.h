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