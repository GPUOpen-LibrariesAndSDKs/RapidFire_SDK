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

Cube::Cube()
    : m_vertexShader(nullptr)
    , m_pixelShader(nullptr)
    , m_constantBuffer(nullptr)
    , m_angle(0.0f)
{}

bool Cube::OnCreateDevice(ID3D11Device* pd3dDevice)
{
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    if (D3DCompile2(vertexShader.c_str(), vertexShader.length(), NULL, nullptr, nullptr, "VS", "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, 0, NULL, 0, &shaderBlob, &errorBlob) != S_OK)
    {
        std::string err(static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        MessageBox(NULL, err.c_str(), "Failed to compile cube vertex shader!", MB_ICONERROR | MB_OK);
        return false;
    }

    if (pd3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &m_vertexShader) != S_OK)
    {
        return false;
    }

    SAFE_RELEASE(shaderBlob);
    SAFE_RELEASE(errorBlob);

    if (D3DCompile2(pixelShader.c_str(), pixelShader.length(), NULL, nullptr, nullptr, "PS", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, 0, NULL, 0, &shaderBlob, &errorBlob) != S_OK)
    {
        std::string err(static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        MessageBox(NULL, err.c_str(), "Compile Error", MB_ICONERROR | MB_OK);
        return false;
    }

    if (pd3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &m_pixelShader) != S_OK)
    {
        return false;
    }

    SAFE_RELEASE(shaderBlob);
    SAFE_RELEASE(errorBlob);

    D3D11_BUFFER_DESC shaderBufferDesc = {};
    shaderBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    shaderBufferDesc.ByteWidth = 4 * 4 * sizeof(float);
    shaderBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    shaderBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (pd3dDevice->CreateBuffer(&shaderBufferDesc, nullptr, &m_constantBuffer))
    {
        return false;
    }

    return true;
}

void Cube::OnDestroyDevice()
{
    SAFE_RELEASE(m_constantBuffer);
    SAFE_RELEASE(m_vertexShader);
    SAFE_RELEASE(m_pixelShader);
}

void Cube::Update(ID3D11DeviceContext* pd3dImmediateContext, const XMMATRIX& viewProjMatrix)
{
    m_angle -= 0.02f;
    XMMATRIX world = XMMatrixRotationAxis(XMVectorSet(1, 1, 0, 0), m_angle);
    XMMATRIX worldViewProj = world * viewProjMatrix;
    worldViewProj = XMMatrixTranspose(worldViewProj);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (pd3dImmediateContext->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) != S_OK)
    {
        return;
    }

    *static_cast<XMMATRIX*>(mappedResource.pData) = worldViewProj;

    pd3dImmediateContext->Unmap(m_constantBuffer, 0);

    pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);
}

void Cube::Draw(ID3D11DeviceContext* pd3dImmediateContext) const
{
    pd3dImmediateContext->IASetInputLayout(nullptr);
    pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = 0;
    UINT offset = 0;
    pd3dImmediateContext->IASetVertexBuffers(0, 0, nullptr, &stride, &offset);

    pd3dImmediateContext->VSSetShader(m_vertexShader, nullptr, 0);
    pd3dImmediateContext->PSSetShader(m_pixelShader, nullptr, 0);

    pd3dImmediateContext->Draw(36, 0);
}