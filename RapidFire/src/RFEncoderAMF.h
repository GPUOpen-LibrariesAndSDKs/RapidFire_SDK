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
    virtual RFStatus            init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig) override;

    virtual RFStatus            resize(unsigned int uiWidth, unsigned int uiHeight) override;

    virtual RFStatus            encode(unsigned int uiBufferIdx, bool bUseInputImages) override;

    // Sets single parameter to encoder.
    virtual RFStatus            setParameter(const unsigned int uiParameterName, RFParameterType rfType, RFProperties value) override;

    // Returns the value that is used by the encoder for parameter uiParameterName.
    virtual RFParameterState    getParameter(const unsigned int uiParameterName, RFVideoCodec codec, RFProperties& value) const override;

    virtual bool                isFormatSupported(RFFormat format) const override;

    virtual bool                isResizeSupported() const override { return true; }

    // Returns preferred format of the encoder.
    virtual RFFormat            getPreferredFormat() const override { return RF_NV12; }

    virtual RFVideoCodec        getPreferredVideoCodec() const override { return RF_VIDEO_CODEC_AVC; }

    virtual RFStatus            getEncodedFrame(unsigned int& uiSize, void* &pBitStream) override;

    // Defines if getEncodeFrame should block until a frame is ready or not. If no blocking is used the call will return
    // immediatly unless the VCE queue is full and AMF needs to wait for an empty slot before submitting the next frame.
    void                        setBlockingRead(bool block);

    struct MAPPING_ENTRY
    {
        const unsigned int   RFPropertyName;
        const wchar_t*       AMFPropertyName;
    };

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

    RFVideoCodec                    m_videoCodec;
    const MAPPING_ENTRY*            m_pPropertyNameMap;
    unsigned int                    m_uiPropertyNameMapCount;

    std::vector<std::pair<const wchar_t*, unsigned int>>   m_pPreSubmitSettings;
};