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

#include "RFError.h"

#include <stdio.h>

#include <sstream>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <OpenCL/cl_gl.h>
#include <OpenCL/cl_d3d11.h>
#elif defined WIN32 || defined _WIN32
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <CL/cl_d3d11.h>
#else
#include <CL/cl.h>
#include <CL/cl_gl.h>
#endif

#include "RapidFire.h"
#include "RFTypes.h"

void rfError(int code, const char* err, const char* file, const int line)
{
#if defined _DEBUG || defined DEBUG

    static RFLogFile g_ErrorFile("RapidFire_Debug_Errors.log");

    std::stringstream oss;

    oss << "RapidFire Error: " << err << " File : " << file << " Line " << line << " code : " << code;

    g_ErrorFile.logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());

#else
    (void)code;
    (void)err;
    (void)file;
    (void)line;
#endif
}

const char* getErrorStringRF(int code)
{
    switch (code)
    {
        case RF_STATUS_FAIL:
            return "RapidFire operation failed";
        case RF_STATUS_MEMORY_FAIL:
            return "Failed to allocate memory";
        case RF_STATUS_RENDER_TARGET_FAIL:
            return "Render target failed";
        case RF_STATUS_OPENGL_FAIL:
            return "OpenGL call failed";
        case RF_STATUS_OPENCL_FAIL:
            return "OpenCL call failed";
        case RF_STATUS_DOPP_FAIL:
            return "DOPP call failed";

        case RF_STATUS_AMF_FAIL:
            return "AMD Media Foundation encoder failed";
        case RF_STATUS_QUEUE_FULL:
            return "Encode queue is full";
        case RF_STATUS_NO_ENCODED_FRAME:
            return "No encoded frame is ready in the encoder";

        case RF_STATUS_PARAM_ACCESS_DENIED:
            return "Access to parameter denied";

        case RF_STATUS_INVALID_SESSION:
            return "Invalid RapidFire session";
        case RF_STATUS_INVALID_CONTEXT:
            return "Invalid RapidFire context";
        case RF_STATUS_INVALID_DIMENSION:
            return "Invalid image dimension";
        case RF_STATUS_INVALID_TEXTURE:
            return "Invalid image texture";
        case RF_STATUS_INVALID_INDEX:
            return "Invalid buffer index";
        case RF_STATUS_INVALID_FORMAT:
            return "Invalid data format for encoder";
        case RF_STATUS_INVALID_CONFIG:
            return "Invalid encoder config";
        case RF_STATUS_INVALID_ENCODER:
            return "Invalid encoder";
        case RF_STATUS_INVALID_RENDER_TARGET:
            return "Invalid render target";
        case RF_STATUS_INVALID_DESKTOP_ID:
            return "Invalid desktop id";
        case RF_STATUS_INVALID_OPENGL_CONTEXT:
            return "Invalid OpenGL context";
        case RF_STATUS_INVALID_D3D_DEVICE:
            return "Invalid Direct3D device";
        case RF_STATUS_INVALID_OPENCL_ENV:
            return "Invalid OpenCL environment";
        case RF_STATUS_INVALID_OPENCL_CONTEXT:
            return "Invalid OpenCL context";
        case RF_STATUS_INVALID_OPENCL_MEMOBJ:
            return "Invalid OpenCL memory object";
        case RF_STATUS_INVALID_SESSION_PROPERTIES:
            return "Session property is not supported or has invalid value";
        case RF_STATUS_INVALID_ENCODER_PARAMETER:
            return "Encoder parameter is not supported or has invalid value";
    }

    static char buf[256];
    sprintf_s(buf, 256, "%d", code);
    return buf;
}


const char* getErrorStringCL(int code)
{
    switch (code)
    {
        case CL_DEVICE_NOT_FOUND:
            return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE:
            return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE:
            return "CL_COMPILER_NOT_AVAILABLE";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case CL_OUT_OF_RESOURCES:
            return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY:
            return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP:
            return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH:
            return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_BUILD_PROGRAM_FAILURE:
            return "CL_BUILD_PROGRAM_FAILURE";
        case CL_MAP_FAILURE:
            return "CL_MAP_FAILURE";
        case CL_MISALIGNED_SUB_BUFFER_OFFSET:
            return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
            return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
        case CL_INVALID_VALUE:
            return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE:
            return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_PLATFORM:
            return "CL_INVALID_PLATFORM";
        case CL_INVALID_DEVICE:
            return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT:
            return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES:
            return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE:
            return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_HOST_PTR:
            return "CL_INVALID_HOST_PTR";
        case CL_INVALID_MEM_OBJECT:
            return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE:
            return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER:
            return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY:
            return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS:
            return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PROGRAM:
            return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE:
            return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME:
            return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION:
            return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL:
            return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX:
            return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE:
            return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE:
            return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS:
            return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION:
            return "CL_INVALID_WORK_DIMENSION";
        case CL_INVALID_WORK_GROUP_SIZE:
            return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE:
            return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET:
            return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST:
            return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT:
            return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION:
            return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT:
            return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE:
            return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL:
            return "CL_INVALID_MIP_LEVEL";
        case CL_INVALID_GLOBAL_WORK_SIZE:
            return "CL_INVALID_GLOBAL_WORK_SIZE";

            // cl_gl
        case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR:
            return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";

#if defined WIN32 || defined _WIN32
            // cl_d3d11
        case CL_INVALID_D3D11_DEVICE_KHR:
            return "CL_INVALID_D3D11_DEVICE_KHR";
        case CL_INVALID_D3D11_RESOURCE_KHR:
            return "CL_INVALID_D3D11_RESOURCE_KHR";
        case CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR:
            return "CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR";
        case CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR:
            return "CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR";
#endif
    }

    static char buf[256];
    sprintf_s(buf, 256, "%d", code);
    return buf;
}


void cleanLogFiles(const std::string& strPath, const std::string& strFilePrefix)
{
    HANDLE              hFind;
    WIN32_FIND_DATA     FindFileData;

    std::stringstream  oss;

    oss << strPath << strFilePrefix << "*.log";

    hFind = FindFirstFile(oss.str().c_str(), &FindFileData);

    if (hFind)
    {
        do
        {
            std::string strFullPath(strPath);

            strFullPath += FindFileData.cFileName;

            remove(strFullPath.c_str());

        } while (FindNextFile(hFind, &FindFileData));

        FindClose(hFind);
    }
}


RFLogFile::RFLogFile(const std::string& strLogFileName)
{
    m_LogFile.open(strLogFileName.c_str(), std::fstream::out);

    if (!m_LogFile.is_open())
    {
        m_LogFile.open(strLogFileName, std::fstream::out);

        if (!m_LogFile.is_open())
        {
            throw std::exception("Failed to open log file");
        }
        else
        {
            m_LogFile << "ERROR: Failed to open log file : " << strLogFileName << " using default location!" << std::endl;
        }
    }

    m_LogFile << "\n-------------------------------------- Starting log --------------------------------------" << std::endl;
}


RFLogFile::~RFLogFile()
{
    if (m_LogFile.is_open())
    {
        m_LogFile << "\n-------------------------------------- Stopping log --------------------------------------" << std::endl;
        m_LogFile.close();
    }
}


void RFLogFile::logMessage(MessageType mt, const std::string& strMsg)
{
    logMessage (mt, strMsg, RF_STATUS_OK);
}


void RFLogFile::logMessage(MessageType mt, const std::string& strMsg, RFStatus err)
{
    if (m_LogFile.is_open())
    {
        SYSTEMTIME  sysTime;

        GetLocalTime(&sysTime);

        m_LogFile << sysTime.wYear << " " << sysTime.wMonth << " " << sysTime.wDay << " " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << " ";

        switch (mt)
        {
            case RF_LOG_INFO:
                m_LogFile << "INFO: ";
                break;

            case RF_LOG_WARNING:
                m_LogFile << "WARNING: ";
                break;

            case RF_LOG_ERROR:
                m_LogFile << "ERROR: ";
                break;
        }

        if (err != RF_STATUS_OK)
        {
            m_LogFile << err << " " << getErrorStringRF(err) << "  ";
        }

        m_LogFile << strMsg << std::endl;
    }
}