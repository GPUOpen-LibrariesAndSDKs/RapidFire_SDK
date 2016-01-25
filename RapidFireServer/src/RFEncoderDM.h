#pragma once

#include <queue>

#include <CL/opencl.h>
#include <components/Component.h>
#include <core/Buffer.h>
#include <core/Context.h>

#include "RFEncoder.h"
#include "RFLock.h"

class RFContextCL;

class RFEncoderDM : public RFEncoder
{
public:

    RFEncoderDM();
    ~RFEncoderDM();

    virtual RFStatus            init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig)                        override;
                                                                                                                             
    virtual RFStatus            resize(unsigned int uiWidth, unsigned int uiHeight)                                          override;
                                                                                                                             
    virtual RFStatus            encode(unsigned int uiBufferIdx)                                                             override;
                                                                                                                             
    virtual RFStatus            getEncodedFrame(unsigned int& uiSize, void* &pBitStream)                                     override;
                                                                                                                             
    virtual bool                isFormatSupported(RFFormat format) const                                                     override;
                                                                                                                             
    virtual RFStatus            setParameter(unsigned int const uiParameterName, RFParameterType rfType, RFProperties value) override;
                                                                                                                             
    virtual RFParameterState    getParameter(unsigned int const uiParameterName, RFProperties &value) const                  override;

    // Returns preferred format of the encoder.
    virtual RFFormat            getPreferredFormat() const override { return RF_RGBA8; }

    virtual bool                isResizeSupported()  const override { return true; }

private:

    bool                      deleteBuffers();
    bool                      createBuffers();
    RFStatus                  GenerateCLProgramAndKernel();

    struct DMDiffMapBuffer
    {
        cl_mem              clGPUBuffer;
        cl_mem              clPageLockedBuffer;
        char*               pSysmemBuffer;

        cl_event            clDiffFinished;
        cl_event            clDMAFinished;
    };

    bool                                        m_bLockMappedBuffer;

    unsigned int                                m_uiPreviousBuffer;
    unsigned int                                m_uiDiffMapSize;

    unsigned int                                m_uiNumLocalPixels[2];
    unsigned int                                m_uiTotalBlockSize[2];

    const unsigned int                          m_uiNumTargetBuffers;
    unsigned int                                m_uiCurrentTargetBuffer;

    char*                                       m_pClearData;

    size_t                                      m_globalDim[2];
    size_t                                      m_localDim[2];

    cl_kernel                                   m_DiffMapkernel;
    cl_program                                  m_DiffMapProgram;

    const RFContextCL*                          m_pContext;

    // vector of buffers into which the diff kernel can write.
    std::vector<DMDiffMapBuffer>                m_TargetBuffers;     

    // Queue that contains references to buffers that store a diff map which were not yet
    // read by calling getEncodedFrame
    RFLockedQueue<const DMDiffMapBuffer*>       m_ResultQueue;

    // Pointer to the buffer that was retrieved by calling getEncodedFrame
    const DMDiffMapBuffer*                      m_pMappedBuffer;
};