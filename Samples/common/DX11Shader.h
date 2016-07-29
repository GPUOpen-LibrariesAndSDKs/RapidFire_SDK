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

class DX11Shader
{
public:

    DX11Shader();
    ~DX11Shader();

    bool create(ID3D11Device* pd3dDevice, const char* vertexShader, const char* pixelShader);
    void release();
    void set(ID3D11DeviceContext* pd3dImmediateContext) const;

    operator bool() const
    {
        return m_pixelShader != nullptr;
    }

protected:

    ID3D11VertexShader*	m_vertexShader;
    ID3D11PixelShader* m_pixelShader;

    DX11Shader(DX11Shader const& w);

    DX11Shader operator=(DX11Shader const rhs);
};

class DX11ConstantBuffer
{
public:

    DX11ConstantBuffer();
    ~DX11ConstantBuffer();

    bool create(ID3D11Device* pd3dDevice, UINT size);
    void release();
    void* map(ID3D11DeviceContext* pd3dImmediateContext, bool discardBufferContent) const;
    void unmap(ID3D11DeviceContext* pd3dImmediateContext) const;
    void set(ID3D11DeviceContext* pd3dImmediateContext, UINT slot) const;

    operator bool() const
    {
        return m_buffer != nullptr;
    }

protected:

    ID3D11Buffer* m_buffer;

    DX11ConstantBuffer(DX11ConstantBuffer const& w);

    DX11ConstantBuffer operator=(DX11ConstantBuffer const rhs);
};