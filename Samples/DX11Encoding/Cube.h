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
#include <DirectXMath.h>

#include "..\common\DX11Shader.h"

class Cube
{
public:

    Cube();

    bool create(ID3D11Device* pd3dDevice, uint32_t screenWidth, uint32_t screenHeight);
    void release();
    void update(ID3D11DeviceContext* pd3dImmediateContext, float time);
    void draw(ID3D11DeviceContext* pd3dImmediateContext) const;

private:

    DX11Shader          m_shader;
    DX11ConstantBuffer  m_constantBuffer;

    DirectX::XMFLOAT4X4	m_viewMatrix;
    DirectX::XMFLOAT4X4 m_projMatrix;
    float               m_angle;
};