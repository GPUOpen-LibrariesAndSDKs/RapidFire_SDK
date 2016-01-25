#include "RFDOPPSession.h"

#ifdef _DEBUG
//#include <vld.h>
#endif

#include "DOPPDrv.h"

#include "RFError.h"
#include "RFEncoderSettings.h"
#include "RFMouseGrab.h"
#include "RFGLDOPPCapture.h"


// The DOPP session will crteate its own OpenGL context. Currently it is not supported to use a context that
// is passed by the application since glew_init (used by GLDOPPCapture) fails if a core context is used.
// The RFSessionFactory prevents creating a DOPPSession if an OpenGL context is in the attribute list.
RFDOPPSession::RFDOPPSession(RFEncoderID rfEncoder)
    : RFSession(rfEncoder)
    , m_hDC(NULL)
    , m_hGlrc(NULL)
    , m_bMouseShapeData(false)
    , m_bBlockUntilChange(false)
    , m_bUpdateOnlyOnChange(false)
    , m_uiDisplayId(0)
    , m_pDeskotpCapture(nullptr)
    , m_pDrvInterface(nullptr)
    , m_pMouseGrab(nullptr)
{
    try
    {
        // Add all know parameters to the map.
        m_ParameterMap.addParameter(RF_DESKTOP, RFParameterAttr("RF_DESKTOP", RF_PARAMETER_UINT, 0));
        m_ParameterMap.addParameter(RF_DESKTOP_DSP_ID, RFParameterAttr("RF_DESKTOP_DSP_ID", RF_PARAMETER_UINT, 0));
        m_ParameterMap.addParameter(RF_DESKTOP_BLOCK_UNTIL_CHANGE, RFParameterAttr("RF_DESKTOP_BLOCK_UNTIL_CHANGE", RF_PARAMETER_BOOL, 0));
        m_ParameterMap.addParameter(RF_DESKTOP_UPDATE_ON_CHANGE, RFParameterAttr("RF_DESKTOP_UPDATE_ON_CHANGE", RF_PARAMETER_BOOL, 0));
        m_ParameterMap.addParameter(RF_MOUSE_DATA, RFParameterAttr("RF_MOUSE_DATA", RF_PARAMETER_BOOL, 0));
    }
    catch (...)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[CreateSession] Failed to create DOPP Parameters.");

        throw std::runtime_error("Failed to create DOPP Parameters.");
    }
}


RFDOPPSession::~RFDOPPSession()
{
    RFGLContextGuard glGuard(m_hDC, m_hGlrc);

    // Delete GLDOPPCapture class while we have a valid Ctx.
    m_pDeskotpCapture.reset(nullptr);

    wglMakeCurrent(NULL, NULL);

    if (m_hGlrc)
    {
        wglDeleteContext(m_hGlrc);
        m_hGlrc = NULL;
    }

    if (m_hDC)
    {
        DeleteDC(m_hDC);
        m_hDC = NULL;
    }

    if (m_strClassName.size() > 0)
    {
        if (UnregisterClass(m_strClassName.c_str(), NULL) == 0)
        {
            std::stringstream oss;

            oss << "Failed to unregister class " << m_strClassName << " Error Code " << GetLastError();

            m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());
        }
    }
}


RFStatus RFDOPPSession::createContextFromGfx()
{
    DisplayManager dpManager;

    if (dpManager.enumDisplays() == 0)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP Create context] No mapped displays found");
        return RF_STATUS_FAIL;
    }

    dumpDspInfo(dpManager);

    unsigned int uiCCCDesktopId = 0;
    unsigned int uiWinDisplayId = 0;

    // The application can either specify a desktop or a display. The factory will make sure that only 
    // one of the two values is set.
    m_ParameterMap.getParameterValue(RF_DESKTOP, uiCCCDesktopId);
    m_ParameterMap.getParameterValue(RF_DESKTOP_DSP_ID, uiWinDisplayId);

    // Application defined a windows display ID. Check if the DisplayManager knows this ID and
    // get corresponding desktop ID.
    if (uiWinDisplayId > 0)
    {
        if (!dpManager.getDisplayIdFromWinID(uiWinDisplayId, m_uiDisplayId))
        {
            std::stringstream oss;

            oss << "[DOPP Create context] " << uiWinDisplayId << " is an invalid Windows Display ID";

            m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());
            return RF_STATUS_INVALID_DESKTOP_ID;
        }
    }
    else if (uiCCCDesktopId > 0)
    {
        if (!dpManager.getDisplayIdFromCCCID(uiCCCDesktopId, m_uiDisplayId))
        {
            std::stringstream oss;

            oss << "[DOPP Create context] " << uiCCCDesktopId << " is an invalid Desktop ID";

            m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());
            return RF_STATUS_INVALID_DESKTOP_ID;
        }
    }
    else
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP Create context]: No display or desktop specified ");
        return RF_STATUS_INVALID_DESKTOP_ID;
    }

    m_strDisplayName = dpManager.getDisplayName(m_uiDisplayId);
    m_strPrimaryDisplayName = dpManager.getDisplayName(dpManager.getPrimaryDisplay());

    if (m_strPrimaryDisplayName.size() == 0)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP Create context]: No primary display found ");
        return RF_STATUS_INVALID_DESKTOP_ID;
    }

    unsigned int uiBusNumber = dpManager.getBusNumber(m_uiDisplayId);

    m_ParameterMap.getParameterValue(RF_MOUSE_DATA, m_bMouseShapeData);

    try
    {
        std::unique_ptr<DOPPDrvInterface> pDoppDrv = std::unique_ptr<DOPPDrvInterface>(new DOPPDrvInterface(m_strDisplayName, uiBusNumber));

        std::unique_ptr<GLDOPPCapture>    pDoppCapture = std::unique_ptr<GLDOPPCapture>(new GLDOPPCapture(dpManager.getDesktopId(m_uiDisplayId), pDoppDrv.get()));

        if (m_bMouseShapeData)
        {
            m_pMouseGrab = std::unique_ptr<RFMouseGrab>(new RFMouseGrab(pDoppDrv.get(), m_uiDisplayId));
        }

        m_pDrvInterface = std::move(pDoppDrv);
        m_pDeskotpCapture = std::move(pDoppCapture);
    }

    catch (const std::exception& e)
    {
        std::stringstream oss;

        oss << "[DOPP Create context] Context failed for GPU with Bus Number: " << uiBusNumber << " Displayname: " << m_strDisplayName << " Reason: " << e.what();
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, oss.str());

        return RF_STATUS_DOPP_FAIL;
    }

    RFStatus rfStatus;

    {
        // Store a context that might have been bound by the application.
        RFGLContextGuard glCtxGuard;

        std::stringstream oss;

        oss << "RFClass" << GetCurrentThreadId() << m_uiDisplayId;
        m_strClassName = oss.str();

        // Create the OpenGL context that is used for DOPP.
        if (!createGLContext())
        {
            m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP Create context]: Failed to create GL context");
            return RF_STATUS_OPENGL_FAIL;
        }

        // Create the OpenCL context that shares resources with OpenGL.
        rfStatus = m_pContextCL->createContext(m_hDC, m_hGlrc);
    }

    return rfStatus;
}


RFStatus RFDOPPSession::finalizeContext()
{
    RFGLContextGuard glGuard(m_hDC, m_hGlrc);

    if (!m_pDeskotpCapture)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP context]: No valid context");
        return RF_STATUS_DOPP_FAIL;
    }

    m_ParameterMap.getParameterValue(RF_DESKTOP_BLOCK_UNTIL_CHANGE, m_bBlockUntilChange);
    m_ParameterMap.getParameterValue(RF_DESKTOP_UPDATE_ON_CHANGE, m_bUpdateOnlyOnChange);

    RFStatus rfStatus = m_pDeskotpCapture->initDOPP(m_pEncoderSettings->getEncoderWidth(), m_pEncoderSettings->getEncoderHeight(), 0.0f, m_bUpdateOnlyOnChange, m_bBlockUntilChange);

    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP context] Failed to init Desktop capturing", rfStatus);

        return rfStatus;
    }

    m_DesktopRTIndexList.resize(m_pDeskotpCapture->getNumFramebufferTex());

    for (unsigned int i = 0, iEnd = m_pDeskotpCapture->getNumFramebufferTex(); i < iEnd; ++i)
    {
        // RFGLDOPPCapture will create a render target of the size m_pEncoderSettings->getEncoderWidth(), m_pEncoderSettings->getEncoderHeight()
        m_Properties.uiInputDim[0] = m_pEncoderSettings->getEncoderWidth();
        m_Properties.uiInputDim[1] = m_pEncoderSettings->getEncoderHeight();

        rfStatus = m_pContextCL->setInputTexture(m_pDeskotpCapture->getFramebufferTex(i), m_Properties.uiInputDim[0], m_Properties.uiInputDim[1], m_DesktopRTIndexList[i]);

        if (rfStatus != RF_STATUS_OK)
        {
            m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP context] Failed to add input texture to CL context", rfStatus);

            return rfStatus;
        }
    }

    return RF_STATUS_OK;
}


RFStatus RFDOPPSession::resizeResources(unsigned int w, unsigned int h)
{
    RFGLContextGuard glGuard(m_hDC, m_hGlrc);

    if (!glGuard.isContextBound())
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP resize] Failed to bind GL context", RF_STATUS_OPENGL_FAIL);

        return RF_STATUS_OPENGL_FAIL;
    }

    // Resize the desktop texture to the dimension of the desktop.
    RFStatus rfStatus = m_pDeskotpCapture->resizeDesktopTexture();

    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP resize] Failed to resize desktop texture", rfStatus);

        return rfStatus;
    }

    // Resize the present texture to the new dimension.
    rfStatus = m_pDeskotpCapture->resizePresentTexture(w, h);

    if (rfStatus != RF_STATUS_OK)
    {
        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP resize] Failed to resize present texture", rfStatus);

        return rfStatus;
    }

    m_DesktopRTIndexList.resize(m_pDeskotpCapture->getNumFramebufferTex());

    // Register new render targets
    for (unsigned int i = 0, iEnd = m_pDeskotpCapture->getNumFramebufferTex(); i < iEnd; ++i)
    {
        // RFGLDOPPCapture will create a render target of the size m_pEncoderSettings->getEncoderWidth(), m_pEncoderSettings->getEncoderHeight()
        m_Properties.uiInputDim[0] = m_pEncoderSettings->getEncoderWidth();
        m_Properties.uiInputDim[1] = m_pEncoderSettings->getEncoderHeight();

        rfStatus = m_pContextCL->setInputTexture(m_pDeskotpCapture->getFramebufferTex(i), m_Properties.uiInputDim[0], m_Properties.uiInputDim[1], m_DesktopRTIndexList[i]);

        if (rfStatus != RF_STATUS_OK)
        {
            m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "Failed to add input texture to CL context on Resize", rfStatus);

            return rfStatus;
        }
    }

    return RF_STATUS_OK;
}


RFStatus RFDOPPSession::registerTexture(RFTexture rt, unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx)
{
    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_ERROR, "[DOPP context] Adding RenderTargets is not supported");

    return RF_STATUS_FAIL;
}


RFStatus RFDOPPSession::getMouseData(bool bWaitForShapeChange, RFMouseData& md) const
{
    if (!m_pMouseGrab)
    {
        return RF_STATUS_FAIL;
    }

    if (!m_pMouseGrab->getShapeData(bWaitForShapeChange, md))
    {
        return RF_STATUS_MOUSEGRAB_NO_CHANGE;
    }

    return RF_STATUS_OK;
}


RFStatus RFDOPPSession::releaseSessionEvents(const RFNotification rfEvent)
{
    if (rfEvent == RFDesktopNotification)
    {
        if (m_pDeskotpCapture->releaseEvent())
        {
            return RF_STATUS_OK;
        }
    }
    else if (rfEvent == RFMouseShapeNotification && m_pMouseGrab)
    {
        if (m_pMouseGrab->releaseEvent())
        {
            return RF_STATUS_OK;
        }
    }

    return RF_STATUS_FAIL;
}

RFStatus RFDOPPSession::preprocessFrame(unsigned int& idx)
{
    RFGLContextGuard glGuard(m_hDC, m_hGlrc);

    // Render desktop to image.
    if (!m_pDeskotpCapture->processDesktop(idx))
    {
        return RF_STATUS_DOPP_NO_UPDATE;
    }

    idx = m_DesktopRTIndexList[idx];

    return RF_STATUS_OK;
}


void MyDebugFunc(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
#ifdef _DEBUG
    __debugbreak();
#endif
}


// Creates a DC and an OpenGL context that is used for DOPP.
bool RFDOPPSession::createGLContext()
{
    if (m_hGlrc != NULL || m_hDC != NULL || m_strDisplayName.size() == 0)
    {
        return false;
    }

    // Register WindowClass
    WNDCLASSEX wndclass = {};
    wndclass.cbSize        = sizeof(WNDCLASSEX);
    wndclass.style         = CS_OWNDC;
    wndclass.lpfnWndProc   = DefWindowProc;
    wndclass.hInstance     = static_cast<HINSTANCE>(GetModuleHandle(NULL));
    wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = m_strClassName.c_str();
    wndclass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wndclass))
    {
        // Clear class name to avoid that destructor unregisters.
        m_strClassName.clear();

        return false;
    }

    // Create a dummy window in order to create a context.
    int mPixelFormat;

    static PIXELFORMATDESCRIPTOR pfd = {};

    pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 24;
    pfd.cRedBits     = 8;
    pfd.cGreenBits   = 8;
    pfd.cBlueBits    = 8;
    pfd.cAlphaBits   = 8;
    pfd.cDepthBits   = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType   = PFD_MAIN_PLANE;

    // For now only one GPU inside a VM is supported so it is safe to open the ctx on the primary GPU.
    // Opening the ctx on the actual display (m_strDisplayName) works as well but currently some 
    // application will unmap a display while a session is still running. This causess problems since
    // the OpenGL ctx gets invalid. To avoid this we remain on the primary GPU until applications will
    // synchronize to unmap only once the session is deleted.
    m_hDC = CreateDC(NULL, m_strPrimaryDisplayName.c_str(), NULL, nullptr);

    if (!m_hDC)
    {
        return false;
    }

    mPixelFormat = ChoosePixelFormat(m_hDC, &pfd);

    if (!mPixelFormat)
    {
        return false;
    }

    if (!SetPixelFormat(m_hDC, mPixelFormat, &pfd))
    {
        return false;
    }

    m_hGlrc = wglCreateContext(m_hDC);

    if (!wglMakeCurrent(m_hDC, m_hGlrc))
    {
        return false;
    }

    if (glewInit() != GLEW_OK)
    {
        return false;
    }

    if (WGLEW_ARB_create_context)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(m_hGlrc);

        int attribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                          WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                          WGL_CONTEXT_PROFILE_MASK_ARB , WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef _DEBUG             
                          WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif                    
                          0 };

        m_hGlrc = wglCreateContextAttribsARB(m_hDC, NULL, attribs);

        if (m_hGlrc)
        {
            wglMakeCurrent(m_hDC, m_hGlrc);
        }
        else
        {
            return false;
        }
    }

    if (!wglMakeCurrent(m_hDC, m_hGlrc))
    {
        return false;
    }

    if (GLEW_AMD_debug_output)
    {
        glDebugMessageCallbackAMD(reinterpret_cast<GLDEBUGPROCAMD>(&MyDebugFunc), nullptr);
    }

    return true;
}


void RFDOPPSession::dumpDspInfo(const DisplayManager& dpManager)
{
    // Dump display info to log file.
    std::stringstream       oss;
    unsigned int            uiNumDisplays = dpManager.getNumDisplays();
    unsigned int            uiPrimaryId   = dpManager.getPrimaryDisplay();

    oss << "Number of displays : " << uiNumDisplays << std::endl;

    m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, oss.str());

    for (unsigned int i = 0; i < uiNumDisplays; ++i)
    {
        oss.str(std::string());
        oss << "Display " << i << std::endl;
        oss << "\t\t\tDisplay ID            : " << i << std::endl;
        oss << "\t\t\tPrimary               : " << ((uiPrimaryId == i) ? "TRUE" : "FALSE") << std::endl;;
        oss << "\t\t\tWindows Display Name  : " << dpManager.getDisplayName(i) << std::endl;
        oss << "\t\t\tWindows Display ID    : " << dpManager.getWindowsDisplayId(i) << std::endl;
        oss << "\t\t\tDesktop ID            : " << dpManager.getDesktopId(i) << std::endl;
        oss << "\t\t\tOrientation           : " << dpManager.getDesktopRotation(i) << std::endl;
        oss << "\t\t\tMonitor Name          : " << dpManager.getMonitorName(i) << std::endl;
        oss << "\t\t\tGPU ID                : " << dpManager.getGpuId(i) << std::endl;
        oss << "\t\t\tBUS Number            : " << dpManager.getBusNumber(i) << std::endl;
        oss << "\t\t\tOrigin                : " << dpManager.getOriginX(i) << "x" << dpManager.getOriginY(i) << std::endl;
        oss << "\t\t\tDimension             : " << dpManager.getWidth(i) << "x" << dpManager.getHeight(i) << std::endl;

        m_pSessionLog->logMessage(RFLogFile::MessageType::RF_LOG_INFO, oss.str());
    }
}