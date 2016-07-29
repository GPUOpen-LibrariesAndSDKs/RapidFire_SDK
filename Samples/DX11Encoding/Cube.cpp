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

#include "Cube.h"

#include <string>

#include <D3Dcompiler.h>

using namespace DirectX;

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if (x != nullptr)    \
   {                    \
      x->Release();     \
      x = nullptr;      \
   }
#endif

namespace {

std::string vertexShader = "																													  \
    cbuffer global																																\n\
    {																																			\n\
        float4x4 gWorldViewProj;																												\n\
    };																																			\n\
                                                                                                                                                \n\
    static const float3 cubePos[8] = {	float3(-.5f,-.5f,-.5f), float3(.5f,-.5f,-.5f), float3(-.5f,-.5f,.5f), float3(.5f,-.5f,.5f),			    \n\
                                        float3(-.5f,.5f,-.5f), float3(.5f,.5f,-.5f), float3(-.5f,.5f,.5f), float3(.5f,.5f,.5f) };			    \n\
    static const int cubeVertices[36] = { 0,4,5,0,5,1,																							\n\
                                          1,5,7,1,7,3,																							\n\
                                          3,7,6,3,6,2,																							\n\
                                          2,6,4,2,4,0,																							\n\
                                          4,6,7,4,7,5,																							\n\
                                          2,0,1,2,1,3 };																						\n\
                                                                                                                                                \n\
    struct VertexOut																															\n\
    {																																			\n\
        float4 PosH : SV_POSITION;																												\n\
        float4 Color : COLOR;																													\n\
    };																																			\n\
                                                                                                                                                \n\
    VertexOut VS(uint vid : SV_VertexID)																										\n\
    {																																			\n\
        VertexOut vout;																															\n\
        vout.PosH = mul(float4(cubePos[cubeVertices[vid]], 1.0f), gWorldViewProj);															    \n\
        vout.Color = float4(cubePos[cubeVertices[vid]] + float3(.5f, .5f, .5f), 1);															    \n\
        return vout;																															\n\
    }																																			\n";



std::string pixelShader = "								  \
    struct VertexOut									\n\
    {													\n\
        float4 PosH : SV_POSITION;						\n\
        float4 Color : COLOR;							\n\
    };													\n\
                                                        \n\
    float4 PS(VertexOut pin) : SV_TARGET				\n\
    {													\n\
        return pin.Color;								\n\
    }													\n";

}

Cube::Cube()
    : m_angle(0.0f)
{}

bool Cube::create(ID3D11Device* pd3dDevice, uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_shader.create(pd3dDevice, vertexShader.c_str(), pixelShader.c_str()))
    {
        return false;
    }

    if (!m_constantBuffer.create(pd3dDevice, sizeof(XMMATRIX)))
    {
        return false;
    }

    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.0f);
    XMStoreFloat4x4(&m_projMatrix, proj);

    return true;
}

void Cube::release()
{
    m_constantBuffer.release();
    m_shader.release();
}

void Cube::update(ID3D11DeviceContext* pd3dImmediateContext, float time)
{
    float x = static_cast<float>(1.5 * cos(time));
    float y = static_cast<float>(0.75 * sin(time));

    XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -3, 1), XMVectorSet(x, y, 0, 1), XMVectorSet(0, 1, 0, 0));
    XMStoreFloat4x4(&m_viewMatrix, view);

    m_angle -= 0.02f;
    XMMATRIX world = XMMatrixRotationAxis(XMVectorSet(1, 1, 0, 0), m_angle);
    XMMATRIX worldViewProj = world * view * XMLoadFloat4x4(&m_projMatrix);
    worldViewProj = XMMatrixTranspose(worldViewProj);

    void* constantBufferPtr = m_constantBuffer.map(pd3dImmediateContext, true);
    if (constantBufferPtr == nullptr)
    {
        return;
    }

    *static_cast<XMMATRIX*>(constantBufferPtr) = worldViewProj;

    m_constantBuffer.unmap(pd3dImmediateContext);
}

void Cube::draw(ID3D11DeviceContext* pd3dImmediateContext) const
{
    pd3dImmediateContext->IASetInputLayout(nullptr);
    pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = 0;
    UINT offset = 0;
    pd3dImmediateContext->IASetVertexBuffers(0, 0, nullptr, &stride, &offset);

    m_constantBuffer.set(pd3dImmediateContext, 0);

    m_shader.set(pd3dImmediateContext);

    pd3dImmediateContext->Draw(36, 0);
}