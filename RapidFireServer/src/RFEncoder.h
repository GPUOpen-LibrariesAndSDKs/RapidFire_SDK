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

    virtual RFParameterState    getParameter(unsigned int const uiParameterName, RFProperties &value) const { return RF_PARAMETER_STATE_INVALID; }

    // Encode a frame
    virtual RFStatus            encode(unsigned int uiBufferIdx)                          { return RF_STATUS_FAIL; }

    virtual RFStatus            getEncodedFrame(unsigned int& uiSize, void* &pBitStream)  { return RF_STATUS_FAIL; }

    // Returns true if the format is supporetd as input by the encoder.
    virtual bool                isFormatSupported(RFFormat format)  const { return false; };

    virtual bool                isResizeSupported()                 const { return false; };        

    // Returns the preferred input format of the encoder.
    virtual RFFormat    getPreferredFormat()    const           { return RF_FORMAT_UNKNOWN; };

    unsigned int        getWidth()              const           { return m_uiWidth;         }

    unsigned int        getHeight()             const           { return m_uiHeight;        }

    unsigned int        getAlignedWidth()       const           { return m_uiAlignedWidth;  }

    unsigned int        getAlignedHeight()      const           { return m_uiAlignedHeight; }

    unsigned int        getOutputWidth()        const           { return m_uiOutputWidth;   }

    unsigned int        getOutputHeight()       const           { return m_uiOutputHeight;  }

    std::string         getName()               const           { return m_strEncoderName;  }

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