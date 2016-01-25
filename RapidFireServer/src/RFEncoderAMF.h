#pragma once

#include <vector>

#include <CL/cl.h>
#include <core/Buffer.h>
#include <components/Component.h>

#include "RFContextAMF.h"
#include "RFEncoder.h"

class RFEncoderAMF : public RFEncoder
{
public:

    RFEncoderAMF(void);
    ~RFEncoderAMF(void);

    // Initializes encoder based on the configuration pConfig.
    virtual RFStatus            init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig);

    virtual RFStatus            resize(unsigned int uiWidth, unsigned int uiHeight);

    virtual RFStatus            encode(unsigned int uiBufferIdx);

    // Sets single parameter to encoder.
    virtual RFStatus            setParameter(const unsigned int uiParameterName, RFParameterType rfType, RFProperties value);

    // Returns the value that is used by the encoder for parameter uiParameterName.
    virtual RFParameterState    getParameter(const unsigned int uiParameterName, RFProperties& value) const;

    virtual bool                isFormatSupported(RFFormat format) const;

    virtual bool                isResizeSupported() const { return m_pContext ? m_pContext->getCtxType() == RFContextCL::RF_CTX_FROM_DX9 : false; }

    // Returns preferred format of the encoder.
    virtual RFFormat            getPreferredFormat() const { return RF_NV12; }

    virtual RFStatus            getEncodedFrame(unsigned int& uiSize, void* &pBitStream);

    // Defines if getEncodeFrame should block until a frame is ready or not. If no blocking is used the call will return
    // immediatly unless the VCE queue is full and AMF needs to wait for an empty slot before submitting the next frame.
    void                        setBlockingRead(bool block);

private:

    // Applys parameters in pConfig and validates them. pConfig may contain parameters that are not suppported
    // by the encoder. Those will be set to INVALID by this function.
    bool						applyConfiguration(const RFEncoderSettings* pConfig);
    // Applys preset in pConfig and validate parameters. The AMF preset is applied and the values used by the 
    // encoder are writtten to pConfig.
    bool						applyPreset(const RFEncoderSettings* pConfig);

    // Updates the AMF context with the property specified by uiParameterIndex.
    RFStatus					setAMFProperty(unsigned int uiParameterIndex, RFParameterType rfType, RFProperties value);

    bool                            m_bBlock;
    unsigned int                    m_uiPendingFrames;

    const RFContextAMF*             m_pContext;

    amf::AMFContextPtr              m_amfContext;
    amf::AMFComponentPtr            m_amfEncoder;
    amf::AMFBufferPtr               m_amfEncodedFrame;

    std::vector<std::pair<const wchar_t*, unsigned int>>   m_pPreSubmitSettings;
};