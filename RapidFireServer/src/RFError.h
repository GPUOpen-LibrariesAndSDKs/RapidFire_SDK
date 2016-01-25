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

#include <fstream>
#include <sstream>
#include <string>

void rfError(int code, const char* err, const char* file, int line);

const char* getErrorStringRF(int code);
const char* getErrorStringCL(int code);

#define RF_Error(code, msg) rfError(code, msg, __FILE__, __LINE__)
#define RF_Assert(expr) if (!!(expr)) ; else rfError(RF_STATUS_FAIL, #expr, __FILE__, __LINE__)

#define SAFE_CALL_RF(expr)  \
    { \
        int code = expr; \
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

    void logMessage(MessageType mtype, const std::string& strMessage, int err = 0);

private:

    std::fstream m_LogFile;
};