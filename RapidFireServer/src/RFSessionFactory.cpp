#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <iostream>
#include <map>
#include <stdlib.h>

#include "RFDOPPSession.h"
#include "RFGfxSession.h"


RFStatus createRFSession(RFSession** pSession, const RFProperties* properties)
{
    if (!properties)
    {
        *pSession = nullptr;
        return RF_STATUS_INVALID_SESSION_PROPERTIES;
    }

    std::map<int, RFProperties> parameters;

    // parse properties and fill m_Parameters
    struct Element
    {
        int             name;
        RFProperties    ptr;
    };
    
    const Element* p = reinterpret_cast<const Element*>(properties);


    HGLRC                   hGLRC  = NULL;
    HDC                     hDC    = NULL;
    IDirect3DDevice9*       pDX9   = nullptr;
    IDirect3DDevice9Ex*     pDX9Ex = nullptr;
    ID3D11Device*           pDX11  = nullptr;

    unsigned int            uiDesktop = 0;
    unsigned int            uiDisplay = 0;

    RFEncoderID             rfEncoder = RF_ENCODER_UNKNOWN;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // Loop through the property list that was defined by the application. The following properties
    // need to be defined:
    // 1: Encoder used by the session
    // 2: Source for capturing. This can be one of the following:
    //          a. OpenGL context -> RF_GL_GRAPHICS_CTX and RF_GL_DEVICE_CTX need to be defined
    //          b. Dx9            -> RF_D3D9_DEVICE needs to be set
    //          c. Dx9Ex          -> RF_D3D9EX_DEVICE needs to be set
    //          d. Dx11           -> RF_D3D11_DEVICE needs to be set
    //          e. Desktop        -> RF_DESKTOP or RF_DESKTOP_DSP_ID need to be set
    //
    // All remaining properties are optional and are passed to the session. Depending on the session
    // type different parameters are supported
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    while (p->name != 0)
    {
        parameters[p->name] = p->ptr;

        switch (p->name)
        {
            case RF_ENCODER:
                rfEncoder = static_cast<RFEncoderID>(p->ptr);
                break;

            case RF_GL_GRAPHICS_CTX:
                hGLRC = reinterpret_cast<HGLRC>(p->ptr);
                break;

            case RF_GL_DEVICE_CTX:
                hDC = reinterpret_cast<HDC>(p->ptr);
                break;

            case RF_D3D9_DEVICE:
                pDX9 = reinterpret_cast<IDirect3DDevice9*>(p->ptr);
                break;

            case RF_D3D9EX_DEVICE:
                pDX9Ex = reinterpret_cast<IDirect3DDevice9Ex*>(p->ptr);
                break;

            case RF_D3D11_DEVICE:
                pDX11 = reinterpret_cast<ID3D11Device*>(p->ptr);
                break;

            case RF_DESKTOP:
                uiDesktop = static_cast<unsigned int>(p->ptr);
                break;

            case RF_DESKTOP_DSP_ID:
                uiDisplay = static_cast<unsigned int>(p->ptr);
                break;

            default:
                parameters[p->name] = p->ptr;
        }

        ++p;
    }

    if (rfEncoder == RF_ENCODER_UNKNOWN)
    {
        *pSession = nullptr;
        return RF_STATUS_INVALID_ENCODER;
    }

    try
    {
        // Make sure we have a valid session description
        if (hDC && hGLRC && pDX9 == nullptr && pDX9Ex == nullptr && pDX11 == nullptr && uiDesktop == 0 && uiDisplay == 0)
        {
            // GL Session
            *pSession = new RFGLSession(hDC, hGLRC, rfEncoder);
        }
        else if (pDX9 && hDC == NULL && hGLRC == NULL && pDX9Ex == nullptr && pDX11 == nullptr && uiDesktop == 0 && uiDisplay == 0)
        {
            // DX9 Session
            *pSession = new RFDX9Session(pDX9, rfEncoder);
        }
        else if (pDX9Ex && hDC == NULL && hGLRC == NULL && pDX9 == nullptr && pDX11 == nullptr && uiDesktop == 0 && uiDisplay == 0)
        {
            // DX9 Ex Session
            *pSession = new RFDX9Session(pDX9Ex, rfEncoder);
        }
        else if (pDX11 && hDC == NULL && hGLRC == NULL && pDX9 == nullptr && pDX9Ex == nullptr && uiDesktop == 0 && uiDisplay == 0)
        {
            // DX11 Session
            *pSession = new RFDX11Session(pDX11, rfEncoder);
        }
        else if (uiDesktop > 0 && uiDisplay == 0 && hDC == NULL && hGLRC == NULL && pDX9 == nullptr && pDX9Ex == nullptr && pDX11 == nullptr)
        {
            // Desktop session based on Desktop ID
            *pSession = new RFDOPPSession(rfEncoder);
        }
        else if (uiDisplay > 0 && uiDesktop == 0 && hDC == NULL && hGLRC == NULL && pDX9 == nullptr && pDX9Ex == nullptr && pDX11 == nullptr)
        {
            // Desktop session based on Display ID
            *pSession = new RFDOPPSession(rfEncoder);
        }
    }
    catch (...)
    {
        *pSession = nullptr;
        return RF_STATUS_FAIL;
    }

    if (!*pSession)
    {
        return RF_STATUS_INVALID_SESSION_PROPERTIES;
    }

    // Copy remaining optional parameters to the session parameter map
    for (const auto& p : parameters)
    {
        if ((*pSession)->setParameter(p.first, p.second) != RF_STATUS_OK)
        {
            delete *pSession;
            *pSession = nullptr;

            return RF_STATUS_INVALID_SESSION_PROPERTIES;
        }
    }

    return RF_STATUS_OK;
}