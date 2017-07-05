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

#include "RFUtils.h"

#include <limits.h>

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif

float Timer::s_clockTime = 0.0;

Timer::Timer()
{
    if (s_clockTime == 0.0)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);

        s_clockTime = 1.0f / freq.QuadPart;
    }

    reset();
}

void Timer::reset()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    m_startTime = time.QuadPart;
}

float Timer::getTime()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);

    return s_clockTime * (time.QuadPart - m_startTime);
}

#if defined WIN32 || defined _WIN32

#ifdef _DEBUG

// Dumps rgba images that can be viewed with GIMP.
extern void writeRGBAimage(unsigned char* outbuf, int xsize, int ysize, const char* fname);

// Dumps argb images to rgba files that can be viewed with GIMP.
extern void writeARGBimage(unsigned char* outbuf, int xsize, int ysize, const char* fname);

// Dumps argb images to rgba files that can be viewed with GIMP.
extern void writeBGRAimage(unsigned char* outbuf, int xsize, int ysize, const char* fname);

extern void writeNV12Image(unsigned char* pPixel, unsigned int w, unsigned int h, const char* fname);

extern void writeYUVImage(unsigned char* pPixel, unsigned int w, unsigned int h, const char* fname);

#endif

std::string utilGetExecutablePath()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
#ifdef UNICODE
    if (!GetModuleFileName(NULL, (LPWCH)buffer, sizeof(buffer)))
    {
        throw std::string("GetModuleFileName() failed!");
    }
#else
    if (!GetModuleFileName(NULL, buffer, sizeof(buffer)))
    {
        throw std::string("GetModuleFileName() failed!");
    }
#endif
    std::string str(buffer);

    size_t last = str.find_last_of('\\');
#else // ifdef _WIN32
    char buffer[PATH_MAX + 1];
    size_t len;
    if ((len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1)) == -1)
    {
        throw std::string("readlink() failed!");
    }
    buffer[len] = '\0';
    std::string str(buffer);
    size_t last = str.find_last_of('/');
#endif // ifdef _WIN32
    return str.substr(0, last + 1);
}


unsigned int utilAlignNum(unsigned int width, unsigned int ALIGNMENT)
{
    if (ALIGNMENT == 0)
    {
        return width;
    }

    return (width + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}


bool utilIsFpsValid(size_t fps)
{
    return (fps > 0) && (fps < 1000);
}


bool utilIsBitrateValid(size_t bitrate)
{
    return (bitrate >= 10000) && (bitrate <= 1000000000);
}


bool utilIsPropertyValid(size_t Property)
{
    return (Property > 0) && (Property < UINT_MAX);
}


#ifdef _DEBUG

void dumpCLBuffer(cl_mem clBuffer, RFContextCL* pContext, unsigned int uiWidth, unsigned int uiHeight, RFFormat rfFormat, const char* pFileName)
{
    cl_int       nStatus;
    unsigned int uiBufferSize = 0;

    switch (rfFormat)
    {
    case RF_NV12:
        uiBufferSize = (uiWidth * uiHeight) + (uiWidth * uiHeight) / 2;
        break;

    case RF_RGBA8:
    case RF_ARGB8:
    case RF_BGRA8:
        uiBufferSize = uiWidth * uiHeight * 4;
        break;
    }

    cl_mem              clImageBuffer = NULL;
    cl_mem_object_type  clMemType;

    nStatus = clGetMemObjectInfo(clBuffer, CL_MEM_TYPE, sizeof(cl_mem_object_type), &clMemType, nullptr);

    if (uiBufferSize > 0 && nStatus == CL_SUCCESS)
    {
        if (clMemType == CL_MEM_OBJECT_IMAGE2D)
        {
            // In case of an image we need to copy it into a regular buffer.
            clImageBuffer = clCreateBuffer(pContext->getContext(), CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, uiBufferSize, nullptr, &nStatus);

            if (nStatus == CL_SUCCESS)
            {
                size_t origin[3] = { 0, 0, 0 };
                size_t region[3] = { uiWidth, uiHeight, 1 };

                nStatus = clEnqueueCopyImageToBuffer(pContext->getCmdQueue(), clBuffer, clImageBuffer, origin, region, 0, 0, nullptr, nullptr);
            }
        }
        else
        {
            clImageBuffer = clBuffer;
        }

        char* pTmp = static_cast<char*>(clEnqueueMapBuffer(pContext->getCmdQueue(), clImageBuffer, CL_TRUE, CL_MAP_READ, 0, uiBufferSize, 0, nullptr, nullptr, &nStatus));

        if (nStatus == CL_SUCCESS && pTmp != nullptr)
        {
            if (rfFormat == RF_RGBA8)
            {
                writeRGBAimage(reinterpret_cast<unsigned char*>(pTmp), uiWidth, uiHeight, pFileName);
            }
            else if (rfFormat == RF_ARGB8)
            {
                writeARGBimage(reinterpret_cast<unsigned char*>(pTmp), uiWidth, uiHeight, pFileName);
            }
            else if (rfFormat == RF_BGRA8)
            {
                writeBGRAimage(reinterpret_cast<unsigned char*>(pTmp), uiWidth, uiHeight, pFileName);
            }
            else if (rfFormat == RF_NV12)
            {
                writeNV12Image(reinterpret_cast<unsigned char*>(pTmp), uiWidth, uiHeight, pFileName);
            }

            clEnqueueUnmapMemObject(pContext->getCmdQueue(), clImageBuffer, pTmp, 0, nullptr, nullptr);
        }

        if (clMemType == CL_MEM_OBJECT_IMAGE2D)
        {
            clReleaseMemObject(clImageBuffer);
        }
    }
}

#endif // _DEBUG

#endif // if defined WIN32 || defined _WIN32