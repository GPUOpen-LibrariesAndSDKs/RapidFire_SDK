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

#include "StretchRectShader.h"

#include <string>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != nullptr)     \
   {                    \
      x->Release();     \
      x = nullptr;      \
   }
#endif

namespace
{

std::string stretchRectVertexShader = "                                             \
    cbuffer global                                                                \n\
    {                                                                             \n\
        float2 gSourcePosMin;                                                     \n\
        float2 gSourcePosMax;                                                     \n\
    };                                                                            \n\
                                                                                  \n\
    struct VertexOut                                                              \n\
    {                                                                             \n\
        float4 PosH : SV_POSITION;                                                \n\
        float2 Tex : TEXCOORD;                                                    \n\
    };                                                                            \n\
                                                                                  \n\
    VertexOut VS(uint vid : SV_VertexID)                                          \n\
    {                                                                             \n\
        VertexOut vout;                                                           \n\
        switch(vid)                                                               \n\
        {                                                                         \n\
            case 0:                                                               \n\
                vout.PosH = float4(-1, -1, 0, 1);                                 \n\
                vout.Tex = gSourcePosMin;                                         \n\
                return vout;                                                      \n\
            case 1:                                                               \n\
                vout.PosH = float4(-1, 1, 0, 1);                                  \n\
                vout.Tex = float2(gSourcePosMin.x, gSourcePosMax.y);              \n\
                return vout;                                                      \n\
            case 2:                                                               \n\
                vout.PosH = float4(1, -1, 0, 1);                                  \n\
                vout.Tex = float2(gSourcePosMax.x, gSourcePosMin.y);              \n\
                return vout;                                                      \n\
            case 3:                                                               \n\
                vout.PosH = float4(1, 1, 0, 1);                                   \n\
                vout.Tex = gSourcePosMax;                                         \n\
                return vout;                                                      \n\
            default:                                                              \n\
                vout.PosH = float4(0, 0, 0, 0);                                   \n\
                vout.Tex = float2(0, 0);                                          \n\
                return vout;                                                      \n\
        }                                                                         \n\
    }                                                                             \n\
                                                                                  \n";

std::string stretchRectPixelShader = "                                              \
    Texture2D sourceTexture;                                                      \n\
    SamplerState Sampler;                                                         \n\
                                                                                  \n\
    struct VertexOut                                                              \n\
    {                                                                             \n\
        float4 PosH : SV_POSITION;                                                \n\
        float2 Tex : TEXCOORD;                                                    \n\
    };                                                                            \n\
                                                                                  \n\
    float4 PS(VertexOut pin) : SV_TARGET				                          \n\
    {                                                                             \n\
        return sourceTexture.Sample(Sampler, pin.Tex);                            \n\
    }                                                                             \n\
                                                                                  \n";

}

StretchRectShader::StretchRectShader()
    : m_pD3DSamplerState(nullptr)
{}

StretchRectShader::~StretchRectShader()
{
    release();
}

bool StretchRectShader::create(ID3D11Device* device)
{
    if (m_Shader)
    {
        return false;
    }

    if (!m_Shader.create(device, stretchRectVertexShader.c_str(), stretchRectPixelShader.c_str()))
    {
        return false;
    }

    if (!m_ConstantBuffer.create(device, 4 * sizeof(float)))
    {
        return false;
    }

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    if (device->CreateSamplerState(&samplerDesc, &m_pD3DSamplerState) != S_OK)
    {
        return false;
    }

    return true;
}

void StretchRectShader::release()
{
    SAFE_RELEASE(m_pD3DSamplerState);
    m_Shader.release();
    m_ConstantBuffer.release();
}

void StretchRectShader::stretchRect(ID3D11DeviceContext* context, ID3D11ShaderResourceView* src, const RECT* sourceRect, ID3D11RenderTargetView* dest, const RECT* destRect) const
{
    if (!context || !src || !dest)
    {
        return;
    }

    D3D11_VIEWPORT vp;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    if (!destRect)
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        dest->GetDesc(&rtvDesc);
        if (rtvDesc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D)
        {
            return;
        }

        ID3D11Resource* tex;
        dest->GetResource(&tex);
        D3D11_TEXTURE2D_DESC texDesc;
        static_cast<ID3D11Texture2D*>(tex)->GetDesc(&texDesc);
        SAFE_RELEASE(tex);

        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        vp.Width = static_cast<float>(texDesc.Width);
        vp.Height = static_cast<float>(texDesc.Height);
    }
    else
    {
        vp.TopLeftX = static_cast<float>(destRect->left);
        vp.TopLeftY = static_cast<float>(destRect->top);
        vp.Width = static_cast<float>(destRect->right - destRect->left);
        vp.Height = static_cast<float>(destRect->bottom - destRect->top);
    }

    context->RSSetViewports(1, &vp);
    context->OMSetRenderTargets(1, &dest, nullptr);

    float* sourceTexcoords = reinterpret_cast<float*>(m_ConstantBuffer.map(context, true));
    if (!sourceRect)
    {
        sourceTexcoords[0] = 0.0f;
        sourceTexcoords[1] = 1.0f;
        sourceTexcoords[2] = 1.0f;
        sourceTexcoords[3] = 0.0f;
    }
    else
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        src->GetDesc(&srvDesc);
        if (srvDesc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D)
        {
            m_ConstantBuffer.unmap(context);
            return;
        }

        ID3D11Resource* tex;
        src->GetResource(&tex);
        D3D11_TEXTURE2D_DESC texDesc;
        static_cast<ID3D11Texture2D*>(tex)->GetDesc(&texDesc);
        SAFE_RELEASE(tex);

        sourceTexcoords[0] = static_cast<float>(sourceRect->left) / texDesc.Width;
        sourceTexcoords[1] = static_cast<float>(sourceRect->bottom) / texDesc.Height;
        sourceTexcoords[2] = static_cast<float>(sourceRect->right) / texDesc.Width;
        sourceTexcoords[3] = static_cast<float>(sourceRect->top) / texDesc.Height;
    }

    m_ConstantBuffer.unmap(context);
    m_ConstantBuffer.set(context, 0);

    context->PSSetShaderResources(0, 1, &src);
    context->PSSetSamplers(0, 1, &m_pD3DSamplerState);

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    UINT stride = 0;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 0, nullptr, &stride, &offset);
    m_Shader.set(context);

    context->Draw(4, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);
}