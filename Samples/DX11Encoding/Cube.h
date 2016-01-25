#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

class Cube
{
public:

    Cube();

    bool OnCreateDevice(ID3D11Device* pd3dDevice);
    void OnDestroyDevice();
    void Update(ID3D11DeviceContext* pd3dImmediateContext, const DirectX::XMMATRIX& viewProjMatrix);
    void Draw(ID3D11DeviceContext* pd3dImmediateContext) const;

private:

    ID3D11VertexShader*	m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11Buffer* m_constantBuffer;

    float m_angle;
};