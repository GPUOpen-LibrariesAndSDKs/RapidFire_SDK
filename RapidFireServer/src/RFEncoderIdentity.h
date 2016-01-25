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

#include "RFEncoder.h"

class RFEncoderIdentity : public RFEncoder
{
public:

    RFEncoderIdentity();

    virtual RFStatus    init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig) override;
                                                                                              
    virtual RFStatus    resize(unsigned int uiWidth, unsigned int uiHeight)                   override;
                                                                                              
    virtual bool        isFormatSupported(RFFormat format) const                              override;
                                                                                              
    virtual RFStatus    encode(unsigned int uiBufferIdx)                                      override;
                                                                                              
    virtual RFStatus    getEncodedFrame(unsigned int& uiSize, void* &pBitStream)              override;

    // Returns preferred format of the encoder.
    virtual RFFormat    getPreferredFormat() const override { return RF_RGBA8; }

    virtual bool        isResizeSupported()  const override { return true; }

private:

    size_t              m_nBufferSize;
    void*               m_pBuffer;

    const RFContextCL*  m_pContext;
};