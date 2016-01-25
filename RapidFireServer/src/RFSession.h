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

#include <memory>
#include <queue>

#include "RFContext.h"
#include "RFEncoder.h"
#include "RFLock.h"
#include "RFPropertyMap.h"

class RFEncoderSettings;
class RFMouseGrab;
class RFLogFile;

class RFSession
{
public:

    explicit RFSession(RFEncoderID rfEncoder);
    virtual ~RFSession();

    // Creates the graphics context that is used by the calling application and
    // the OpenCL context that is required for the color space conversion.
    RFStatus              createContext();

    // Creates an encoder and initializes buffers of the OpenCL context.
    RFStatus              createEncoder(unsigned int uiWidth, unsigned int uiHeight, const RFEncodePreset preset);
    RFStatus              createEncoder(unsigned int uiWidth, unsigned int uiHeight, const RFProperties* properties);

    RFStatus              registerRenderTarget(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx);

    RFRenderTargetState   getRenderTargetState(unsigned int idx)  const;

    RFStatus              removeRenderTarget(unsigned int idx);

    // Encodes the OpenCL input buffer.
    RFStatus              encodeFrame(unsigned int idx);

    // Returns the encoded frame.
    RFStatus              getEncodedFrame(unsigned int& uiSize, void* &pBitStream);

    RFStatus              getSourceFrame(unsigned int& uiSize, void* &pBitStream);

    RFStatus              releaseEvent(const RFNotification rfEvent);

    RFStatus              resize(unsigned int uiWidth, unsigned int uiHeight);

    RFStatus              setParameter(const int param, RFProperties value);

    RFStatus              getParameter(const int param, RFProperties& value) const;

    // Change a dynamic encoding parameter.
    RFStatus              setEncodeParameter(const int param, RFProperties value);

    // Return the value of an encoder specific parameter.
    RFStatus              getEncodeParameter(const int param, RFProperties& value) const;

    // Can be implemented by a derived class to give access to mouse shape data.
    virtual RFStatus      getMouseData(bool bWaitForShapeChange, RFMouseData& md) const;

protected:

    struct RFSessionProperties
    {
        RFEncoderID     EncoderId;
        unsigned int    uiInputDim[2];
        bool            bInvertInput;
        bool            bAsyncCopyToSysMem;
        bool            bBlockingEncoderRead;
        bool            bMousedata;
    };

    RFSessionProperties                   m_Properties;

    RFParameterMap                        m_ParameterMap;

    // OpenCL Context used for CSC        
    std::unique_ptr<RFContextCL>          m_pContextCL;

    // List of all encoder settings that are known by the session
    std::unique_ptr<RFEncoderSettings>    m_pEncoderSettings;

    std::unique_ptr<RFLogFile>            m_pSessionLog;

private:

    // This function needs to be implemented by a derived class to create the OpenCL context based
    // on the GFX context. The function is called by createContext().
    virtual RFStatus            createContextFromGfx() = 0;

    // This function needs to be implemented by a derived class to allow texture registration based on the used
    // Gfx context. The function is called by registerRenderTarget.
    virtual RFStatus            registerTexture(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx) = 0;

    // This function might be implemented by a derived class to do some preprocessing prior to the actual
    // processing (CSC and Encoding) of the frame.
    virtual RFStatus            preprocessFrame(unsigned int& idx);

    virtual RFStatus            finalizeContext();

    // This function might be implemented by a derived class to resize internally created resources.
    virtual RFStatus            resizeResources(unsigned int uiWidth, unsigned int uiHeight);

    // This function might be implemented by a derived class to release events, semaphores etc that were used and 
    // might block an application.
    virtual RFStatus            releaseSessionEvents(const RFNotification rfEvent);

    RFStatus                    createEncoderConfig(unsigned int uiWidth, unsigned int uiHeight, const RFEncodePreset preset);

    void                        createSessionLog();

    // Parse encoder properties and store them in m_pEncoderSettings.
    RFStatus                    parseEncoderProperties(const RFProperties* props);

    // Validate encoder settings. Not all settings are accepted by all encoders. validateEncoderSettings will loop through
    // all known settings and check which value the encoder is using.
    void                        validateEncoderSettings();

    RFStatus                    createEncoder();

    // Returns the path to the RF DLL that was loaded and the version of the DLL
    bool                        getModuleInformation(std::string& strPath, std::string& strVersion);

    void                        dumpSessionProperties();
    void                        dumpContextProperties();

    // Index of the buffer into which the source is processed (ResultBuffer of RFContextCL)
    unsigned int                                    m_uiResultBuffer;

    // The encoder that is used by the session
    std::unique_ptr<RFEncoder>                      m_pEncoder;

    // List of submitted buffers
    RFLockedQueue<unsigned int>                     m_BufferQueue;

    RFLock                                          m_SessionLock;
};

extern RFStatus createRFSession(RFSession** session, const RFProperties* properties);