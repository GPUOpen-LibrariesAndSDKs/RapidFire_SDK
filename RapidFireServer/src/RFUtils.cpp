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
#include "RFUtils.h"

#include <limits.h>

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif

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

    case RF_I420:
        uiBufferSize = uiWidth * uiHeight * 2;
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
            else if (rfFormat == RF_I420)
            {
                writeYUVImage(reinterpret_cast<unsigned char*>(pTmp), uiWidth, uiHeight, pFileName);
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