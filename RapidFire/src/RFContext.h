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

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <CL/cl.h>

#include "RapidFire.h"
#include "RFPlatform.h"
#include "RFTypes.h"

class RFEventCL
{
public:

    RFEventCL();
    ~RFEventCL();

    void        wait();
    void        release();

    cl_event*   operator&();

    operator cl_event() const { return m_clEvent; }

private:

    // disable copy constructor
    RFEventCL(const RFEventCL& RFEventCL);
    // Disable assignment
    RFEventCL& operator=(const RFEventCL& rhs);

    bool        m_bReleased;
    cl_event    m_clEvent;
};


class RFProgramCL
{
public:

    RFProgramCL() : m_device(NULL), m_program(NULL), m_built(false) {}
    bool Create(cl_context context, cl_device_id device, const char* kernelFileName, const char* kernelSourceCode, const char* options = nullptr);

    std::string GetBuildLog() const;

    void Release();

    operator cl_program() const { return m_program; }

    operator bool() const
    {
        return m_built;
    }

private:

    // disable copy constructor
    RFProgramCL(const RFProgramCL& RFProgramCL);
    // Disable assignment
    RFProgramCL& operator=(const RFProgramCL& rhs);

    uint64_t GetModuleVer(const char* moduleName) const;
    std::string GetDeviceName() const;

    struct BinFileData
    {
        BinFileData() : moduleVer(0), openCLVer(0) {}

        uint64_t moduleVer;
        uint64_t openCLVer;
        std::string deviceName;
        std::vector<unsigned char> data;
    };
    BinFileData LoadBinaryFile(const char* binaryFileName) const;
    void StoreBinaryFile(const char* binaryFileName, const char* deviceName) const;

    bool BuildFromSourceCode(cl_context context, const char* sourceCode, const char* options);
    bool BuildFromSourceFile(cl_context context, const char* sourceFile, const char* options);

    cl_device_id		m_device;
    cl_program			m_program;
    bool				m_built;

    static uint64_t		s_uiModuleVer;
    static uint64_t		s_uiOpenCLVer;
    static std::mutex	s_lock;


};


class RFContextCL
{
public:

    explicit RFContextCL();
    virtual ~RFContextCL();

    // Creates OpenCL context.
    virtual RFStatus    createContext();
    // Creates OpenCL context based on an existing OpenGL context.
    virtual RFStatus    createContext(DeviceCtx hDC, GraphicsCtx hGLRC);
    // Creates OpenCL context based on an existing D3D11 Device.
    virtual RFStatus    createContext(ID3D11Device*       pD3DDevice);
    // Creates OpenCL context based on an existing D3D9 Device.
    virtual RFStatus    createContext(IDirect3DDevice9*   pD3DDevice);
    // Creates OpenCL context based on an existing D3D9Ex Device.
    virtual RFStatus    createContext(IDirect3DDevice9Ex* pD3DDeviceEx);

    // Creates OpenCL Output buffers. Those buffers will contain the results of the CSC.
    virtual RFStatus    createBuffers(RFFormat format, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiAlignedWidth, unsigned int uiAlignedHeight, bool bUseAsyncCopy = false);

    // Deletes all buffers including registered textures.
    virtual RFStatus    deleteBuffers();

    // Registers OpenGL texture.
    virtual RFStatus    setInputTexture(const unsigned int uiTextureName, const unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx);
    // Registers DX 11 texture.
    virtual RFStatus    setInputTexture(ID3D11Texture2D* pD3D11Texture, const unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx);
    // registers DX 9 texture.
    virtual RFStatus    setInputTexture(IDirect3DSurface9* pD3D9Texture, const unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx);

    // Converts color space. The input buffer is m_clBuffer[uiSorceIdx], the output is stored in m_clResultBuffer[uiDestIdx].
    virtual RFStatus    processBuffer(bool bRunCSC, bool bInvert, unsigned int uiSorceIdx, unsigned int uiDestIdx);

    // Removes an OpenCL object that has been created from a GL/D3D object.
    RFStatus            removeCLInputMemObj(unsigned int idx);

    // Gets the state of a GL/D3D object.
    RFStatus            getInputMemObjState(RFRenderTargetState* state, unsigned int idx) const;

    // Copys CSC results to the GPU or sys memory.
    void                getResultBuffer(unsigned int idx, cl_mem* pBuffer) const;
    // Blocks until all results are written into the m_clResultBuffer[idx] and returns the pointer to the buffer in sys mem.
    void                getResultBuffer(unsigned int idx, void* &pBuffer) const;

    // Acquires an OpenCL object that has been created from a GL/D3D object.
    RFStatus            acquireCLMemObj(cl_command_queue clQueue, unsigned int idx, unsigned int numEvents = 0, cl_event* eventsWait = nullptr, cl_event* eventReturned = nullptr);

    // Releases an OpenCL object that has been created from a GL/D3D object.
    RFStatus            releaseCLMemObj(cl_command_queue clQueue, unsigned int idx, unsigned int numEvents = 0, cl_event* eventsWait = nullptr, cl_event* eventReturned = nullptr);

    void                getInputImage(unsigned int idx, cl_mem* pBuffer) const;

    bool                isValid()       const { return m_bValid; }

    cl_context          getContext()    const { return m_clCtx; }

    cl_device_id        getDeviceId()   const { return m_clDevId; }

    cl_command_queue    getCmdQueue()   const { return m_clCmdQueue; }

    cl_command_queue    getDMAQueue()   const { return m_clDMAQueue; }

    unsigned int        getNumResultBuffers() const { return m_uiNumResultBuffers; }

    unsigned int        getNumRegisteredRT()  const { return m_uiNumRegisteredRT; }

    unsigned int        getResultBufferSize() const { return static_cast<unsigned int>(m_nOutputBufferSize); }

    RFFormat            getTargetFormat()     const { return m_TargetFormat; }

    unsigned int        getOutputWidth()      const { return m_uiOutputWidth; }

    unsigned int        getOutputHeight()     const { return m_uiOutputHeight; }

    bool                getAsyncCopy()        const { return m_bUseAsyncCopy; }

    enum ctx_type { RF_CTX_UNKNOWN = -1, RF_CTX_CL = 0, RF_CTX_FROM_GL = 1, RF_CTX_FROM_DX9EX = 2, RF_CTX_FROM_DX9 = 3, RF_CTX_FROM_DX11 = 4 };

    ctx_type            getCtxType()          const { return m_CtxType; }

protected:

    enum csc_kernel { RF_KERNEL_UNKNOWN = -1, RF_KERNEL_RGBA_TO_NV12 = 0, RF_KERNEL_RGBA_TO_NV12_PLANES = 1, RF_KERNEL_RGBA_TO_I420 = 2, RF_KERNEL_RGBA_COPY = 3, RF_KERNEL_NUMBER = 4 };

    typedef struct
    {
        cl_kernel    kernel;
        size_t       uiGlobalWorkSize[2];
        size_t       uiLocalWorkSize[2];
    } CSC_KERNEL;

    typedef cl_int(CL_API_CALL *CL_MEM_ACCESS_FUNCTION) (cl_command_queue, cl_uint, const cl_mem*, cl_uint, const cl_event*, cl_event*);

    cl_mem              createFromDX9MediaSurface(cl_mem_flags flags, IDirect3DSurface9 *resource, UINT subresource, cl_int  *errcode_ret) const;
    cl_mem              createFromD3D11Texture2DKHR(cl_mem_flags flags, ID3D11Texture2D *resource, UINT subresource, cl_int  *errcode_ret) const;
    cl_mem              convertImageAmd(cl_mem image, cl_image_format* dstFormat, cl_int* errcode_ret) const;
    cl_mem              getPlaneFromImageAmd(cl_mem image, cl_uint plane, cl_int* errcode_ret) const;

    RFStatus            finalizeContext(const cl_context_properties*);

    void                setMemAccessFunction();

    bool                configureKernels();
    bool                getFreeRenderTargetIndex(unsigned int& uiIndex);

    RFStatus            setupKernel();

    // Checks if the texture and the buffer dimension match.
    bool                validateDimensions(unsigned int uiWidth, unsigned int uiHeight);

    bool                        m_bValid;

    // Dimensions of output buffers
    unsigned int                m_uiOutputWidth;
    unsigned int                m_uiOutputHeight;
    unsigned int                m_uiAlignedOutputWidth;
    unsigned int                m_uiAlignedOutputHeight;
    size_t                      m_nOutputBufferSize;

    // Dimensions of input buffer/texture
    unsigned int                m_uiInputWidth;
    unsigned int                m_uiInputHeight;

    // The amount of result and page locked buffers
    const unsigned int          m_uiNumResultBuffers;
    // The number of registered render targets.
    unsigned int                m_uiNumRegisteredRT;

    unsigned int                m_uiPendingTransfers;

    // OpenCL specific members
    cl_platform_id              m_clPlatformId;
    cl_device_id                m_clDevId;
    cl_context                  m_clCtx;
    RFProgramCL					m_clCscProgram;
    cl_command_queue            m_clCmdQueue;
    cl_command_queue            m_clDMAQueue;

    RFFormat                    m_TargetFormat;
    csc_kernel                  m_uiCSCKernelIdx;

    CSC_KERNEL                  m_CSCKernels[RF_KERNEL_NUMBER];

    // m_clInputBuffer is set by the application when calling setInputTexture.
    // m_clInputBuffer is used as input for the CSC.
    cl_mem                      m_clInputImage[MAX_NUM_RENDER_TARGETS];
    // m_clResultBuffer stores the results of the CSC.
    cl_mem                      m_clResultBuffer[NUM_RESULT_BUFFERS];

    mutable RFEventCL           m_clDMAFinished[NUM_RESULT_BUFFERS];
    mutable RFEventCL           m_clCSCFinished[NUM_RESULT_BUFFERS];

    RFRenderTargetState         m_rtState[MAX_NUM_RENDER_TARGETS];

    // Pinned buffer used for data transfer between GPU and host.
    cl_mem                      m_clPageLockedBuffer[NUM_RESULT_BUFFERS];
    char*                       m_pSysmemBuffer[NUM_RESULT_BUFFERS];

    // Indicates if an asynchronous copy of the result buffer to sys mem should be used.
    bool                        m_bUseAsyncCopy;

    ctx_type                    m_CtxType;

    CL_MEM_ACCESS_FUNCTION      m_fnAcquireInputMemObj;
    CL_MEM_ACCESS_FUNCTION      m_fnReleaseInputMemObj;

    CL_MEM_ACCESS_FUNCTION      m_fnAcquireDX9Obj;
    CL_MEM_ACCESS_FUNCTION      m_fnReleaseDX9Obj;
    CL_MEM_ACCESS_FUNCTION      m_fnAcquireDX11Obj;
    CL_MEM_ACCESS_FUNCTION      m_fnReleaseDX11Obj;

    DWORD						m_dwVersion[4];

private:

    RFContextCL(const RFContextCL& other);
    RFContextCL& operator=(const RFContextCL& other);
};