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

#include "DX11Window.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != nullptr)     \
   {                    \
      x->Release();     \
      x = nullptr;      \
   }
#endif

#define DX11WINDOW_CLASSNAME "DX11WindowClass"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DX11Window::DX11Window()
    : m_uiWidth(0)
    , m_uiHeight(0)
    , m_uiPosX(0)
    , m_uiPosY(0)
    , m_bMinimized(false)
    , m_hWND(NULL)
    , m_pD3DDevice(nullptr)
    , m_pD3DContext(nullptr)
    , m_pD3DSwapChain(nullptr)
{
    ZeroMemory(&m_D3DViewPort, sizeof(m_D3DViewPort));
}


DX11Window::DX11Window(DX11Window&& other)
    : m_strWindowName(std::move(other.m_strWindowName))
    , m_uiWidth(other.m_uiWidth)
    , m_uiHeight(other.m_uiHeight)
    , m_uiPosX(other.m_uiPosX)
    , m_uiPosY(other.m_uiPosY)
    , m_bMinimized(other.m_bMinimized)
    , m_hWND(other.m_hWND)
    , m_pD3DDevice(other.m_pD3DDevice)
    , m_pD3DContext(other.m_pD3DContext)
    , m_pD3DSwapChain(other.m_pD3DSwapChain)
    , m_D3DViewPort(other.m_D3DViewPort)
{
    other.m_hWND = NULL;
    other.m_pD3DDevice = nullptr;
    other.m_pD3DContext = nullptr;
    other.m_pD3DSwapChain = nullptr;
    ZeroMemory(&other.m_D3DViewPort, sizeof(other.m_D3DViewPort));
}

DX11Window::~DX11Window(void)
{
    SAFE_RELEASE(m_pD3DSwapChain);
    SAFE_RELEASE(m_pD3DContext);
    SAFE_RELEASE(m_pD3DDevice);

    if (m_hWND)
    {
        DestroyWindow(m_hWND);
    }
}

bool DX11Window::create(const std::string& strWindowName, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiPosX, unsigned int uiPosY)
{
    m_strWindowName = strWindowName;
    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;
    m_uiPosX = uiPosX;
    m_uiPosY = uiPosY;

    WNDCLASSEX wndClass = {};
    if (!GetClassInfoEx(static_cast<HINSTANCE>(GetModuleHandle(NULL)), DX11WINDOW_CLASSNAME, &wndClass))
    {
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = WndProc;
        wndClass.hInstance = static_cast<HINSTANCE>(GetModuleHandle(NULL));
        wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        wndClass.lpszClassName = DX11WINDOW_CLASSNAME;
        wndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

        if (!RegisterClassEx(&wndClass))
        {
            return false;
        }
    }

    RECT wndRect = { 0, 0, static_cast<LONG>(m_uiWidth), static_cast<LONG>(m_uiHeight) };
    AdjustWindowRect(&wndRect, WS_OVERLAPPEDWINDOW, false);

    m_hWND = CreateWindow(DX11WINDOW_CLASSNAME,
                          m_strWindowName.c_str(),
                          WS_OVERLAPPEDWINDOW,
                          m_uiPosX,
                          m_uiPosY,
                          wndRect.right - wndRect.left,
                          wndRect.bottom - wndRect.top,
                          NULL,
                          NULL,
                          static_cast<HINSTANCE>(GetModuleHandle(NULL)),
                          nullptr);

    if (!m_hWND)
    {
        return false;
    }

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
    swapchainDesc.BufferCount = 1;
    swapchainDesc.BufferDesc.Width = m_uiWidth;
    swapchainDesc.BufferDesc.Height = m_uiHeight;
    swapchainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.OutputWindow = m_hWND;
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.SampleDesc.Quality = 0;
    swapchainDesc.Windowed = TRUE;

#ifdef _DEBUG
    DWORD deviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
    DWORD deviceFlags = NULL;
#endif

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &swapchainDesc, &m_pD3DSwapChain, &m_pD3DDevice, nullptr, &m_pD3DContext) != S_OK)
    {
        return false;
    }

    ID3D11Texture2D* backBuffer;
    if (m_pD3DSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)) != S_OK)
    {
        return false;
    }

    m_RenderTarget.create(backBuffer);
    SAFE_RELEASE(backBuffer);

    m_D3DViewPort.TopLeftX = 0;
    m_D3DViewPort.TopLeftY = 0;
    m_D3DViewPort.Width = static_cast<float>(m_uiWidth);
    m_D3DViewPort.Height = static_cast<float>(m_uiHeight);
    m_D3DViewPort.MinDepth = 0.0f;
    m_D3DViewPort.MaxDepth = 1.0f;

    return true;
}

void DX11Window::open() const
{
    if (m_hWND)
    {
        ShowWindow(m_hWND, SW_SHOWDEFAULT);

        UpdateWindow(m_hWND);

        SetWindowLongPtr(m_hWND, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
}

void DX11Window::close() const
{
    if (m_hWND)
    {
        ShowWindow(m_hWND, SW_HIDE);

        UpdateWindow(m_hWND);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DX11Window* pWin = reinterpret_cast<DX11Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (!pWin)
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
        case WM_CHAR:
            if (wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}