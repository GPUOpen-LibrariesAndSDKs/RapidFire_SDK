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

#include "RFSession.h"

#include <sstream>

#include "RFContextAMF.h"
#include "RFError.h"
#include "RFEncoderAMF.h"
#include "RFEncoderDM.h"
#include "RFEncoderIdentity.h"
#include "RFEncoderSettings.h"
#include "RFMouseGrab.h"
#include "RFUtils.h"

// Global lock that can be used to make sure only one thread can work on a resource.
// Required if multiple threads run a session.
// Each session has a local lock m_SessionLock which is used to make sure that some functions
// cannot be interrupted by another thread belonging to the same session.
static RFLock g_GlobalSessionLock;


RFSession::RFSession(RFEncoderID rfEncoder)
    : m_ParameterMap()
    , m_uiResultBuffer(0)
    , m_pSessionLog(nullptr)
    , m_pContextCL(nullptr)
    , m_pEncoder(nullptr)
    , m_pEncoderSettings(nullptr)
    , m_BufferQueue()
    , m_SessionLock()
{
    // Local lock: Make sure no other thread of the session interrupts the session creation.
    RFReadWriteAccess enabler(&m_SessionLock);

    createSessionLog();

    try
    {
        // Add all know parameters to map.
        m_ParameterMap.addParameter(RF_ENCODER, RFParameterAttr("RF_FLIP_SOURCE", RF_PARAMETER_INT, 0));
        m_ParameterMap.addParameter(RF_FLIP_SOURCE, RFParameterAttr("RF_FLIP_SOURCE", RF_PARAMETER_BOOL, 0));
        m_ParameterMap.addParameter(RF_ASYNC_SOURCE_COPY, RFParameterAttr("RF_ASYNC_SOURCE_COPY", RF_PARAMETER_BOOL, 0));
        m_ParameterMap.addParameter(RF_ENCODER_BLOCKING_READ, RFParameterAttr("RF_ENCODER_BLOCKING_READ", RF_PARAMETER_BOOL, 0));
    }
    catch (const std::exception& e)
    {
        std::stringstream oss;

        oss << "[rfCreateEncodeSession] Failed to initialize session parameter map " << e.what();
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());

        throw std::runtime_error("RFSession contructor failed");
    }

    m_Properties.EncoderId = rfEncoder;
    m_Properties.uiInputDim[0] = 0;
    m_Properties.uiInputDim[1] = 0;
    m_Properties.bAsyncCopyToSysMem = false;
    m_Properties.bBlockingEncoderRead = false;
    m_Properties.bInvertInput = false;
    m_Properties.bEncoderCSC = true;
    m_Properties.bMousedata = false;
}


RFSession::~RFSession()
{
    // Global lock. Make sure session deletion is not interupted.
    RFReadWriteAccess enabler(&g_GlobalSessionLock);
}


RFStatus RFSession::createContext()
{
    // Global lock: Some encoders like AMF fail to initialize if the context
    // creation is interrupted by another thread.
    RFReadWriteAccess enabler(&g_GlobalSessionLock);

    try
    {
        if (m_Properties.EncoderId == RF_AMF)
        {
            m_pContextCL = std::unique_ptr<RFContextAMF>(new RFContextAMF);
        }
        else
        {
            m_pContextCL = std::unique_ptr<RFContextCL>(new RFContextCL);
        }

    }
    catch (const std::exception& e)
    {
        std::stringstream oss;

        oss << "[CreateContext]: Failed to create context: " << e.what();

        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());

        return (m_Properties.EncoderId == RF_AMF) ? RF_STATUS_AMF_FAIL : RF_STATUS_OPENCL_FAIL;
    }

    RFStatus rfStatus = createContextFromGfx();

    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateContext]: Failed to create context.", rfStatus);
    }

    return rfStatus;
}


RFStatus RFSession::createEncoder(unsigned int uiWidth, unsigned int uiHeight, const RFVideoCodec codec, const RFEncodePreset preset)
{
    RFStatus rfStatus;

    rfStatus = createEncoderConfig(uiWidth, uiHeight, codec, preset);

    if (rfStatus != RF_STATUS_OK)
    {
        return rfStatus;
    }

    // Create encoder and buffers.
    return createEncoder();
}


RFStatus RFSession::createEncoder(unsigned int uiWidth, unsigned int uiHeight, const RFProperties* properties)
{
    RFStatus rfStatus;

    rfStatus = createEncoderConfig(uiWidth, uiHeight, RF_VIDEO_CODEC_NONE, RF_PRESET_NONE);

    if (rfStatus != RF_STATUS_OK)
    {
        return rfStatus;
    }

    rfStatus = parseEncoderProperties(properties);

    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Error parsing encoder properties", rfStatus);

        return rfStatus;
    }

    // Create encoder and buffers.
    return createEncoder();
}


RFStatus RFSession::registerRenderTarget(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx)
{
    // Local lock: Make sure no other thread of this session is using the resources.
    RFReadWriteAccess enabler(&m_SessionLock);

    if (!rt.rfRT)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfRegisterRenderTarget] Handle of render target is invalid");
        return RF_STATUS_INVALID_RENDER_TARGET;
    }

    if (!uiWidth || !uiHeight)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfRegisterRenderTarget] Input render target width or height is zero");
        return RF_STATUS_INVALID_DIMENSION;
    }

    RFStatus rfStatus = registerTexture(rt, uiWidth, uiHeight, idx);

    std::stringstream oss;

    if (rfStatus != RF_STATUS_OK)
    {
        oss << "[rfRegisterRenderTarget] Failed to register texture " << rfStatus;

        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());

        return rfStatus;
    }

    if (m_Properties.uiInputDim[0] == 0)
    {
        m_Properties.uiInputDim[0] = uiWidth;
        m_Properties.uiInputDim[1] = uiHeight;
    }
    else if (m_Properties.uiInputDim[0] != uiWidth || m_Properties.uiInputDim[1] != uiHeight)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfRegisterRenderTarget] Render target width or height is not the same as previous registered render target");
        m_pContextCL->removeCLInputMemObj(idx);
        return RF_STATUS_INVALID_DIMENSION;
    }

    oss << "[rfRegisterRenderTarget]  Successfully registered Texture " << idx << " " << uiWidth << "x" << uiHeight;
    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, oss.str());

    return RF_STATUS_OK;
}


RFStatus RFSession::removeRenderTarget(unsigned int idx)
{
    // Local lock: Make sure no other thread of this session is using the resources
    RFReadWriteAccess enabler(&m_SessionLock);

    SAFE_CALL_RF(m_pContextCL->removeCLInputMemObj(idx));

    return RF_STATUS_OK;
}


RFStatus RFSession::getRenderTargetState(RFRenderTargetState* state, unsigned int idx) const
{
    if (!m_pContextCL || !m_pContextCL->isValid())
    {
        *state = RF_STATE_INVALID;
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    return m_pContextCL->getInputMemObjState(state, idx);
}


// The application may register one or more render targets and will call encodeFrame with the id
// of the registered RT. Internally more buffers are used e.g. to store frames needed for diff
// encoding. The submitted buffer ids are stored in m_BufferQueue.
// getEncodeFrame will remove the index from the queue once a frame is encoded and was read by
// the application.
RFStatus RFSession::encodeFrame(unsigned int idx)
{
    // Local lock: Make sure no other thread of this session is using the resources
    RFReadWriteAccess enabler(&m_SessionLock);

    // Check if we have a valid encoder. Having a valid encoder implies thet we have a valid
    // context as well.
    if (!m_pEncoder)
    {
        return RF_STATUS_INVALID_ENCODER;
    }

    if (idx >= m_pContextCL->getNumRegisteredRT())
    {
        return RF_STATUS_INVALID_INDEX;
    }

    // We don't have to free the result buffer. A call to m_pContextCL->processBuffer will override an
    // existing buffer. The app needs to call getEncodedFrame to free the buffers.
    if (m_BufferQueue.size() >= m_pContextCL->getNumResultBuffers())
    {
        return RF_STATUS_QUEUE_FULL;
    }

    // Run pre processor. This function might be implemented by a derived class like e.g. DesktopSession.
    // ATTENTION: idx might be changed by preprocessFrame to map on some internally created RTs.
    RFStatus rfStatus = preprocessFrame(idx);

    if (rfStatus != RF_STATUS_OK)
    {
        // Preprocessing failed -> we have no new data but if a frame is still in the reslut queue
        // we can return this frame without error notification to the application
        if (m_BufferQueue.size() > 0)
        {
            return RF_STATUS_OK;
        }

        return rfStatus;
    }

    // Processes input texture and stores it in the result buffer. The result buffer can be used as input
    // for the encoders. During this process the CSC can be done and the image can get inverted.
    // If a sys mem buffer was requested when createBuffers was called, a transfer of the result to sys
    // mem is triggered.
    SAFE_CALL_RF(m_pContextCL->processBuffer(m_Properties.bEncoderCSC, m_Properties.bInvertInput, idx, m_uiResultBuffer));

    // Encode frame
    SAFE_CALL_RF(m_pEncoder->encode(m_uiResultBuffer, !m_Properties.bEncoderCSC));

    // Store result buffer index in queue since processBuffer filled a new resultBuffer. The ResultBuffer
    // should only be considered as valid if the enode call succeeded. Only in this case a valid pair of
    // ResultBuffer and Enoced Buffer exist that then can be queried by the application.
    m_BufferQueue.push(m_uiResultBuffer);

    // Switch to next result buffer for new frame.
    m_uiResultBuffer = (m_uiResultBuffer + 1) % m_pContextCL->getNumResultBuffers();

    return RF_STATUS_OK;
}


RFStatus RFSession::getEncodedFrame(unsigned int& uiSize, void* &pBitStream)
{
    if (!m_pEncoder)
    {
        return RF_STATUS_INVALID_ENCODER;
    }

    uiSize = 0;
    pBitStream = nullptr;

    RFStatus status = RF_STATUS_OK;

    status = m_pEncoder->getEncodedFrame(uiSize, pBitStream);

    if (status == RF_STATUS_OK && m_BufferQueue.size() > 0)
    {
        // We got a frame encoded, remove index from buffer queue.
        m_BufferQueue.pop();
    }

    return status;
}


RFStatus RFSession::getSourceFrame(unsigned int& uiSize, void* &pBitStream)
{
    if (!m_pEncoder)
    {
        return RF_STATUS_INVALID_ENCODER;
    }

    uiSize = 0;
    pBitStream = nullptr;

    unsigned int idx = 0;

    {
        // Local lock: Ensure that m_BufferQueue is not accessed by other threads.
        // This is required if an async reader thread is used. In this case encodeFrame
        // might access the queue while we read the front index.
        RFReadWriteAccess enabler(&m_SessionLock);

        if (m_BufferQueue.size() == 0)
        {
            return RF_STATUS_NO_ENCODED_FRAME;
        }

        // Get index of the oldest element in the queue. This is the index that will be used for the next call to
        // get getEncodedFrame. If getSourceFrame is called prior to getEncoded frame the source frame is the one
        // that was used to generate the encoded frame.
        idx = m_BufferQueue.front();
    }

    void* pBuffer = nullptr;

    m_pContextCL->getResultBuffer(idx, pBuffer);

    if (pBuffer == nullptr)
    {
        return RF_STATUS_NO_ENCODED_FRAME;
    }

    uiSize = m_pContextCL->getResultBufferSize();

    pBitStream = pBuffer;

    return RF_STATUS_OK;
}


RFStatus RFSession::releaseEvent(RFNotification const rfEvent)
{
    // Local lock: Make sure no other thread of this session is using the resources.
    RFReadWriteAccess enabler(&m_SessionLock);

    return releaseSessionEvents(rfEvent);
}


RFStatus RFSession::resize(unsigned int uiWidth, unsigned int uiHeight)
{
    // Local lock: Make sure no other thread of this session is using the resources.
    RFReadWriteAccess enabler(&m_SessionLock);

    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, "Changing resolution");

    if (!m_pEncoder->isResizeSupported())
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "Resize not supported by encoder test");

        return RF_STATUS_FAIL;
    }

    // Free all OpenCL buffers.
    RFStatus rfStatus = m_pContextCL->deleteBuffers();

    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "Failed to delete CL buffers", rfStatus);
        return RF_STATUS_FAIL;
    }

    // Resize the encoder.
    m_pEncoderSettings->setDimension(uiWidth, uiHeight);


    rfStatus = m_pEncoder->resize(uiWidth, uiHeight);
    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "Failed to resize encoder", rfStatus);
        return RF_STATUS_FAIL;
    }

    // Create new buffers that match the aligned dimensions of the encoder.
    rfStatus = m_pContextCL->createBuffers(m_pEncoderSettings->getInputFormat(), m_pEncoderSettings->getEncoderWidth(), m_pEncoderSettings->getEncoderHeight(),
                                           m_pEncoder->getAlignedWidth(), m_pEncoder->getAlignedHeight(), m_Properties.bAsyncCopyToSysMem);
    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "Failed to resize encoder", rfStatus);
        return RF_STATUS_FAIL;
    }

    // Resize internal resources.
    rfStatus = resizeResources(uiWidth, uiHeight);

    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "Failed to resize resources", rfStatus);
        return RF_STATUS_FAIL;
    }

    m_Properties.uiInputDim[0] = 0;
    m_Properties.uiInputDim[1] = 0;

    std::stringstream oss;

    oss << "Changed resolution to " << uiWidth << " x " << uiHeight;

    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, oss.str());

    return RF_STATUS_OK;
}


RFStatus RFSession::getMouseData(int iWaitForShapeChange, RFMouseData& md) const
{
    return RF_STATUS_FAIL;
}


RFStatus RFSession::getMouseData2(int iWaitForShapeChange, RFMouseData2& md) const
{
    return RF_STATUS_FAIL;
}


RFStatus RFSession::setParameter(const int param, RFProperties value)
{
    // Set parameter and set protection since it was explicitly set by user
    // this prevents the session to override it.
    if (!m_ParameterMap.setParameterValue(param, value, true))
    {
        return RF_STATUS_INVALID_SESSION_PROPERTIES;
    }

    return RF_STATUS_OK;
}


RFStatus RFSession::getParameter(const int param, RFProperties& value) const
{
    int n;

    // To build 64 Bit the value needs to be read into a 32 Bit int and can then
    // be assigned to a 64 Bit RFProperties.
    if (!m_ParameterMap.getParameterValue<int>(param, n))
    {
        return RF_STATUS_INVALID_SESSION_PROPERTIES;
    }

    value = n;

    return RF_STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// private section
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


inline RFStatus RFSession::preprocessFrame(unsigned int& idx)
{
    // No preprocessing required for default session.
    return RF_STATUS_OK;
}


RFStatus RFSession::resizeResources(unsigned int uiWidth, unsigned int uiHeight)
{
    // No resource resizing required for default session.
    return RF_STATUS_OK;
}


RFStatus RFSession::releaseSessionEvents(RFNotification const rfEvent)
{
    // No events used for default session.
    return RF_STATUS_OK;
}


RFStatus RFSession::finalizeContext()
{
    return RF_STATUS_OK;
}


RFStatus RFSession::createEncoderConfig(unsigned int uiWidth, unsigned int uiHeight, const RFVideoCodec codec, const RFEncodePreset preset)
{
    if (!uiWidth || !uiHeight || uiWidth > 10000 || uiHeight > 10000)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Encoder width or height is out of range [1,10000]");
        return RF_STATUS_INVALID_DIMENSION;
    }

    if (m_pEncoderSettings)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_WARNING, "[rfCreateEncoder] Encoder settings already present");
    }

    // Create configuration to store all encoding parameters.
    m_pEncoderSettings.reset(new RFEncoderSettings);

    if (!m_pEncoderSettings->createSettings(uiWidth, uiHeight, codec, preset))
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Failed to create encoder settings");
        return RF_STATUS_INVALID_CONFIG;
    }

    return RF_STATUS_OK;
}


RFStatus RFSession::createEncoder()
{
    // Validate Session properties.
    m_ParameterMap.getParameterValue(RF_FLIP_SOURCE, m_Properties.bInvertInput);
    m_ParameterMap.getParameterValue(RF_ASYNC_SOURCE_COPY, m_Properties.bAsyncCopyToSysMem);
    m_ParameterMap.getParameterValue(RF_ENCODER_BLOCKING_READ, m_Properties.bBlockingEncoderRead);
    m_ParameterMap.getParameterValue(RF_MOUSE_DATA, m_Properties.bMousedata);

    RFStatus rfStatus = finalizeContext();

    if (rfStatus != RF_STATUS_OK)
    {
        return rfStatus;
    }

    // Check if a valid context exists.
    if (!m_pContextCL || !m_pContextCL->isValid())
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] No valid OpenCL context");
        return RF_STATUS_INVALID_OPENCL_CONTEXT;
    }

    if (m_pEncoder)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Encoder is already created");
        return RF_STATUS_FAIL;
    }

    // Create encoder.
    RFEncoder* pEncoder = nullptr;

    switch (m_Properties.EncoderId)
    {
        case RF_AMF:
            pEncoder = new (std::nothrow)RFEncoderAMF;

            if (!pEncoder)
            {
                m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Failed to create AMF encoder");
                return RF_STATUS_FAIL;
            }

            // The default is to use non-blocking read. If defined otherwise
            // set AMF encoder to block.
            if (m_Properties.bBlockingEncoderRead)
            {
                dynamic_cast<RFEncoderAMF*>(pEncoder)->setBlockingRead(true);
            }

            break;

        case RF_IDENTITY:
        {
            pEncoder = new (std::nothrow)RFEncoderIdentity;

            // Try to update parameter. If user set value explicitly this will fail.
            if (m_ParameterMap.setParameterValue(RF_ASYNC_SOURCE_COPY, 1))
            {
                m_Properties.bAsyncCopyToSysMem = true;
            }
            else if (m_Properties.bAsyncCopyToSysMem == false)
            {
                m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_WARNING, "[rfCreateEncoder] For best performance RF_ASYNC_SOURCE_COPY should be 1 but application requested to turn it off");
            }

            if (!pEncoder)
            {
                m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Failed to create IDENTITY encoder");
                return RF_STATUS_FAIL;
            }
            break;
        }

        case RF_DIFFERENCE:
        {
            pEncoder = new (std::nothrow)RFEncoderDM;
            if (!pEncoder)
            {
                m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Failed to create DIFFERENCE encoder");
                return RF_STATUS_FAIL;
            }
            break;
        }

        default:
            m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] No encoder defined");
            return RF_STATUS_INVALID_ENCODER;
            break;
    }

    m_pEncoder = std::unique_ptr<RFEncoder>(pEncoder);

    if (m_pEncoderSettings->getInputFormat() == RF_FORMAT_UNKNOWN)
    {
        m_pEncoderSettings->setFormat(m_pEncoder->getPreferredFormat());
    }

    if (m_pEncoderSettings->getVideoCodec() == RF_VIDEO_CODEC_NONE)
    {
        m_pEncoderSettings->setVideoCodec(m_pEncoder->getPreferredVideoCodec());
    }

    // Init encoder
    rfStatus = m_pEncoder->init(m_pContextCL.get(), m_pEncoderSettings.get());

    if (rfStatus != RF_STATUS_OK)
    {
        m_pEncoder.reset();
        m_pEncoder = nullptr;

        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Failed to init encoder", rfStatus);
        return rfStatus;
    }

    // Update m_pEncoderSettings with the values actually useed by the encoder.
    validateEncoderSettings();

    // The context was created when CreateSession was called. At that time the dimension of the
    // encoder and the format are not yet known. Create the required buffers based on the dimension now.
    rfStatus = m_pContextCL->createBuffers(m_pEncoderSettings->getInputFormat(),
                                           m_pEncoderSettings->getEncoderWidth(),
                                           m_pEncoderSettings->getEncoderHeight(),
                                           m_pEncoder->getAlignedWidth(),
                                           m_pEncoder->getAlignedHeight(),
                                           m_Properties.bAsyncCopyToSysMem);

    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[rfCreateEncoder] Failed to create OpenCL buffers", rfStatus);

        return rfStatus;
    }
    else
    {
        std::stringstream oss;

        oss << "[rfCreateEncoder] Created buffers. Dim " << m_pEncoderSettings->getEncoderWidth() << " x " << m_pEncoderSettings->getEncoderHeight() << "   Aligned Dim " << m_pEncoder->getAlignedWidth() << " x " << m_pEncoder->getAlignedHeight();
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, oss.str());
    }

    // Make sure the buffer queue is empty.
    while (m_BufferQueue.size() > 0)
    {
        m_BufferQueue.pop();
    }

    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, "[rfCreateEncoder] RFEncoder create successfully");

    dumpSessionProperties();

    dumpContextProperties();

    return RF_STATUS_OK;
}


void RFSession::createSessionLog()
{
    DWORD dwThreadId = GetCurrentThreadId();

    static unsigned int uiSessionCount = 0;

    char*   pEnvVar = nullptr;
    size_t  len = 0;

    _dupenv_s(&pEnvVar, &len, "RF_LOG_PATH");

    std::string strLogPath;

    if (len > 0 && pEnvVar)
    {
        strLogPath = std::string(pEnvVar);

        for (auto& c : strLogPath)
        {
            if (c == '\\')
            {
                c = '/';
            }
        }

        if (strLogPath.rfind('/') != strLogPath.size() - 1)
        {
            strLogPath += '/';
        }
    }

    free(pEnvVar);

    if (uiSessionCount == 0)
    {
        cleanLogFiles(strLogPath, "RFEncodeSession_");
    }
    ++uiSessionCount;

    std::stringstream oss;
    oss << strLogPath << "RFEncodeSession_" << uiSessionCount << "_" << dwThreadId << ".log";

    m_pSessionLog = std::unique_ptr<RFLogFile>(new RFLogFile(oss.str()));

    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, "[rfCreateEncodeSession] Create session");

    // Dump version info.
    std::string strPath;
    std::string strVersion;

    if (getModuleInformation(strPath, strVersion))
    {
        std::string strsMessage("[rfCreateEncodeSession] Rapid Fire DLL     : ");

        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, strsMessage + strPath);

        strsMessage = "[rfCreateEncodeSession] Rapid Fire version : ";

        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, strsMessage + strVersion);
    }

    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, "[rfCreateEncodeSession] Completed 1. step of session creation");
}


// Set a dynamic parameter.
RFStatus RFSession::setEncodeParameter(const int param, RFProperties value)
{
    if (!m_pEncoderSettings)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    if (!m_pEncoder)
    {
        return RF_STATUS_INVALID_ENCODER;
    }

    // Get the type and check if parameter exists. If it does not exist rfType will be RF_PARAMETER_UNKNOWN.
    RFParameterType rfType = m_pEncoderSettings->getParameterType(param);

    if (rfType == RF_PARAMETER_UNKNOWN)
    {
        return RF_STATUS_INVALID_ENCODER_PARAMETER;
    }

    // Local lock: Lock update of parameters.
    m_SessionLock.lock();

    RFStatus rfErr = m_pEncoder->setParameter(param, rfType, value);

    m_SessionLock.unlock();

    if (rfErr != RF_STATUS_OK)
    {
        return RF_STATUS_INVALID_ENCODER_PARAMETER;
    }

    // Since the encoder did not fail to update the parameter, update value and set state to READY.
    m_pEncoderSettings->setParameter(param, value, RF_PARAMETER_STATE_READY);

    return RF_STATUS_OK;
}


// The function will return the current parameter value of an encoder specific prameter.
// If the parameter name is invalid return RF_STATUS_INVALID_ENCODER_PARAMETER.
RFStatus RFSession::getEncodeParameter(const int param, RFProperties& value) const
{
    value = 0;

    if (!m_pEncoder)
    {
        return RF_STATUS_INVALID_ENCODER;
    }

    switch (param)
    {
        case RF_ENCODER_FORMAT:
            value = m_pEncoderSettings->getInputFormat();

            return RF_STATUS_OK;

        case RF_ENCODER_WIDTH:
            value = m_pEncoder->getWidth();

            return RF_STATUS_OK;

        case RF_ENCODER_HEIGHT:
            value = m_pEncoder->getHeight();

            return RF_STATUS_OK;

        case RF_ENCODER_OUTPUT_WIDTH:
            value = m_pEncoder->getOutputWidth();

            return RF_STATUS_OK;

        case RF_ENCODER_OUTPUT_HEIGHT:
            value = m_pEncoder->getOutputHeight();

            return RF_STATUS_OK;
    };

    RFProperties tmp;
    RFParameterState ParamState = m_pEncoderSettings->getValidatedParameterValue(param, tmp);

    if (ParamState == RF_PARAMETER_STATE_READY)
    {
        value = tmp;

        return RF_STATUS_OK;
    }
    else if (ParamState == RF_PARAMETER_STATE_BLOCKED)
    {
        value = tmp;

        return RF_STATUS_PARAM_ACCESS_DENIED;
    }

    return RF_STATUS_INVALID_ENCODER_PARAMETER;
}


// The EncoderSettings in m_pEncoderSettings contain all parameters known by RF. Not all encoders will
// support them and some of the parameters might be blocked. validateEncoderSettings will query the value
// and state from the encoder and update the m_pEncoderSettings.
// This functiom should be called once after the encoder was created.
void RFSession::validateEncoderSettings()
{
    if (m_pEncoderSettings && m_pEncoder)
    {
        RFProperties EncoderValue = 0;

        for (unsigned int i = 0, iEnd = m_pEncoderSettings->getNumSettings(); i < iEnd; ++i)
        {
            unsigned int     uiParamName;
            RFParameterState rfParamState;

            if (m_pEncoderSettings->getParameterName(i, uiParamName))
            {
                rfParamState = m_pEncoder->getParameter(uiParamName, m_pEncoderSettings->getVideoCodec(), EncoderValue);

                m_pEncoderSettings->setParameter(uiParamName, EncoderValue, rfParamState);

                if (rfParamState != RF_PARAMETER_STATE_INVALID)
                {
                    std::string strParamName;

                    if (m_pEncoderSettings->getParameterString(uiParamName, strParamName))
                    {
                        std::stringstream oss;

                        oss << "[Encoder Settings] " << strParamName << "    Value : " << EncoderValue << " State : ";
                        if (rfParamState == RF_PARAMETER_STATE_READY)
                        {
                            oss << "READY, parameter can be changed";
                        }
                        else if (rfParamState == RF_PARAMETER_STATE_BLOCKED)
                        {
                            oss << "BLOCKED, parameter is static";
                        }

                        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, oss.str());
                    }
                }
            }
        }
    }
}


bool RFSession::getModuleInformation(std::string& strPath, std::string& strVersion)
{
    size_t const    maxlength = 512;

#ifdef _WIN64
    HMODULE         hModule = GetModuleHandle("RapidFire64.dll");
#else
    HMODULE         hModule = GetModuleHandle("RapidFire.dll");
#endif

    if (!hModule)
    {
        return false;
    }

    char pModulepath[maxlength];

    DWORD dwRes = GetModuleFileName(hModule, pModulepath, maxlength);

    if (dwRes == 0 || (dwRes == maxlength && GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        return false;
    }

    strPath = std::string(pModulepath);

    DWORD dwVersionSize = GetFileVersionInfoSize(pModulepath, NULL);

    if (dwVersionSize == 0)
    {
        return false;
    }

    void* pVersionBlock = alloca(dwVersionSize);

    if (!GetFileVersionInfo(pModulepath, NULL, dwVersionSize, pVersionBlock))
    {
        return false;
    }

    VS_FIXEDFILEINFO* pFileInfo = nullptr;
    unsigned int      uiFileInfoLength = 0;

    if (!VerQueryValue(pVersionBlock, "\\", reinterpret_cast<LPVOID*>(&pFileInfo), &uiFileInfoLength) || uiFileInfoLength != sizeof(VS_FIXEDFILEINFO))
    {
        return false;
    }

    // Build version string
    std::stringstream oss;

    oss << HIWORD(pFileInfo->dwFileVersionMS) << "." << LOWORD(pFileInfo->dwFileVersionMS) << "." << HIWORD(pFileInfo->dwFileVersionLS) << "." << LOWORD(pFileInfo->dwFileVersionLS);

    strVersion = oss.str();

    return true;
}


///////////////////////////////////////////////////////////////////
// property parser
///////////////////////////////////////////////////////////////////


RFStatus RFSession::parseEncoderProperties(const RFProperties* props)
{
    if (!m_pEncoderSettings)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    if (props)
    {
        const struct Element
        {
            int             name;
            RFProperties    ptr;
        };

        const Element* p = reinterpret_cast<const Element*>(props);

        while (p->name != 0)
        {
            if (p->name == RF_ENCODER_FORMAT)
            {
                m_pEncoderSettings->setFormat(static_cast<RFFormat>(p->ptr));
            }
            else if (p->name == RF_ENCODER_CODEC)
            {
                m_pEncoderSettings->setVideoCodec(static_cast<RFVideoCodec>(p->ptr));
            }
            // INVALID indicates that the parameter was not yet validated by an encoder.
            else if (!m_pEncoderSettings->setParameter(p->name, p->ptr, RF_PARAMETER_STATE_INVALID))
            {
                std::stringstream oss;

                oss << "Failed to set parameter " << std::hex << "0x" << p->name;

                m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());
                return RF_STATUS_INVALID_ENCODER_PARAMETER;
            }
            ++p;
        }
    }

    return RF_STATUS_OK;
}


void RFSession::dumpSessionProperties()
{
    std::stringstream oss;

    oss << "Session properties" << std::endl;

    oss << "\t\t\tEncoder :" << m_pEncoder->getName() << std::endl;

    for (const auto& p : m_ParameterMap)
    {
        RFProperties v = p.second.getRawValue();

        if (p.second.getType() == RF_PARAMETER_PTR)
        {
            oss << "\t\t\t" << p.second.getName() << " : 0x" << std::hex << static_cast<unsigned int>(v);
            oss << std::endl;
        }
        else
        {
            oss << "\t\t\t" << p.second.getName() << " : " << static_cast<unsigned int>(v);
            oss << std::endl;
        }
    }

    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, oss.str());
}


void RFSession::dumpContextProperties()
{
    if (m_pContextCL && m_pContextCL->isValid())
    {
        std::stringstream oss;

        oss << "Context properties" << std::endl;

        oss << "\t\t\t Target Format : ";

        switch (m_pContextCL->getTargetFormat())
        {
            case RF_RGBA8:
                oss << "RF_RGBA8";
                break;

            case RF_BGRA8:
                oss << "RF_BGRA8";
                break;

            case RF_NV12:
                oss << "RF_NV12";
                break;

            default:
                oss << "RF_UNKNOWN";
                break;
        }
        oss << std::endl;

        oss << "\t\t\t Dimension  : " << m_pContextCL->getOutputWidth() << " x " << m_pContextCL->getOutputHeight() << std::endl;

        oss << "\t\t\t Async Copy : " << m_pContextCL->getAsyncCopy() << std::endl;

        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, oss.str());
    }
}