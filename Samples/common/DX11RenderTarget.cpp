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

#include "DX11RenderTarget.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != nullptr)     \
   {                    \
      x->Release();     \
      x = nullptr;      \
   }
#endif


DX11RenderTarget::DX11RenderTarget()
    : m_pD3DRenterTargetView(nullptr)
    , m_pD3DTexture2D(nullptr)
    , m_pD3DShaderResourceView(nullptr)
    , m_pD3DDepthStencilView(nullptr)
    , m_uiWidth(0)
    , m_uiHeight(0)
{}

DX11RenderTarget::~DX11RenderTarget()
{
    release();
}

bool DX11RenderTarget::create(ID3D11Device* device, unsigned int nWidth, unsigned int nHeight, DXGI_FORMAT bufferFormat)
{
    if (m_pD3DRenterTargetView != nullptr)
    {
        return false;
    }

    m_uiWidth = nWidth;
    m_uiHeight = nHeight;

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = m_uiWidth;
    textureDesc.Height = m_uiHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = bufferFormat;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (device->CreateTexture2D(&textureDesc, nullptr, &m_pD3DTexture2D) != S_OK)
    {
        return false;
    }

    if (device->CreateRenderTargetView(m_pD3DTexture2D, nullptr, &m_pD3DRenterTargetView))
    {
        return false;
    }

    if (device->CreateShaderResourceView(m_pD3DTexture2D, nullptr, &m_pD3DShaderResourceView))
    {
        return false;
    }

    textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthStencil;
    if (device->CreateTexture2D(&textureDesc, nullptr, &depthStencil) != S_OK)
    {
        return false;
    }

    if (device->CreateDepthStencilView(depthStencil, nullptr, &m_pD3DDepthStencilView) != S_OK)
    {
        SAFE_RELEASE(depthStencil);
        return false;
    }
    SAFE_RELEASE(depthStencil);

    return true;
}

bool DX11RenderTarget::create(ID3D11Texture2D* texture)
{
    if (m_pD3DRenterTargetView != nullptr)
    {
        return false;
    }

    m_pD3DTexture2D = texture;
    m_pD3DTexture2D->AddRef();

    D3D11_TEXTURE2D_DESC textureDesc;
    m_pD3DTexture2D->GetDesc(&textureDesc);

    m_uiWidth = textureDesc.Width;
    m_uiHeight = textureDesc.Height;

    ID3D11Device* device;
    m_pD3DTexture2D->GetDevice(&device);

    if (device->CreateRenderTargetView(m_pD3DTexture2D, nullptr, &m_pD3DRenterTargetView))
    {
        SAFE_RELEASE(device);
        return false;
    }

    textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthStencil;
    if (device->CreateTexture2D(&textureDesc, nullptr, &depthStencil) != S_OK)
    {
        SAFE_RELEASE(device);
        return false;
    }

    if (device->CreateDepthStencilView(depthStencil, nullptr, &m_pD3DDepthStencilView) != S_OK)
    {
        SAFE_RELEASE(depthStencil);
        SAFE_RELEASE(device);
        return false;
    }
    SAFE_RELEASE(depthStencil);
    SAFE_RELEASE(device);

    return true;
}

void DX11RenderTarget::release()
{
    SAFE_RELEASE(m_pD3DDepthStencilView);
    SAFE_RELEASE(m_pD3DShaderResourceView);
    SAFE_RELEASE(m_pD3DRenterTargetView);
    SAFE_RELEASE(m_pD3DTexture2D);
}

void DX11RenderTarget::set(ID3D11DeviceContext* context)
{
    context->OMSetRenderTargets(1, &m_pD3DRenterTargetView, m_pD3DDepthStencilView);
}

void DX11RenderTarget::clear(ID3D11DeviceContext* context, const float* color, float depth)
{
    context->ClearRenderTargetView(m_pD3DRenterTargetView, color);
    context->ClearDepthStencilView(m_pD3DDepthStencilView, D3D11_CLEAR_DEPTH, depth, 0);
}

void DX11RenderTarget::clear(ID3D11DeviceContext* context, const float* color)
{
    context->ClearRenderTargetView(m_pD3DRenterTargetView, color);
}

void DX11RenderTarget::clear(ID3D11DeviceContext* context, float depth)
{
    context->ClearDepthStencilView(m_pD3DDepthStencilView, D3D11_CLEAR_DEPTH, depth, 0);
}