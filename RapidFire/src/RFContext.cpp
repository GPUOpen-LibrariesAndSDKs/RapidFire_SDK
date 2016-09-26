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

#include "RFContext.h"

#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <CL/cl_d3d11.h>
#include <CL/cl_dx9_media_sharing.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>

#include "RFError.h"
#include "RFUtils.h"

#define clGetGLContextInfoKHR               clGetGLContextInfoKHR_proc
static clGetGLContextInfoKHR_fn             clGetGLContextInfoKHR = NULL;

// D3D11 and OpenCL interop extension functions
static clGetDeviceIDsFromD3D11KHR_fn        pfn_clGetDeviceIDsFromD3D11KHR = NULL;
static clCreateFromD3D11BufferKHR_fn        pfn_clCreateFromD3D11BufferKHR = NULL;
static clCreateFromD3D11Texture2DKHR_fn     pfn_clCreateFromD3D11Texture2DKHR = NULL;
static clEnqueueAcquireD3D11ObjectsKHR_fn   pfn_clEnqueueAcquireD3D11ObjectsKHR = NULL;
static clEnqueueReleaseD3D11ObjectsKHR_fn   pfn_clEnqueueReleaseD3D11ObjectsKHR = NULL;

// D3D9 media share function
static clGetDeviceIDsFromDX9MediaAdapterKHR_fn pfn_clGetDeviceIDsFromDX9MediaAdapterKHR = NULL;
static clCreateFromDX9MediaSurfaceKHR_fn       pfn_clCreateFromDX9MediaSurfaceKHR = NULL;
static clEnqueueAcquireDX9MediaSurfacesKHR_fn  pfn_clEnqueueAcquireDX9MediaSurfacesKHR = NULL;
static clEnqueueReleaseDX9MediaSurfacesKHR_fn  pfn_clEnqueueReleaseDX9MediaSurfacesKHR = NULL;


#define INIT_CL_EXT_FCN_PTR(platform_id, name) \
    if (!pfn_ ## name) { \
        pfn_ ## name = (name ## _fn) \
            clGetExtensionFunctionAddressForPlatform(platform_id, #name); \
        if (!pfn_ ## name) { \
            RF_Error(RF_STATUS_OPENCL_FAIL, "Cannot get pointer to ext. fcn. "); \
            return NULL; \
        } \
    }

#define CSC_KERNEL_FILE_NAME    "rfkernels.cl"

using namespace std;

// str_cl_kernels is defined in RFKernelCL.cpp and contains the kernel sources.
extern const char* str_cl_kernels;

class CLPlatform
{
public:

    const cl_platform_id id;

    static const CLPlatform& getInstance()
    {
        /* all sessions share the same platform. */
        static const CLPlatform m_clPlatform;

        return m_clPlatform;
    }

private:

    CLPlatform()
        : id(getPlatform("Advanced Micro Devices, Inc.", 12))
    {}

    cl_platform_id CLPlatform::getPlatform(const std::string strPlatformName, unsigned int uiVersion)
    {
        ////////////////////////////////////////////////////////////////////
        // Get Platform IDs, and search for AMD platform.
        ////////////////////////////////////////////////////////////////////
        cl_uint uiNumPlatforms;
        cl_platform_id* pPlatform = nullptr;

        cl_platform_id pSelectedPlatform = NULL;

        // Get number of platforms.
        if (clGetPlatformIDs(0, nullptr, &uiNumPlatforms) != CL_SUCCESS)
        {
            return NULL;
        }

        if (uiNumPlatforms <= 0)
        {
            return NULL;
        }

        pPlatform = new (nothrow)cl_platform_id[uiNumPlatforms];
        if (!pPlatform)
        {
            return NULL;
        }

        if (clGetPlatformIDs(uiNumPlatforms, pPlatform, nullptr) != CL_SUCCESS)
        {
            delete[] pPlatform;
            return NULL;
        }

        // Loop through platforms.
        for (unsigned int i = 0; i < uiNumPlatforms; ++i)
        {
            char pBuffer[128];

            if (clGetPlatformInfo(pPlatform[i], CL_PLATFORM_VENDOR, 128, pBuffer, nullptr) == CL_SUCCESS)
            {
                if (strPlatformName.find(pBuffer, 0, strPlatformName.size()) == 0)
                {
                    if (clGetPlatformInfo(pPlatform[i], CL_PLATFORM_VERSION, 128, pBuffer, nullptr) == CL_SUCCESS)
                    {
                        std::string strOCLVersion(pBuffer);

                        size_t dotPos = strOCLVersion.find('.');

                        if (dotPos != std::string::npos && dotPos < (strOCLVersion.size() - 1) && dotPos > 0)
                        {
                            std::stringstream convert(strOCLVersion.substr(dotPos - 1, dotPos + 1));

                            unsigned int uiMajor = 0;
                            unsigned int uiMinor = 0;

                            convert >> uiMajor;
                            convert.seekg(2);
                            convert >> uiMinor;

                            if (uiMajor * 10 + uiMinor >= uiVersion)
                            {
                                pSelectedPlatform = pPlatform[i];
                                break;
                            }
                        }
                    }
                }
            }
        }

        delete[] pPlatform;

        if (!pSelectedPlatform)
        {
            return NULL;
        }

        // Init extension function pointers
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clCreateFromD3D11BufferKHR);
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clGetDeviceIDsFromD3D11KHR);
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clCreateFromD3D11Texture2DKHR);
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clEnqueueAcquireD3D11ObjectsKHR);
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clEnqueueReleaseD3D11ObjectsKHR);
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clGetDeviceIDsFromDX9MediaAdapterKHR);
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clCreateFromDX9MediaSurfaceKHR);
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clEnqueueAcquireDX9MediaSurfacesKHR);
        INIT_CL_EXT_FCN_PTR(pSelectedPlatform, clEnqueueReleaseDX9MediaSurfacesKHR);

        return pSelectedPlatform;
    }
};


/////////////////////////////////////////////////////////////////////
// RFCLEvent
// This class is used to track if a cl_event is blocked or not.
// It will ensure that the event is only released if required.
/////////////////////////////////////////////////////////////////////
RFEventCL::RFEventCL()
    : m_clEvent(NULL)
    , m_bReleased(true)
{}


RFEventCL::~RFEventCL()
{
    if (!m_bReleased)
    {
        clReleaseEvent(m_clEvent);
    }
}

void RFEventCL::wait()
{
    if (!m_bReleased)
    {
        clWaitForEvents(1, &m_clEvent);
        clReleaseEvent(m_clEvent);

        m_bReleased = true;
    }
}

void RFEventCL::release()
{
    if (!m_bReleased)
    {
        clReleaseEvent(m_clEvent);

        m_bReleased = true;
    }
}

cl_event* RFEventCL::operator&()
{
    m_bReleased = false;

    return &m_clEvent;
}


//////////////////////////////////////////////////////////
// Native CL context for CSC
//////////////////////////////////////////////////////////
RFContextCL::RFContextCL()
    : m_uiNumResultBuffers(NUM_RESULT_BUFFERS)
    , m_bValid(false)
    , m_bUseAsyncCopy(false)
    , m_uiOutputWidth(0)
    , m_uiOutputHeight(0)
    , m_uiAlignedOutputWidth(0)
    , m_uiAlignedOutputHeight(0)
    , m_nOutputBufferSize(0)
    , m_uiInputWidth(0)
    , m_uiInputHeight(0)
    , m_uiNumRegisteredRT(0)
    , m_uiPendingTransfers(0)
    , m_WaveFrontSize(64)
    , m_clPlatformId(NULL)
    , m_clDevId(NULL)
    , m_clCtx(NULL)
    , m_clCscProgram(NULL)
    , m_clCmdQueue(NULL)
    , m_clDMAQueue(NULL)
    , m_CtxType(RF_CTX_UNKNOWN)
    , m_TargetFormat(RF_FORMAT_UNKNOWN)
    , m_uiCSCKernelIdx(RF_KERNEL_UNKNOWN)
    , m_fnAcquireMemObj(NULL)
    , m_fnReleaseMemObj(NULL)
{
    memset(m_CSCKernels, 0, RF_KERNEL_NUMBER * sizeof(CSC_KERNEL));

    memset(m_clInputImage, 0, MAX_NUM_RENDER_TARGETS * sizeof(cl_mem));

    for (int i = 0; i < NUM_RESULT_BUFFERS; ++i)
    {
        m_rtState[i] = RF_STATE_INVALID;
        m_clResultBuffer[i] = NULL;
        m_clPageLockedBuffer[i] = NULL;
        m_pSysmemBuffer[i] = nullptr;
    }

    m_clPlatformId = CLPlatform::getInstance().id;

    if (m_clPlatformId == NULL)
    {
        throw std::runtime_error("No AMD platform");
    }

    // get version info of RapidFire.dll
    memset(m_dwVersion, 0, sizeof(DWORD) * 4);

    size_t const    maxlength = 512;

#ifdef _WIN64
    HMODULE         hModule = GetModuleHandle("RapidFire64.dll");
#else
    HMODULE         hModule = GetModuleHandle("RapidFire.dll");
#endif

    if (!hModule)
    {
        return;
    }

    char pModulepath[maxlength];

    DWORD dwRes = GetModuleFileName(hModule, pModulepath, maxlength);

    if (dwRes == 0 || (dwRes == maxlength && GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        return;
    }

    DWORD infoSize = GetFileVersionInfoSize(pModulepath, NULL);
    if (infoSize > 0)
    {
        void* pData = alloca(infoSize);
        if (GetFileVersionInfo(pModulepath, NULL, infoSize, pData))
        {
            VS_FIXEDFILEINFO* pFileInfo;
            unsigned int uiInfoSize;
            if (VerQueryValue(pData, TEXT("\\"), reinterpret_cast<LPVOID*>(&pFileInfo), &uiInfoSize) && uiInfoSize == sizeof(VS_FIXEDFILEINFO))
            {
                m_dwVersion[0] = HIWORD(pFileInfo->dwFileVersionMS);
                m_dwVersion[1] = LOWORD(pFileInfo->dwFileVersionMS);
                m_dwVersion[2] = HIWORD(pFileInfo->dwFileVersionLS);
                m_dwVersion[3] = LOWORD(pFileInfo->dwFileVersionLS);
            }
        }
    }
}


RFContextCL::~RFContextCL()
{
    cl_int nStatus;

    if (m_clCmdQueue)
    {
        clFinish(m_clCmdQueue);
    }

    if (m_clDMAQueue)
    {
        clFinish(m_clDMAQueue);
    }

    for (unsigned int i = 0; i < RF_KERNEL_NUMBER; ++i)
    {
        if (m_CSCKernels[i].kernel)
        {
            nStatus = clReleaseKernel(m_CSCKernels[i].kernel);
        }
    }

    memset(m_CSCKernels, 0, RF_KERNEL_NUMBER * sizeof(CSC_KERNEL));

    if (m_clCscProgram)
    {
        nStatus = clReleaseProgram(m_clCscProgram);
    }

    deleteBuffers();

    if (m_clCmdQueue)
    {
        clReleaseCommandQueue(m_clCmdQueue);
    }

    if (m_clDMAQueue)
    {
        clReleaseCommandQueue(m_clDMAQueue);
    }

    if (m_clCtx)
    {
        clReleaseContext(m_clCtx);
    }
}


////////////////////////////////////////////////////////////////////
// Create OpenCL context.
////////////////////////////////////////////////////////////////////
RFStatus RFContextCL::createContext()
{
    unsigned int uiNumDevices = 0;

    SAFE_CALL_CL(clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_GPU, 0, nullptr, &uiNumDevices));
    if (uiNumDevices == 0)
    {
        RF_Error(RF_STATUS_OPENCL_FAIL, "OpenCL GPU device is not found");
        return RF_STATUS_OPENCL_FAIL;
    }

    cl_device_id* pDevices = new (nothrow)cl_device_id[uiNumDevices];
    if (!pDevices)
    {
        return RF_STATUS_MEMORY_FAIL;
    }

    SAFE_CALL_CL(clGetDeviceIDs(m_clPlatformId, CL_DEVICE_TYPE_GPU, uiNumDevices, pDevices, nullptr));

    // Simply take first matching device.
    m_clDevId = pDevices[0];

    delete[] pDevices;

    cl_context_properties pProperties[] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(m_clPlatformId),
                                            0 };

    return finalizeContext(pProperties);
}


////////////////////////////////////////////////////////////////////
// Create OpenCL context based on an OpenGL context
////////////////////////////////////////////////////////////////////
RFStatus RFContextCL::createContext(DeviceCtx hDC, GraphicsCtx hGLRC)
{
    cl_int nStatus;

    if (!m_clPlatformId)
    {
        return RF_STATUS_OPENCL_FAIL;
    }

    // Check if we already have a valid context.
    if (m_bValid)
    {
        return RF_STATUS_FAIL;
    }

    // Create OpenCL context that shares resources with OpenGL.
    if (!clGetGLContextInfoKHR)
    {
        clGetGLContextInfoKHR = static_cast<clGetGLContextInfoKHR_fn>(clGetExtensionFunctionAddressForPlatform(m_clPlatformId, "clGetGLContextInfoKHR"));

        if (!clGetGLContextInfoKHR)
        {
            RF_Error(RF_STATUS_OPENCL_FAIL, "clGetExtensionFunctionAddressForPlatform failed");
            return RF_STATUS_OPENCL_FAIL;
        }
    }

    cl_context_properties pProperties[] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(m_clPlatformId),
                                            CL_GL_CONTEXT_KHR,   reinterpret_cast<cl_context_properties>(hGLRC),
                                            CL_WGL_HDC_KHR,      reinterpret_cast<cl_context_properties>(hDC),
                                            0 };

    size_t uiNumDevices = 0;
    SAFE_CALL_CL(clGetGLContextInfoKHR(pProperties, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, 0, nullptr, &uiNumDevices));

    if (uiNumDevices == 0)
    {
        RF_Error(RF_STATUS_OPENCL_FAIL, "clGetGLContextInfoKHR failed");
        return RF_STATUS_OPENCL_FAIL;
    }

    // Get the device id of the OpenCL device that can be used to share data with OpenGL.
    nStatus = clGetGLContextInfoKHR(pProperties, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &m_clDevId, nullptr);
    SAFE_CALL_CL(nStatus);

    m_CtxType = RF_CTX_FROM_GL;

    return finalizeContext(pProperties);
}


////////////////////////////////////////////////////////////////////
// Create OpenCL context based on DX11 device.
////////////////////////////////////////////////////////////////////
RFStatus RFContextCL::createContext(ID3D11Device* pD3D11Device)
{
    if (m_bValid || !pD3D11Device)
    {
        return RF_STATUS_FAIL;
    }

    // Number of OpenCL devices corresponding to the Direct3D 11 object
    cl_uint numDevices;

    SAFE_CALL_CL(pfn_clGetDeviceIDsFromD3D11KHR(m_clPlatformId, CL_D3D11_DEVICE_KHR, static_cast<void*>(pD3D11Device), CL_PREFERRED_DEVICES_FOR_D3D11_KHR,
                 0, nullptr, &numDevices));

    if (numDevices == 0)
    {
        RF_Error(RF_STATUS_OPENCL_FAIL, "None OpenCL device can be created from the Direct3D 11 object");
        return RF_STATUS_OPENCL_FAIL;
    }

    // Get all the OpenCL devices corresponding to the Direct3D object.
    cl_device_id* openCLInteropDevices = new (nothrow)cl_device_id[numDevices];
    if (!openCLInteropDevices)
    {
        return RF_STATUS_MEMORY_FAIL;
    }

    SAFE_CALL_CL(pfn_clGetDeviceIDsFromD3D11KHR(m_clPlatformId, CL_D3D11_DEVICE_KHR, static_cast<void*>(pD3D11Device), CL_PREFERRED_DEVICES_FOR_D3D11_KHR,
                 numDevices, openCLInteropDevices, nullptr));

    m_clDevId = openCLInteropDevices[0];

    delete[] openCLInteropDevices;

    // Create OpenCL context from ID3D11Device.
    cl_context_properties cps[] = { CL_CONTEXT_PLATFORM,         reinterpret_cast<cl_context_properties>(m_clPlatformId),
                                    CL_CONTEXT_D3D11_DEVICE_KHR, reinterpret_cast<cl_context_properties>(pD3D11Device),
                                    0 };

    m_CtxType = RF_CTX_FROM_DX11;

    return finalizeContext(cps);
}


////////////////////////////////////////////////////////////////////
// Create OpenCL context based on DX9 device.
////////////////////////////////////////////////////////////////////
RFStatus RFContextCL::createContext(IDirect3DDevice9* pD3D9Device)
{
    if (m_bValid || !pD3D9Device)
    {
        return RF_STATUS_FAIL;
    }

    // Number of OpenCL devices corresponding to the Direct3D 9 object
    cl_uint numDevices;
    cl_dx9_media_adapter_type_khr tpAdapter = CL_ADAPTER_D3D9_KHR;

    SAFE_CALL_CL(pfn_clGetDeviceIDsFromDX9MediaAdapterKHR(m_clPlatformId, 3, &tpAdapter, static_cast<void*>(pD3D9Device), CL_PREFERRED_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR,
                 0, nullptr, &numDevices));

    if (numDevices == 0)
    {
        RF_Error(RF_STATUS_OPENCL_FAIL, "None OpenCL device can be created from the Direct3D 9 object");
        return RF_STATUS_OPENCL_FAIL;
    }

    // Get all the OpenCL devices corresponding to the Direct3D object.
    cl_device_id* openCLInteropDevices = new (nothrow)cl_device_id[numDevices];
    if (!openCLInteropDevices)
    {
        return RF_STATUS_MEMORY_FAIL;
    }

    SAFE_CALL_CL(pfn_clGetDeviceIDsFromDX9MediaAdapterKHR(m_clPlatformId, 3, &tpAdapter, static_cast<void*>(pD3D9Device), CL_PREFERRED_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR,
                 numDevices, openCLInteropDevices, nullptr));

    m_clDevId = openCLInteropDevices[0];

    delete[] openCLInteropDevices;

    // Create OpenCL context from ID3D9 Device.
    cl_context_properties cps[] = { CL_CONTEXT_PLATFORM,         reinterpret_cast<cl_context_properties>(m_clPlatformId),
                                    CL_CONTEXT_ADAPTER_D3D9_KHR, reinterpret_cast<cl_context_properties>(pD3D9Device),
                                    0 };

    m_CtxType = RF_CTX_FROM_DX9;

    return finalizeContext(cps);
}


////////////////////////////////////////////////////////////////////
// Create OpenCL context based on DX9 Ex device.
////////////////////////////////////////////////////////////////////
RFStatus RFContextCL::createContext(IDirect3DDevice9Ex* pD3D9ExDevice)
{
    if (m_bValid || !pD3D9ExDevice)
    {
        return RF_STATUS_FAIL;
    }

    // Number of OpenCL devices corresponding to the Direct3D 9 object
    cl_uint numDevices;
    cl_dx9_media_adapter_type_khr tpAdapter = CL_ADAPTER_D3D9EX_KHR;

    SAFE_CALL_CL(pfn_clGetDeviceIDsFromDX9MediaAdapterKHR(m_clPlatformId, 3, &tpAdapter, static_cast<void*>(pD3D9ExDevice), CL_PREFERRED_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR,
                 0, nullptr, &numDevices));

    if (numDevices == 0)
    {
        RF_Error(RF_STATUS_OPENCL_FAIL, "None OpenCL device can be created from the Direct3D 9 Ex object");
        return RF_STATUS_OPENCL_FAIL;
    }

    // Get all the OpenCL devices corresponding to the Direct3D object.
    cl_device_id *openCLInteropDevices = new (nothrow)cl_device_id[numDevices];
    if (!openCLInteropDevices)
    {
        return RF_STATUS_MEMORY_FAIL;
    }

    SAFE_CALL_CL(pfn_clGetDeviceIDsFromDX9MediaAdapterKHR(m_clPlatformId, 3, &tpAdapter, static_cast<void*>(pD3D9ExDevice), CL_PREFERRED_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR,
                 numDevices, openCLInteropDevices, nullptr));

    m_clDevId = openCLInteropDevices[0];

    delete[] openCLInteropDevices;

    // Create OpenCL context from ID3D9 Device.
    cl_context_properties cps[] = { CL_CONTEXT_PLATFORM,           reinterpret_cast<cl_context_properties>(m_clPlatformId),
                                    CL_CONTEXT_ADAPTER_D3D9EX_KHR, reinterpret_cast<cl_context_properties>(pD3D9ExDevice),
                                    0 };

    m_CtxType = RF_CTX_FROM_DX9EX;

    return finalizeContext(cps);
}


RFStatus RFContextCL::finalizeContext(const cl_context_properties* pContextProperties)
{
    cl_int nStatus = 0;

    if (!pContextProperties)
    {
        return RF_STATUS_INVALID_OPENCL_ENV;
    }

    m_clCtx = clCreateContext(pContextProperties, 1, &m_clDevId, nullptr, nullptr, &nStatus);
    SAFE_CALL_CL(nStatus);

    m_clCmdQueue = clCreateCommandQueue(m_clCtx, m_clDevId, 0, &nStatus);
    SAFE_CALL_CL(nStatus);

    m_clDMAQueue = clCreateCommandQueue(m_clCtx, m_clDevId, 0, &nStatus);
    SAFE_CALL_CL(nStatus);

    cl_device_type DeviceType = 0;

    nStatus = clGetDeviceInfo(m_clDevId, CL_DEVICE_TYPE, sizeof(cl_device_type), &DeviceType, nullptr);

    if ((nStatus == CL_SUCCESS) && (DeviceType & CL_DEVICE_TYPE_GPU))
    {
        nStatus = clGetDeviceInfo(m_clDevId, CL_DEVICE_WAVEFRONT_WIDTH_AMD, sizeof(size_t), &m_WaveFrontSize, nullptr);
    }

    // Create CSC kernel.
    SAFE_CALL_CL(setupKernel());

    // Indicate that we have now a valid context.
    m_bValid = true;

    setMemAccessFunction();

    return RF_STATUS_OK;
}


void RFContextCL::setMemAccessFunction()
{
    switch (m_CtxType)
    {
        case RF_CTX_FROM_GL:
            m_fnAcquireMemObj = clEnqueueAcquireGLObjects;
            m_fnReleaseMemObj = clEnqueueReleaseGLObjects;
            break;
        case RF_CTX_FROM_DX11:
            m_fnAcquireMemObj = pfn_clEnqueueAcquireD3D11ObjectsKHR;
            m_fnReleaseMemObj = pfn_clEnqueueReleaseD3D11ObjectsKHR;
            break;
        case RF_CTX_FROM_DX9:
        case RF_CTX_FROM_DX9EX:
            m_fnAcquireMemObj = pfn_clEnqueueAcquireDX9MediaSurfacesKHR;
            m_fnReleaseMemObj = pfn_clEnqueueReleaseDX9MediaSurfacesKHR;
            break;
        default:
            m_fnAcquireMemObj = NULL;
            m_fnReleaseMemObj = NULL;
            break;
    }
}


////////////////////////////////////////////////////////////////////
// Set OpenGL input texture
////////////////////////////////////////////////////////////////////
RFStatus RFContextCL::setInputTexture(const unsigned int uiTextureName, const unsigned int uiWidth, const unsigned int uiHeight, unsigned int& idx)
{
    unsigned int index;

    idx = 0xFF;

    if (m_CtxType != RF_CTX_FROM_GL)
    {
        return RF_STATUS_INVALID_TEXTURE;
    }

    if (!validateDimensions(uiWidth, uiHeight))
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    // Get index of a free slot.
    if (!getFreeRenderTargetIndex(index))
    {
        return RF_STATUS_RENDER_TARGET_FAIL;
    }

    cl_int nStatus;

    m_clInputImage[index] = clCreateFromGLTexture(m_clCtx, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, uiTextureName, &nStatus);
    SAFE_CALL_CL(nStatus);

    size_t w, h;

    nStatus = clGetImageInfo(m_clInputImage[index], CL_IMAGE_WIDTH, sizeof(size_t), &w, nullptr);
    SAFE_CALL_CL(nStatus);

    nStatus = clGetImageInfo(m_clInputImage[index], CL_IMAGE_HEIGHT, sizeof(size_t), &h, nullptr);
    SAFE_CALL_CL(nStatus);

    if (w != uiWidth || h != uiHeight)
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    m_rtState[index] = RF_STATE_FREE;

    idx = index;

    m_uiNumRegisteredRT++;

    m_uiInputWidth = uiWidth;
    m_uiInputHeight = uiHeight;

    return RF_STATUS_OK;
}


////////////////////////////////////////////////////////////////////
// Set D3D 11 input texture
////////////////////////////////////////////////////////////////////
RFStatus RFContextCL::setInputTexture(ID3D11Texture2D* pD3D11Texture, const unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx)
{
    if (m_CtxType != RF_CTX_FROM_DX11 || !pD3D11Texture)
    {
        return RF_STATUS_INVALID_TEXTURE;
    }

    if (!validateDimensions(uiWidth, uiHeight))
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    unsigned int index;

    idx = 0xFF;

    // Get index of a free slot.
    if (!getFreeRenderTargetIndex(index))
    {
        return RF_STATUS_RENDER_TARGET_FAIL;
    }

    // Create OpenCL Image2D based on D3D11 texture.
    cl_int       nStatus;
    unsigned int subresource = 0;

    m_clInputImage[index] = pfn_clCreateFromD3D11Texture2DKHR(m_clCtx, CL_MEM_READ_ONLY, pD3D11Texture, subresource, &nStatus);
    SAFE_CALL_CL(nStatus);

    size_t w, h;

    nStatus = clGetImageInfo(m_clInputImage[index], CL_IMAGE_WIDTH, sizeof(size_t), &w, nullptr);
    SAFE_CALL_CL(nStatus);

    nStatus = clGetImageInfo(m_clInputImage[index], CL_IMAGE_HEIGHT, sizeof(size_t), &h, nullptr);
    SAFE_CALL_CL(nStatus);

    if (w != uiWidth || h != uiHeight)
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    m_rtState[index] = RF_STATE_FREE;

    idx = index;

    ++m_uiNumRegisteredRT;

    m_uiInputWidth = uiWidth;
    m_uiInputHeight = uiHeight;

    return RF_STATUS_OK;
}


////////////////////////////////////////////////////////////////////
// Set D3D 9 input texture.
////////////////////////////////////////////////////////////////////
RFStatus RFContextCL::setInputTexture(IDirect3DSurface9* pD3D9Texture, const unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx)
{
    // Limit to DX9Ex only since OCL has no interop with DX9.
    if (m_CtxType != RF_CTX_FROM_DX9EX || !pD3D9Texture)
    {
        return RF_STATUS_INVALID_TEXTURE;
    }

    if (!validateDimensions(uiWidth, uiHeight))
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    unsigned int index;

    idx = 0xFF;

    // Get index of a free slot.
    if (!getFreeRenderTargetIndex(index))
    {
        return RF_STATUS_RENDER_TARGET_FAIL;
    }

    typedef struct cl_dx9_surface_info_khr
    {
        IDirect3DSurface9* resource;
        HANDLE shared_handle;
    } cl_dx9_surface_info_khr;

    cl_dx9_surface_info_khr surface = { nullptr, NULL };
    surface.resource = pD3D9Texture;

    // Create an OpenCL Image2D based on the D3D9 texture.
    cl_int nStatus;

    m_clInputImage[index] = pfn_clCreateFromDX9MediaSurfaceKHR(m_clCtx, CL_MEM_READ_WRITE, CL_ADAPTER_D3D9EX_KHR, &surface, 0, &nStatus);
    SAFE_CALL_CL(nStatus);

    size_t w, h;

    nStatus = clGetImageInfo(m_clInputImage[index], CL_IMAGE_WIDTH, sizeof(size_t), &w, nullptr);
    SAFE_CALL_CL(nStatus);

    nStatus = clGetImageInfo(m_clInputImage[index], CL_IMAGE_HEIGHT, sizeof(size_t), &h, nullptr);
    SAFE_CALL_CL(nStatus);

    if (w != uiWidth || h != uiHeight)
    {
        return RF_STATUS_INVALID_DIMENSION;
    }

    m_uiInputWidth = uiWidth;
    m_uiInputHeight = uiHeight;

    m_rtState[index] = RF_STATE_FREE;

    idx = index;

    ++m_uiNumRegisteredRT;

    m_uiInputWidth = uiWidth;
    m_uiInputHeight = uiHeight;

    return RF_STATUS_OK;
}


RFStatus RFContextCL::createBuffers(RFFormat format, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiAlignedWidth, unsigned int uiAlignedHeight, bool bUseAsyncCopy)
{
    cl_int nStatus;

    if (!m_bValid)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    if (!validateDimensions(uiWidth, uiHeight))
    {
        // Scaling is not supported.
        return RF_STATUS_INVALID_DIMENSION;
    }

    m_uiOutputWidth         = uiWidth;
    m_uiOutputHeight        = uiHeight;
    m_uiAlignedOutputWidth  = uiAlignedWidth;
    m_uiAlignedOutputHeight = uiAlignedHeight;

    m_bUseAsyncCopy = bUseAsyncCopy;

    switch (format)
    {
        case RF_NV12:
            m_uiCSCKernelIdx = RF_KERNEL_RGBA_TO_NV12;
            // NV12 width * height * 1 Byte for the Y plane + width * height / 2 for the UV interleaved plane (CbCr)
            m_nOutputBufferSize = (m_uiAlignedOutputWidth * m_uiAlignedOutputHeight) + (m_uiAlignedOutputWidth * m_uiAlignedOutputHeight) / 2;
            break;

        case RF_RGBA8:
        case RF_ARGB8:
        case RF_BGRA8:
            m_uiCSCKernelIdx = RF_KERNEL_RGBA_COPY;
            m_nOutputBufferSize = m_uiAlignedOutputWidth * m_uiAlignedOutputHeight * 4;
            break;

        default:
            return RF_STATUS_INVALID_FORMAT;
    }

    // Store target format. The CSC Context will convert the registered texture into this format.
    m_TargetFormat = format;

    // Create NUM_RESULT_BUFFERS OpenCL buffers that will contain the YUV coded colors.
    // An OpenCL kernel will convert color values from m_clImageBufferRGBA to YUV buffers.
    for (int i = 0; i < NUM_RESULT_BUFFERS; ++i)
    {
        // Create a pinned OpenCL buffer which is used to copy data back from the GPU to sys mem.
        m_clPageLockedBuffer[i] = clCreateBuffer(m_clCtx, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, m_nOutputBufferSize, nullptr, &nStatus);
        if (nStatus != CL_SUCCESS)
        {
            break;
        }

        // Map pinned memory buffer so it can be accessed all the time.
        m_pSysmemBuffer[i] = static_cast<char*>(clEnqueueMapBuffer(m_clCmdQueue, m_clPageLockedBuffer[i], CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, m_nOutputBufferSize, 0, nullptr, nullptr, &nStatus));
        if (nStatus != CL_SUCCESS)
        {
            break;
        }

        memset(m_pSysmemBuffer[i], 0, m_nOutputBufferSize);

        // Create Result buffer.
        m_clResultBuffer[i] = clCreateBuffer(m_clCtx, CL_MEM_READ_WRITE, m_nOutputBufferSize, nullptr, &nStatus);
        if (nStatus != CL_SUCCESS)
        {
            break;
        }

        // Init Result buffers. This buffer will contain the converted image and is used by the kernel as destination buffer.
        char cPattern = 0;
        nStatus = clEnqueueFillBuffer(m_clCmdQueue, m_clResultBuffer[i], &cPattern, sizeof(cPattern), 0, m_nOutputBufferSize, 0, nullptr, nullptr);
        if (nStatus != CL_SUCCESS)
        {
            break;
        }
    }

    SAFE_CALL_CL(nStatus);

    if (!configureKernels())
    {
        return RF_STATUS_OPENCL_FAIL;
    }

    return RF_STATUS_OK;
}


RFStatus RFContextCL::deleteBuffers()
{
    for (int i = 0; i < NUM_RESULT_BUFFERS; ++i)
    {
        m_clCSCFinished[i].release();
        m_clDMAFinished[i].release();
    }

    if (m_clCmdQueue)
    {
        clFinish(m_clCmdQueue);
    }

    if (m_clDMAQueue)
    {
        clFinish(m_clDMAQueue);
    }

    cl_int nStatus = CL_SUCCESS;

    for (int i = 0; i < NUM_RESULT_BUFFERS; ++i)
    {
        if (m_clPageLockedBuffer[i])
        {
            nStatus |= clEnqueueUnmapMemObject(m_clCmdQueue, m_clPageLockedBuffer[i], m_pSysmemBuffer[i], 0, nullptr, nullptr);
            clFinish(m_clCmdQueue);

            nStatus |= clReleaseMemObject(m_clPageLockedBuffer[i]);

            m_clPageLockedBuffer[i] = NULL;
            m_pSysmemBuffer[i] = nullptr;
        }

        if (m_clResultBuffer[i])
        {
            nStatus |= clReleaseMemObject(m_clResultBuffer[i]);
            m_clResultBuffer[i] = NULL;
        }

        m_rtState[i] = RF_STATE_INVALID;
    }


    for (unsigned int i = 0; i < MAX_NUM_RENDER_TARGETS; ++i)
    {
        if (m_clInputImage[i])
        {
            nStatus |= clReleaseMemObject(m_clInputImage[i]);
            m_clInputImage[i] = NULL;
        }
    }

    if (nStatus != CL_SUCCESS)
    {
        return RF_STATUS_OPENCL_FAIL;
    }

    m_uiInputWidth          = 0;
    m_uiInputHeight         = 0;
    m_uiOutputWidth         = 0;
    m_uiOutputHeight        = 0;
    m_uiAlignedOutputWidth  = 0;
    m_uiAlignedOutputHeight = 0;

    m_uiNumRegisteredRT = 0;

    return RF_STATUS_OK;
}


// validateDimensions is called either when a new input texture is registered or when the result buffers
// are created. An input texture might have been registered before the result buffer is created,
// or vice versa. In any case the dimensions have to match.
bool RFContextCL::validateDimensions(unsigned int uiWidth, unsigned int uiHeight)
{
    // If an input texture was already registered any new input texture or 
    // result buffer has to match those dimensions.
    if (m_uiInputWidth > 0)
    {
        if (uiWidth != m_uiInputWidth || uiHeight != m_uiInputHeight)
        {
            return false;
        }
    }

    // If a result buffer was already created the size of any new input texture
    // must match those dimensions.
    if (m_uiOutputWidth > 0)
    {
        if ((uiWidth != m_uiOutputWidth) || uiHeight != m_uiOutputHeight)
        {
            return false;
        }
    }

    return true;
}


bool RFContextCL::configureKernels()
{
    if (m_uiOutputWidth == 0 || m_uiOutputHeight == 0 || m_TargetFormat == RF_FORMAT_UNKNOWN)
    {
        return false;
    }

    m_CSCKernels[RF_KERNEL_RGBA_TO_NV12].uiGlobalWorkSize[0] = m_uiOutputWidth / 2;
    m_CSCKernels[RF_KERNEL_RGBA_TO_NV12].uiGlobalWorkSize[1] = m_uiOutputHeight / 2;
    m_CSCKernels[RF_KERNEL_RGBA_TO_NV12].uiLocalWorkSize[0] = 16;
    m_CSCKernels[RF_KERNEL_RGBA_TO_NV12].uiLocalWorkSize[1] = 16;

    m_CSCKernels[RF_KERNEL_RGBA_TO_NV12_PLANES].uiGlobalWorkSize[0] = m_uiOutputWidth / 2;
    m_CSCKernels[RF_KERNEL_RGBA_TO_NV12_PLANES].uiGlobalWorkSize[1] = m_uiOutputHeight / 2;
    m_CSCKernels[RF_KERNEL_RGBA_TO_NV12_PLANES].uiLocalWorkSize[0] = 16;
    m_CSCKernels[RF_KERNEL_RGBA_TO_NV12_PLANES].uiLocalWorkSize[1] = 16;

    m_CSCKernels[RF_KERNEL_RGBA_TO_I420].uiGlobalWorkSize[0] = m_uiOutputWidth / 2;
    m_CSCKernels[RF_KERNEL_RGBA_TO_I420].uiGlobalWorkSize[1] = m_uiOutputHeight / 2;
    m_CSCKernels[RF_KERNEL_RGBA_TO_I420].uiLocalWorkSize[0] = 16;
    m_CSCKernels[RF_KERNEL_RGBA_TO_I420].uiLocalWorkSize[1] = 16;

    m_CSCKernels[RF_KERNEL_RGBA_COPY].uiGlobalWorkSize[0] = m_uiOutputWidth;
    m_CSCKernels[RF_KERNEL_RGBA_COPY].uiGlobalWorkSize[1] = m_uiOutputHeight;
    m_CSCKernels[RF_KERNEL_RGBA_COPY].uiLocalWorkSize[0] = 16;
    m_CSCKernels[RF_KERNEL_RGBA_COPY].uiLocalWorkSize[1] = 16;

    cl_int4 vDim = { static_cast<int>(m_uiOutputWidth),
                     static_cast<int>(m_uiOutputHeight),
                     static_cast<int>(m_uiAlignedOutputWidth),
                     static_cast<int>(m_uiAlignedOutputHeight) };

    for (unsigned int i = 0; i < RF_KERNEL_NUMBER; ++i)
    {
        cl_int doFlip = 0;

        m_CSCKernels[i].uiGlobalWorkSize[0] = (m_CSCKernels[i].uiGlobalWorkSize[0] + m_CSCKernels[i].uiLocalWorkSize[0] - 1) & ~(m_CSCKernels[i].uiLocalWorkSize[0] - 1);
        m_CSCKernels[i].uiGlobalWorkSize[1] = (m_CSCKernels[i].uiGlobalWorkSize[1] + m_CSCKernels[i].uiLocalWorkSize[1] - 1) & ~(m_CSCKernels[i].uiLocalWorkSize[1] - 1);

        // 3. Diemension
        if (clSetKernelArg(m_CSCKernels[i].kernel, 2, sizeof(cl_int4), &vDim) != CL_SUCCESS)
        {
            return false;
        }

        // 4. Flipping (default off)
        if (clSetKernelArg(m_CSCKernels[i].kernel, 3, sizeof(cl_int), &doFlip) != CL_SUCCESS)
        {
            return false;
        }
    }

    if (m_uiCSCKernelIdx == RF_KERNEL_RGBA_COPY)
    {
        // The RGBA copy kernel converts RGBA input to one of the following outputs:
        // 0: RF_RGBA8
        // 1: RF_ARGB8
        // 2: RF_BGRA8
        // ATTENTION: The copy_rgba_image2d kernel expects the above values. If the enum RFFormat changes, the kernel
        // might need to be updated as well.

        // Set Argumnet 5: Output Ordering
        if (clSetKernelArg(m_CSCKernels[RF_KERNEL_RGBA_COPY].kernel, 4, sizeof(cl_int), &m_TargetFormat) != CL_SUCCESS)
        {
            return false;
        }
    }

    return true;
}


RFStatus RFContextCL::acquireCLMemObj(cl_command_queue clQueue, unsigned int idx, unsigned int numEvents, cl_event* eventsWait, cl_event* eventReturned)
{
    if (m_fnAcquireMemObj)
    {
        if (m_fnAcquireMemObj(clQueue, 1, &m_clInputImage[idx], numEvents, eventsWait, eventReturned) != CL_SUCCESS)
        {
            return RF_STATUS_OPENCL_FAIL;
        }
        clFlush(clQueue);
    }

    m_rtState[idx] = RF_STATE_BLOCKED;

    return RF_STATUS_OK;
}


RFStatus RFContextCL::releaseCLMemObj(cl_command_queue clQueue, unsigned int idx, unsigned int numEvents, cl_event* eventsWait, cl_event* eventReturned)
{
    if (m_fnReleaseMemObj)
    {
        if (m_fnReleaseMemObj(clQueue, 1, &m_clInputImage[idx], numEvents, eventsWait, eventReturned) != CL_SUCCESS)
        {
            return RF_STATUS_OPENCL_FAIL;
        }
        clFlush(clQueue);
    }

    m_rtState[idx] = RF_STATE_FREE;

    return RF_STATUS_OK;
}


RFStatus RFContextCL::removeCLInputMemObj(unsigned int idx)
{
    if (idx >= MAX_NUM_RENDER_TARGETS)
    {
        return RF_STATUS_INVALID_INDEX;
    }
    if ((m_rtState[idx] == RF_STATE_INVALID) || (m_uiNumRegisteredRT == 0))
    {
        return RF_STATUS_INVALID_RENDER_TARGET;
    }

    if (m_clInputImage[idx])
    {
        clReleaseMemObject(m_clInputImage[idx]);
    }

    m_rtState[idx] = RF_STATE_INVALID;
    m_clInputImage[idx] = NULL;

    --m_uiNumRegisteredRT;

    return RF_STATUS_OK;
}


RFStatus RFContextCL::getInputMemObjState(RFRenderTargetState* state, unsigned int idx) const
{
    if (idx >= MAX_NUM_RENDER_TARGETS)
    {
        *state = RF_STATE_INVALID;
        return RF_STATUS_INVALID_INDEX;
    }

    *state = m_rtState[idx];
    return RF_STATUS_OK;
}


// Just return cl_mem which can be used for further processing on the GPU.
// The correctness of idx is verfied in RFSession::encodeFrame.
void RFContextCL::getResultBuffer(unsigned int idx, cl_mem* pBuffer) const
{
    *pBuffer = m_clResultBuffer[idx];
}


void RFContextCL::getResultBuffer(unsigned int idx, void* &pBuffer) const
{
    pBuffer = nullptr;

    if (!m_bUseAsyncCopy)
    {
        clEnqueueCopyBuffer(m_clCmdQueue, m_clResultBuffer[idx], m_clPageLockedBuffer[idx], 0, 0, m_nOutputBufferSize, 0, nullptr, nullptr);
        clFinish(m_clCmdQueue);
    }

    // Wait for transfer to sys mem to complete. The wait will only be executed
    // if the event exists.
    m_clDMAFinished[idx].wait();

    // Release the CSC event. The transfer will wait for the CSC to have completed before
    // starting, hence we only need to release the event here, no wait is required.
    m_clCSCFinished[idx].release();

    pBuffer = m_pSysmemBuffer[idx];
}


void RFContextCL::getInputImage(unsigned int idx, cl_mem* pBuffer) const
{
    *pBuffer = m_clInputImage[idx];
}


RFStatus RFContextCL::processBuffer(bool bRunCSC, bool bInvert, unsigned int uiSrcIdx, unsigned int uiDestIdx)
{
    if (!m_bValid)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    if (m_TargetFormat == RF_FORMAT_UNKNOWN || m_uiCSCKernelIdx <= RF_KERNEL_UNKNOWN || m_uiCSCKernelIdx >= RF_KERNEL_NUMBER)
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    // Make sure events get released. CSC and DMA will create an event per result buffer to be able to check for completion.
    // This is usually only needed if async DMA is used to transfer the m_clResultBuffer to system memory.
    // In case the events were not used we release them here. Here we do NOT need to sync. The session will only submit
    // as many tasks as we have free result buffers.
    m_clDMAFinished[uiDestIdx].release();
    m_clCSCFinished[uiDestIdx].release();

    // Test if output mem object is valid. Acquire will test that the input mem object is valid. 
    if (!m_clResultBuffer[uiDestIdx])
    {
        return RF_STATUS_INVALID_OPENCL_MEMOBJ;
    }

    // Acquire OpenCL object from OpenGl/D3D object.
    RFEventCL clAcquireImageEvent;
    RFStatus rfStatus = acquireCLMemObj(m_clCmdQueue, uiSrcIdx, 0, nullptr, &clAcquireImageEvent);

    if (rfStatus != RF_STATUS_OK)
    {
        return rfStatus;
    }

    if (bRunCSC || m_uiCSCKernelIdx != RF_KERNEL_RGBA_COPY)
    {
        // RGBA input buffer (src)
        SAFE_CALL_CL(clSetKernelArg(m_CSCKernels[m_uiCSCKernelIdx].kernel, 0, sizeof(cl_mem), static_cast<void*>(&(m_clInputImage[uiSrcIdx]))));

        // output buffer (dst)
        SAFE_CALL_CL(clSetKernelArg(m_CSCKernels[m_uiCSCKernelIdx].kernel, 1, sizeof(cl_mem), static_cast<void*>(&(m_clResultBuffer[uiDestIdx]))));

        int nInvert = (bInvert) ? 1 : 0;
        SAFE_CALL_CL(clSetKernelArg(m_CSCKernels[m_uiCSCKernelIdx].kernel, 3, sizeof(cl_int), static_cast<void*>(&nInvert)));

        SAFE_CALL_CL(clEnqueueNDRangeKernel(m_clCmdQueue, m_CSCKernels[m_uiCSCKernelIdx].kernel, 2, nullptr,
                     m_CSCKernels[m_uiCSCKernelIdx].uiGlobalWorkSize, m_CSCKernels[m_uiCSCKernelIdx].uiLocalWorkSize, 0,
                     nullptr, &m_clCSCFinished[uiDestIdx]));

        clFlush(m_clCmdQueue);

        if (m_bUseAsyncCopy)
        {
            clEnqueueCopyBuffer(m_clDMAQueue, m_clResultBuffer[uiDestIdx], m_clPageLockedBuffer[uiDestIdx], 0, 0, m_nOutputBufferSize, 1, &m_clCSCFinished[uiDestIdx], &m_clDMAFinished[uiDestIdx]);
            clFlush(m_clDMAQueue);
        }
    }
    else
    {
        const size_t src_origin[3] = { 0, 0, 0 };
        const size_t region[3] = { m_uiOutputWidth, m_uiOutputHeight, 1 };
        if (m_bUseAsyncCopy)
        {
            SAFE_CALL_CL(clEnqueueCopyImageToBuffer(m_clDMAQueue, m_clInputImage[uiSrcIdx], m_clPageLockedBuffer[uiDestIdx], src_origin, region, 0, 1, &clAcquireImageEvent, &m_clDMAFinished[uiDestIdx]));
            clFlush(m_clDMAQueue);
            // Return without releasing the OpenCL MemObj as it will be used as input for the diffmap kernel.
            return RF_STATUS_OK;
        }
        else
        {
            SAFE_CALL_CL(clEnqueueCopyImageToBuffer(m_clCmdQueue, m_clInputImage[uiSrcIdx], m_clResultBuffer[uiDestIdx], src_origin, region, 0, 0, nullptr, &m_clCSCFinished[uiDestIdx]));
            clFlush(m_clCmdQueue);
        }
    }

    // Release OpenCL object from OpenGL/D3D objec.
    rfStatus = releaseCLMemObj(m_clCmdQueue, uiSrcIdx);

    if (rfStatus != RF_STATUS_OK)
    {
        return rfStatus;
    }

    return RF_STATUS_OK;
}


bool RFContextCL::getFreeRenderTargetIndex(unsigned int& uiIndex)
{
    if (m_uiNumRegisteredRT >= MAX_NUM_RENDER_TARGETS)
    {
        char buf[256];
        sprintf_s(buf, 256, "Exceed the maximum number of render targets: %d", MAX_NUM_RENDER_TARGETS);
        RF_Error(RF_STATUS_RENDER_TARGET_FAIL, buf);

        return false;
    }

    // Find a render target index whose state is invalid.
    bool found = false;
    for (int i = 0; i < MAX_NUM_RENDER_TARGETS; ++i)
    {
        if (m_rtState[i] == RF_STATE_INVALID)
        {
            uiIndex = i;
            found = true;
            break;
        }
    }

    return found;
}


RFStatus RFContextCL::setupKernel()
{
    if (buildCLProgram(CSC_KERNEL_FILE_NAME, str_cl_kernels, m_clCscProgram) == RF_STATUS_OK)
    {
        cl_int nStatus = 0;

        // Create color space conversion kernels.
        m_CSCKernels[RF_KERNEL_RGBA_TO_NV12].kernel = clCreateKernel(m_clCscProgram, "rgbaTonv12_image2d", &nStatus);
        SAFE_CALL_CL(nStatus);
        m_CSCKernels[RF_KERNEL_RGBA_TO_NV12_PLANES].kernel = clCreateKernel(m_clCscProgram, "rgbaToNV12_Planes", &nStatus);
        SAFE_CALL_CL(nStatus);
        m_CSCKernels[RF_KERNEL_RGBA_TO_I420].kernel = clCreateKernel(m_clCscProgram, "rgbaToI420_image2d", &nStatus);
        SAFE_CALL_CL(nStatus);
        m_CSCKernels[RF_KERNEL_RGBA_COPY].kernel = clCreateKernel(m_clCscProgram, "copy_rgba_image2d", &nStatus);
        SAFE_CALL_CL(nStatus);

        return RF_STATUS_OK;
    }

    m_clCscProgram = NULL;

    return RF_STATUS_OPENCL_FAIL;
}


RFStatus RFContextCL::buildCLProgram(const std::string& strKernelFileName, const char* pSources, cl_program& clProgram) const
{
    cl_int nStatus;
    std::string binFileName(utilGetExecutablePath());

    // Build binary file name.
    size_t dot_pos = strKernelFileName.find_last_of(".");

    if (dot_pos == std::string::npos)
    {
        binFileName += strKernelFileName;
    }
    else
    {
        binFileName += strKernelFileName.substr(0, dot_pos);
    }

    binFileName.append(".clbin");

    string options;

    if (m_WaveFrontSize != 0)
    {
        stringstream strStream;

        strStream << m_WaveFrontSize;
        string cWaveSize = strStream.str();
        options = "-D WAVEFRONTSIZE=";

        options += cWaveSize;
    }

#ifndef _DEBUG

    bool sourceWithBinary = false;
    FILE* fp = fopen(binFileName.c_str(), "rb");

    if (fp)
    {
        fseek(fp, 0L, SEEK_END);
        size_t size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        DWORD fileVersion[] = { UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX };
        if (size > sizeof(DWORD) * 4)
        {
            fread(fileVersion, sizeof(DWORD), 4, fp);
        }

        if (   fileVersion[0] == m_dwVersion[0] && fileVersion[1] == m_dwVersion[1]
            && fileVersion[2] == m_dwVersion[2] && fileVersion[3] == m_dwVersion[3])
        {
            size_t binSize = size - (4 * sizeof(DWORD));
            unsigned char* binary = new (nothrow)unsigned char[binSize];
            if (!binary)
            {
                return RF_STATUS_MEMORY_FAIL;
            }
            fread(binary, sizeof(char), binSize, fp);
            clProgram = clCreateProgramWithBinary(m_clCtx, 1, &m_clDevId, &binSize, const_cast<const unsigned char**>(&binary), nullptr, &nStatus);
            sourceWithBinary = true;
            delete[] binary;
        }

        fclose(fp);
    }

    if (!sourceWithBinary)
    {
        clProgram = clCreateProgramWithSource(m_clCtx, 1, &pSources, nullptr, &nStatus);
    }
    SAFE_CALL_CL(nStatus);

    nStatus = clBuildProgram(clProgram, 1, &m_clDevId, options.c_str(), nullptr, nullptr);
    if (nStatus != CL_SUCCESS)
    {
        if (nStatus == CL_BUILD_PROGRAM_FAILURE)
        {
            char* buildLog = nullptr;
            size_t buildLogSize = 0;
            SAFE_CALL_CL(clGetProgramBuildInfo(clProgram, m_clDevId, CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, &buildLogSize));

            buildLog = new (nothrow)char[buildLogSize];
            RF_Assert(!buildLog);
            memset(buildLog, 0, buildLogSize);

            SAFE_CALL_CL(clGetProgramBuildInfo(clProgram, m_clDevId, CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, nullptr));
            RF_Error(RF_STATUS_OPENCL_FAIL, buildLog);
            delete[] buildLog;
        }
        return RF_STATUS_OPENCL_FAIL;
    }
    else if (sourceWithBinary == false)
    {
        size_t binSize;
        char* clBin;
        SAFE_CALL_CL(clGetProgramInfo(clProgram, CL_PROGRAM_BINARY_SIZES, sizeof(binSize), &binSize, nullptr));
        clBin = new (nothrow)char[binSize];
        if (!clBin)
        {
            return RF_STATUS_MEMORY_FAIL;
        }
        SAFE_CALL_CL(clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES, sizeof(char*), &clBin, nullptr));

        fstream f(binFileName.c_str(), fstream::out | fstream::binary | fstream::trunc);
        if (   0 == m_dwVersion[0] && 0 == m_dwVersion[1]
            && 0 == m_dwVersion[2] && 0 == m_dwVersion[3])
        {
            DWORD fileVersion[] = { UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX };
            f.write(reinterpret_cast<const char*>(fileVersion), sizeof(DWORD) * 4);
        }
        else
        {
            f.write(reinterpret_cast<const char*>(m_dwVersion), sizeof(DWORD) * 4);
        }
        f.write(clBin, binSize);
        f.close();
        delete[] clBin;
    }

#else
    // In case of DEBUG builds load kernel from file. This makes tests with kernel changes easier.
    fstream         InFile;
    string          strLine;
    string          strKernelSource;

    // Read kernel sources from file.
    std::string strKernelPath(utilGetExecutablePath());

    strKernelPath += strKernelFileName;

    // Try loading kernel sources from working dir.
    InFile.open(strKernelFileName, fstream::in);

    if (!InFile.is_open())
    {
        // Try loading kernel sources from exe dir.
        InFile.open(strKernelPath, fstream::in);
    }

    if (InFile.is_open())
    {
        // Kernel source file found -> read sources.
        while (!InFile.eof())
        {
            getline(InFile, strLine);
            strKernelSource += strLine;
            strKernelSource += "\n";
        }
    }
    else
    {
        std::stringstream oss;

        oss << "Running debug build but did not find: " << strKernelFileName << " -> using built in kernel";
        RF_Error(CL_SUCCESS, oss.str().c_str());

        // No kernel source file found -> use built in sources.
        strKernelSource = pSources;
    }

    char const* pKernelSrc = strKernelSource.c_str();

    clProgram = clCreateProgramWithSource(m_clCtx, 1, &pKernelSrc, nullptr, &nStatus);
    SAFE_CALL_CL(nStatus);

    nStatus = clBuildProgram(clProgram, 1, &m_clDevId, options.c_str(), nullptr, nullptr);

    if (nStatus != CL_SUCCESS)
    {
        if (nStatus == CL_BUILD_PROGRAM_FAILURE)
        {
            char* buildLog = nullptr;
            size_t buildLogSize = 0;
            SAFE_CALL_CL(clGetProgramBuildInfo(clProgram, m_clDevId, CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, &buildLogSize));

            buildLog = new (nothrow)char[buildLogSize];
            RF_Assert(!buildLog);
            memset(buildLog, 0, buildLogSize);

            SAFE_CALL_CL(clGetProgramBuildInfo(clProgram, m_clDevId, CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, nullptr));
            RF_Error(RF_STATUS_OPENCL_FAIL, buildLog);
            delete[] buildLog;
        }

        return RF_STATUS_OPENCL_FAIL;
    }

#endif

    return RF_STATUS_OK;
}