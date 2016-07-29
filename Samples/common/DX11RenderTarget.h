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

#include <d3d11.h>

class DX11RenderTarget
{
public:

    DX11RenderTarget();
    ~DX11RenderTarget();

    // Create RenderTarget with specified dimension
    bool    create(ID3D11Device* device, unsigned int nWidth, unsigned int nHeight, DXGI_FORMAT bufferFormat);
    // Create RenderTarget for a texture
    bool    create(ID3D11Texture2D* texture);
    // Set RenderTarget for DX11 context
    void    set(ID3D11DeviceContext* context);
    // Clear RenderTarget with color and DepthBuffer with depth
    void    clear(ID3D11DeviceContext* context, const float* color, float depth);
    // Clear RenderTarget with color
    void    clear(ID3D11DeviceContext* context, const float* color);
    // Clear DepthBuffer with depth
    void    clear(ID3D11DeviceContext* context, float depth);
    // Delete RenderTarget and storage
    void    release();

    unsigned int    getWidth() const { return m_uiWidth; }
    unsigned int    getHeight() const { return m_uiHeight; }
    ID3D11RenderTargetView*   getRenderTarget() const { return m_pD3DRenterTargetView; }
    ID3D11ShaderResourceView* getShaderResource() const { return m_pD3DShaderResourceView; }
    ID3D11Texture2D*          getRenderTargetTexture() const { return m_pD3DTexture2D; }
    ID3D11DepthStencilView*   getDepthStencil() const { return m_pD3DDepthStencilView; }

    operator bool() const
    {
        return m_pD3DRenterTargetView != nullptr;
    }

protected:

    ID3D11RenderTargetView*   m_pD3DRenterTargetView;
    ID3D11ShaderResourceView* m_pD3DShaderResourceView;
    ID3D11Texture2D*          m_pD3DTexture2D;
    ID3D11DepthStencilView*   m_pD3DDepthStencilView;
    unsigned int    m_uiWidth;
    unsigned int    m_uiHeight;

    DX11RenderTarget(DX11RenderTarget const& w);

    DX11RenderTarget operator=(DX11RenderTarget const rhs);
};