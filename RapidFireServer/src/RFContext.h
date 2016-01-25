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

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "RapidFireServer.h"
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
    virtual RFStatus    processBuffer(bool bInvert, unsigned int uiSorceIdx, unsigned int uiDestIdx);

    // Removes an OpenCL object that has been created from a GL/D3D object.
    RFStatus            removeCLInputMemObj(unsigned int idx);

    RFStatus            buildCLProgram(const std::string& strKernelFileName, const char* pSources, cl_program& clProgram) const;

    // Gets the state of a GL/D3D object.
    RFRenderTargetState getInputMemObjState(unsigned int idx) const;

    // Copys CSC results to the GPU or sys memory.
    void                getResultBuffer(unsigned int idx, cl_mem* pBuffer) const;
    // Blocks until all results are written into the m_clResultBuffer[idx] and returns the pointer to the buffer in sys mem.
    void                getResultBuffer(unsigned int idx, void* &pBuffer) const;

    void                getInputImage(unsigned int idx, cl_mem* pBuffer) const;

    bool                isValid()       const { return m_bValid; }

    cl_context          getContext()    const { return m_clCtx; }

    cl_device_id        getDeviceId()   const { return m_clDevId; }

    cl_command_queue    getCmdQueue()   const { return m_clCmdQueue; }

    cl_command_queue    getDMAQueue()   const { return m_clDMAQueue; }

    size_t              getWaveFrontSize()   const { return m_WaveFrontSize; }

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

    RFStatus            finalizeContext(const cl_context_properties*);

    void                setMemAccessFunction();

    bool                configureKernels();
    bool                getFreeRenderTargetIndex(unsigned int& uiIndex);

    RFStatus            setupKernel();

    // Acquires an OpenCL object that has been created from a GL/D3D object.
    RFStatus            acquireCLMemObj(unsigned int idx);

    // Releases an OpenCL object that has been created from a GL/D3D object.
    RFStatus            releaseCLMemObj(unsigned int idx);

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
    cl_program                  m_clCscProgram;
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

    size_t                      m_WaveFrontSize;

    CL_MEM_ACCESS_FUNCTION      m_fnAcquireMemObj;
    CL_MEM_ACCESS_FUNCTION      m_fnReleaseMemObj;

    DWORD						m_dwVersion[4];

private:

    RFContextCL(const RFContextCL& other);
    RFContextCL& operator=(const RFContextCL& other);
};