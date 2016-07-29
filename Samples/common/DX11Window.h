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

#include <string>

#include <d3d11.h>

#include "DX11RenderTarget.h"

class DX11Window
{
public:

    DX11Window();
    DX11Window(DX11Window&& other);
    ~DX11Window();

    bool    create(const std::string& strWindowName, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiPosX, unsigned int uiPosY);
    void    open() const;
    void    close() const;

    void    minimized() { m_bMinimized = true; }
    void    restored() { m_bMinimized = false; }

    HWND                    getWindow() const { return m_hWND; }
    ID3D11Device*           getDevice() const { return m_pD3DDevice; }
    ID3D11DeviceContext*    getContext() const { return m_pD3DContext; }
    IDXGISwapChain*         getSwapChain() const { return m_pD3DSwapChain; }
    ID3D11RenderTargetView* getRenderTarget() const { return m_RenderTarget.getRenderTarget(); }
    ID3D11DepthStencilView* getDepthStencil() const { return m_RenderTarget.getDepthStencil(); }
    const D3D11_VIEWPORT&   getViewPort() const { return m_D3DViewPort; }

    unsigned int getWidth()  const { return m_uiWidth; }
    unsigned int getHeight() const { return m_uiHeight; }
    bool isMinimized()       const { return m_bMinimized; }

    operator bool() const
    {
        return m_hWND != NULL;
    }

protected:

    HWND                    m_hWND;
    ID3D11Device*			m_pD3DDevice;
    ID3D11DeviceContext*	m_pD3DContext;
    IDXGISwapChain*			m_pD3DSwapChain;
    DX11RenderTarget        m_RenderTarget;
    D3D11_VIEWPORT          m_D3DViewPort;

    std::string             m_strWindowName;

    bool                    m_bMinimized;
    unsigned int            m_uiWidth;
    unsigned int            m_uiHeight;
    unsigned int            m_uiPosX;
    unsigned int            m_uiPosY;

    DX11Window(DX11Window const& w);

    DX11Window operator=(DX11Window const rhs);
};
