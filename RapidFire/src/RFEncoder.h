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

#include "RFContext.h"

class RFEncoderSettings;

class RFEncoder
{
public:

    RFEncoder()
        : m_uiWidth(0)
        , m_uiHeight(0)
        , m_uiAlignedWidth(0)
        , m_uiAlignedHeight(0)
        , m_uiOutputWidth(0)
        , m_uiOutputHeight(0)
        , m_strEncoderName("RF_ENCODER_UNKNOWN")
    {
        m_format = RF_FORMAT_UNKNOWN;
    }

    virtual ~RFEncoder() {}

    virtual RFStatus            init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig) = 0;

    virtual RFStatus            resize(unsigned int uiWidth, unsigned int uiHeight) { return RF_STATUS_FAIL; }

    virtual RFStatus            setParameter(unsigned int const uiParameterName, RFParameterType rfType, RFProperties value) { return RF_STATUS_FAIL; }

    virtual RFParameterState    getParameter(unsigned int const uiParameterName, RFVideoCodec codec, RFProperties &value) const { return RF_PARAMETER_STATE_INVALID; }

    // Encode a frame
    virtual RFStatus            encode(unsigned int uiBufferIdx, bool bUseInputImages) { return RF_STATUS_FAIL; }

    virtual RFStatus            getEncodedFrame(unsigned int& uiSize, void* &pBitStream)  { return RF_STATUS_FAIL; }

    // Returns true if the format is supporetd as input by the encoder.
    virtual bool                isFormatSupported(RFFormat format)  const { return false; };

    virtual bool                isResizeSupported()                 const { return false; };

    // Returns the preferred input format of the encoder.
    virtual RFFormat    getPreferredFormat()      const           { return RF_FORMAT_UNKNOWN; };

    virtual RFVideoCodec getPreferredVideoCodec() const           { return RF_VIDEO_CODEC_NONE; }

    unsigned int        getWidth()                const           { return m_uiWidth;         }

    unsigned int        getHeight()               const           { return m_uiHeight;        }

    unsigned int        getAlignedWidth()         const           { return m_uiAlignedWidth;  }

    unsigned int        getAlignedHeight()        const           { return m_uiAlignedHeight; }

    unsigned int        getOutputWidth()          const           { return m_uiOutputWidth;   }

    unsigned int        getOutputHeight()         const           { return m_uiOutputHeight;  }

    std::string         getName()                 const           { return m_strEncoderName;  }

protected:

    RFFormat                        m_format;

    unsigned int                    m_uiWidth;
    unsigned int                    m_uiHeight;
    unsigned int                    m_uiAlignedWidth;
    unsigned int                    m_uiAlignedHeight;
    unsigned int                    m_uiOutputWidth;
    unsigned int                    m_uiOutputHeight;

    std::string                     m_strEncoderName;

private:

    RFEncoder(const RFEncoder&);
    RFEncoder& operator=(const RFEncoder& rhs);
};