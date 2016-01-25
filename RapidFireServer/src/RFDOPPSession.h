#pragma once

#include "DisplayManager.h"
#include "RFSession.h"

#define DOPP_NUM_RT     3

class GLDOPPCapture;
class DOPPDrvInterface;
class RFMouseGrab;


class RFDOPPSession : public RFSession
{
public:

    explicit RFDOPPSession(RFEncoderID rfEncoder);
    ~RFDOPPSession();

private:

    virtual RFStatus    createContextFromGfx()  override;

    virtual RFStatus    finalizeContext()       override;

    virtual RFStatus    resizeResources(unsigned int w, unsigned int h)         override;

    virtual RFStatus    registerTexture(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx)    override;

    virtual RFStatus    preprocessFrame(unsigned int& idx)                      override;

    virtual RFStatus    releaseSessionEvents(RFNotification const rfEvent)      override;

    virtual RFStatus    getMouseData(bool bWaitForShapeChange, RFMouseData& md) const override;

    bool                createGLContext();
    void                dumpDspInfo(const DisplayManager& dpManager);


    HDC                                     m_hDC;
    HGLRC                                   m_hGlrc;

    bool                                    m_bMouseShapeData;
    bool                                    m_bBlockUntilChange;
    bool                                    m_bUpdateOnlyOnChange;

    unsigned int                            m_uiDisplayId;

    std::vector<unsigned int>               m_DesktopRTIndexList;

    std::string                             m_strDisplayName;
    std::string                             m_strPrimaryDisplayName;
    std::string                             m_strClassName;

    std::unique_ptr<GLDOPPCapture>          m_pDeskotpCapture;
    std::unique_ptr<DOPPDrvInterface>       m_pDrvInterface;
    std::unique_ptr<RFMouseGrab>            m_pMouseGrab;
};