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

#include "..\common\DX11Shader.h"

class StretchRectShader
{
public:

    StretchRectShader();
    ~StretchRectShader();

    bool create(ID3D11Device* device);
    void release();

    // Copies the src shader resource view into the dest render target.
    // nullptr for rect parameters causes the entire source or dest to be used
    void stretchRect(ID3D11DeviceContext* context, ID3D11ShaderResourceView* src, const RECT* sourceRect, ID3D11RenderTargetView* dest, const RECT* destRect) const;

protected:

    DX11Shader          m_Shader;
    DX11ConstantBuffer  m_ConstantBuffer;
    ID3D11SamplerState* m_pD3DSamplerState;
};