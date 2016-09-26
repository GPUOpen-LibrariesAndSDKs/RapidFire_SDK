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

#include <fstream>
#include <sstream>
#include <string>

#include "RapidFire.h"

void rfError(int code, const char* err, const char* file, int line);

const char* getErrorStringRF(int code);
const char* getErrorStringCL(int code);

#define RF_Error(code, msg) rfError(code, msg, __FILE__, __LINE__)
#define RF_Assert(expr) if (!!(expr)) ; else rfError(RF_STATUS_FAIL, #expr, __FILE__, __LINE__)

#define SAFE_CALL_RF(expr)  \
    { \
        RFStatus code = expr; \
        if (code != RF_STATUS_OK) \
        { \
            rfError(code, getErrorStringRF(code), __FILE__, __LINE__); \
            return code; \
        } \
    }

#define SAFE_CALL_CL(expr)  \
    { \
        int code = expr; \
        if (code != CL_SUCCESS) \
        { \
            rfError(code, getErrorStringCL(code), __FILE__, __LINE__); \
            return RF_STATUS_OPENCL_FAIL; \
        } \
    }

extern void cleanLogFiles(const std::string& strPath, const std::string& strFilePrefix);

class RFLogFile
{
public:

    enum MessageType { RF_LOG_INFO = 0, RF_LOG_WARNING = 1, RF_LOG_ERROR = 2 };

    explicit RFLogFile(const std::string& strLogFileName);
    ~RFLogFile();

    void logMessage(MessageType mtype, const std::string& strMessage);
    void logMessage(MessageType mtype, const std::string& strMessage, RFStatus err);

private:

    std::fstream m_LogFile;
};