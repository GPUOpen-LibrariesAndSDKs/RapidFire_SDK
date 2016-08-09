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
    unsigned int            uiInternalDisplayId = UINT_MAX;

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

            case RF_DESKTOP_INTERNAL_DSP_ID:
                uiInternalDisplayId = static_cast<unsigned int>(p->ptr);
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
        if (hDC && hGLRC && pDX9 == nullptr && pDX9Ex == nullptr && pDX11 == nullptr && uiDesktop == 0 && uiDisplay == 0 && uiInternalDisplayId == UINT_MAX)
        {
            // GL Session
            *pSession = new RFGLSession(hDC, hGLRC, rfEncoder);
        }
        else if (pDX9 && hDC == NULL && hGLRC == NULL && pDX9Ex == nullptr && pDX11 == nullptr && uiDesktop == 0 && uiDisplay == 0 && uiInternalDisplayId == UINT_MAX)
        {
            // DX9 Session
            *pSession = new RFDX9Session(pDX9, rfEncoder);
        }
        else if (pDX9Ex && hDC == NULL && hGLRC == NULL && pDX9 == nullptr && pDX11 == nullptr && uiDesktop == 0 && uiDisplay == 0 && uiInternalDisplayId == UINT_MAX)
        {
            // DX9 Ex Session
            *pSession = new RFDX9Session(pDX9Ex, rfEncoder);
        }
        else if (pDX11 && hDC == NULL && hGLRC == NULL && pDX9 == nullptr && pDX9Ex == nullptr && uiDesktop == 0 && uiDisplay == 0 && uiInternalDisplayId == UINT_MAX)
        {
            // DX11 Session
            *pSession = new RFDX11Session(pDX11, rfEncoder);
        }
        else if (uiDesktop > 0 && uiDisplay == 0  && uiInternalDisplayId == UINT_MAX && pDX9 == nullptr && pDX9Ex == nullptr && pDX11 == nullptr)
        {
            // Desktop session based on Desktop ID
            *pSession = new RFDOPPSession(rfEncoder, hDC, hGLRC);
        }
        else if (uiDisplay > 0 && uiDesktop == 0  && uiInternalDisplayId == UINT_MAX && pDX9 == nullptr && pDX9Ex == nullptr && pDX11 == nullptr)
        {
            // Desktop session based on windows Display ID
            *pSession = new RFDOPPSession(rfEncoder, hDC, hGLRC);
        }
        else if (uiInternalDisplayId < UINT_MAX && uiDisplay == 0 && uiDesktop == 0 && pDX9 == nullptr && pDX9Ex == nullptr && pDX11 == nullptr)
        {
            // Desktop session based on internal Display ID
            *pSession = new RFDOPPSession(rfEncoder, hDC, hGLRC);
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