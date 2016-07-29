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

#include "DX11Shader.h"

#include <string>

#include <D3Dcompiler.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if (x != nullptr)    \
   {                    \
      x->Release();     \
      x = nullptr;      \
   }
#endif

DX11Shader::DX11Shader()
    : m_vertexShader(nullptr)
    , m_pixelShader(nullptr)
{}

DX11Shader::~DX11Shader()
{
    release();
}

bool DX11Shader::create(ID3D11Device* pd3dDevice, const char* vertexShader, const char* pixelShader)
{
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    if (D3DCompile2(vertexShader, strlen(vertexShader), NULL, nullptr, nullptr, "VS", "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, 0, NULL, 0, &shaderBlob, &errorBlob) != S_OK)
    {
        std::string err(static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        MessageBox(NULL, err.c_str(), "Failed to compile vertex shader!", MB_ICONERROR | MB_OK);
        return false;
    }

    if (pd3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &m_vertexShader) != S_OK)
    {
        return false;
    }

    SAFE_RELEASE(shaderBlob);
    SAFE_RELEASE(errorBlob);

    if (D3DCompile2(pixelShader, strlen(pixelShader), NULL, nullptr, nullptr, "PS", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, 0, NULL, 0, &shaderBlob, &errorBlob) != S_OK)
    {
        std::string err(static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
        MessageBox(NULL, err.c_str(), "Failed to compile pixel shader", MB_ICONERROR | MB_OK);
        return false;
    }

    if (pd3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &m_pixelShader) != S_OK)
    {
        return false;
    }

    SAFE_RELEASE(shaderBlob);
    SAFE_RELEASE(errorBlob);

    return true;
}

void DX11Shader::release()
{
    SAFE_RELEASE(m_pixelShader);
    SAFE_RELEASE(m_vertexShader);
}

void DX11Shader::set(ID3D11DeviceContext* pd3dImmediateContext) const
{
    pd3dImmediateContext->VSSetShader(m_vertexShader, nullptr, 0);
    pd3dImmediateContext->PSSetShader(m_pixelShader, nullptr, 0);
}

DX11ConstantBuffer::DX11ConstantBuffer()
    : m_buffer(nullptr)
{}

DX11ConstantBuffer::~DX11ConstantBuffer()
{
    release();
}

bool DX11ConstantBuffer::create(ID3D11Device* pd3dDevice, UINT size)
{
    D3D11_BUFFER_DESC shaderBufferDesc = {};
    shaderBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    shaderBufferDesc.ByteWidth = size;
    shaderBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    shaderBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (pd3dDevice->CreateBuffer(&shaderBufferDesc, nullptr, &m_buffer))
    {
        return false;
    }

    return true;
}

void DX11ConstantBuffer::release()
{
    SAFE_RELEASE(m_buffer);
}

void* DX11ConstantBuffer::map(ID3D11DeviceContext* pd3dImmediateContext, bool discard) const
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (pd3dImmediateContext->Map(m_buffer, 0, discard ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE, 0, &mappedResource) != S_OK)
    {
        return nullptr;
    }

    return mappedResource.pData;
}

void DX11ConstantBuffer::unmap(ID3D11DeviceContext* pd3dImmediateContext) const
{
    pd3dImmediateContext->Unmap(m_buffer, 0);
}

void DX11ConstantBuffer::set(ID3D11DeviceContext* pd3dImmediateContext, UINT slot) const
{
    pd3dImmediateContext->VSSetConstantBuffers(slot, 1, &m_buffer);
}