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
#include "RFEncoderIdentity.h"

#include <assert.h>

#include "RFEncoderSettings.h"
#include "RFError.h"
#include "RFUtils.h"


RFEncoderIdentity::RFEncoderIdentity()
    : RFEncoder()
    , m_nBufferSize(0)
    , m_pBuffer(nullptr)
    , m_pContext(nullptr)
{
    m_strEncoderName = "RF_ENCODER_IDENTITY";
}


RFStatus RFEncoderIdentity::init(const RFContextCL* pContextCL, const RFEncoderSettings* pConfig)
{
    if (!pConfig)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    if (!pContextCL)
    {
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    m_pContext = pContextCL;

    m_format = pConfig->getInputFormat();

    if (!isFormatSupported(m_format))
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    m_uiWidth  = pConfig->getEncoderWidth();
    m_uiHeight = pConfig->getEncoderHeight();

    m_uiAlignedWidth  = m_uiWidth;
    m_uiAlignedHeight = m_uiHeight;

    m_uiOutputWidth  = m_uiAlignedWidth;
    m_uiOutputHeight = m_uiAlignedHeight;

    // Set alignment in CSC.
    if (m_format == RF_RGBA8 || m_format == RF_ARGB8 || m_format == RF_BGRA8)
    {
        m_nBufferSize = m_uiAlignedWidth * m_uiAlignedHeight * 4;
    }
    else if (m_format == RF_NV12 || m_format == RF_I420)
    {
        m_nBufferSize = (m_uiAlignedWidth * m_uiAlignedHeight) + (m_uiAlignedWidth * m_uiAlignedHeight) / 2;
    }
    else
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    return RF_STATUS_OK;
}


RFStatus RFEncoderIdentity::resize(unsigned int uiWidth, unsigned int uiHeight)
{
    m_uiWidth  = uiWidth;
    m_uiHeight = uiHeight;

    m_uiAlignedWidth  = m_uiWidth;
    m_uiAlignedHeight = m_uiHeight;

    m_uiOutputWidth  = m_uiAlignedWidth;
    m_uiOutputHeight = m_uiAlignedHeight;

    // Set alignment in CSC.
    if (m_format == RF_RGBA8 || m_format == RF_ARGB8 || m_format == RF_BGRA8)
    {
        m_nBufferSize = m_uiAlignedWidth * m_uiAlignedHeight * 4;
    }
    else if (m_format == RF_NV12 || m_format == RF_I420)
    {
        m_nBufferSize = (m_uiAlignedWidth * m_uiAlignedHeight) + (m_uiAlignedWidth * m_uiAlignedHeight) / 2;
    }
    else
    {
        return RF_STATUS_INVALID_FORMAT;
    }

    return RF_STATUS_OK;
}


bool RFEncoderIdentity::isFormatSupported(RFFormat format) const
{
    if (format == RF_RGBA8 || format == RF_ARGB8 || format == RF_BGRA8 || format == RF_NV12 || format == RF_I420)
    {
        return true;
    }

    return false;
}


RFStatus RFEncoderIdentity::getEncodedFrame(unsigned int& uiSize, void* &pBitStream)
{
    if (m_pBuffer)
    {
        uiSize = static_cast<unsigned int>(m_nBufferSize);
        pBitStream = m_pBuffer;
        m_pBuffer = nullptr;
    }
    else
    {
        return RF_STATUS_NO_ENCODED_FRAME;
    }

    return RF_STATUS_OK;
}


RFStatus RFEncoderIdentity::encode(unsigned int uiBufferIdx)
{
    assert(m_pContext);

    m_pContext->getResultBuffer(uiBufferIdx, m_pBuffer);

    if (!m_pBuffer)
    {
        RF_Error(RF_STATUS_INVALID_OPENCL_MEMOBJ, "Input pBuffer is invalid");
        return RF_STATUS_INVALID_OPENCL_MEMOBJ;
    }

    return RF_STATUS_OK;
}