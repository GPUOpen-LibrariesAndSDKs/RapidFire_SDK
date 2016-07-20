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

/////////////////////////////////////////////////////////////////////////////////////////
//
// DX11Encoding shows how to use RapidFire to create an H264 encoded stream from a 
// DX11 texture.
// First a RF session is created passing the DirectX 11 device. The session is configured 
// to use the AMF encoder (HW encoding). The rendertargets that are used by the application are
// registered, now the application can render to those rendertargets and encodeFrame will use
// them as input for the encoder and return a H264 frame that is dumped to a file.
// 
/////////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <string>
#include <thread>

#include <d3d11.h>
#include <DirectXMath.h>
#include <windows.h>

#include "Cube.h"
#include "RapidFireServer.h"

using namespace DirectX;

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != nullptr)     \
   {                    \
      x->Release();     \
      x = nullptr;      \
   }
#endif

HWND					g_hWnd = NULL;
ID3D11Device*			g_pD3DDevice = nullptr;
ID3D11DeviceContext*	g_pImmediateContext = nullptr;
IDXGISwapChain*			g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRtv = nullptr;
ID3D11DepthStencilView* g_pDsv = nullptr;
D3D11_VIEWPORT          g_viewPort = {};
ID3D11Texture2D*		g_pRTTextures[2] = { nullptr, nullptr };
ID3D11RenderTargetView*	g_pRTEncoding[2] = { nullptr, nullptr };
XMFLOAT4X4				g_viewMatrix;
XMFLOAT4X4				g_projMatrix;
Cube					g_cube;
double					g_secondsPerCount;
LARGE_INTEGER			g_startTime;

bool                    g_running = true;
unsigned int            g_stream_width = 1280;
unsigned int            g_stream_height = 720;

bool OpenWindow(LPCSTR cClassName, LPCSTR cWindowName);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool CreateDeviceAndSwapChain();
void DestroyDevice();
bool Resize();
void Update();
void Draw(unsigned int rtIdx);


void ReaderThread(const RFEncodeSession& session, const std::string& file_name)
{
    void*         p_bit_stream = nullptr;
    unsigned int  bitstream_size = 0;
    RFStatus      rf_status = RF_STATUS_OK;
    std::ofstream out_file;

    if (file_name.length() > 0)
    {
        out_file.open(file_name, std::fstream::out | std::fstream::trunc | std::fstream::binary);
    }

    while (g_running)
    {
        rf_status = rfGetEncodedFrame(session, &bitstream_size, &p_bit_stream);

        if (rf_status == RF_STATUS_OK)
        {
            if (out_file.is_open())
            {
                out_file.write(static_cast<char*>(p_bit_stream), bitstream_size);
            }
        }
        else
        {
            // Give the other thread a chance
            Sleep(0);
        }
    }
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    RFStatus             rfStatus = RF_STATUS_OK;
    RFEncodeSession      rfSession = nullptr;
    LPCSTR cClassName  = "DX11WindowClass";
    LPCSTR cWindowName = "DX11 Encoding";

    if (!OpenWindow(cClassName, cWindowName))
    {
        MessageBox(NULL, "Failed to create window!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    if (!CreateDeviceAndSwapChain())
    {
        MessageBox(NULL, "Failed to create D3D11Device!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    RFProperties props[] = { RF_ENCODER,      static_cast<RFProperties>(RF_AMF),       // Use HW H.264 encoder
                             RF_D3D11_DEVICE, reinterpret_cast<RFProperties>(g_pD3DDevice), // pass device to RF
                             0 };

    // Create encoding session
    rfStatus = rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    // Set properties to quality preset with 60fps for rfCreateEncoder2
    RFProperties encoder_props[] = { RF_ENCODER_FORMAT,               static_cast<RFProperties>(RF_NV12),
                                     RF_ENCODER_BITRATE,              static_cast<RFProperties>(20000000),
                                     RF_ENCODER_PEAK_BITRATE,         static_cast<RFProperties>(20000000),
                                     RF_ENCODER_RATE_CONTROL_METHOD,  static_cast<RFProperties>(0),
                                     RF_ENCODER_MIN_QP,               static_cast<RFProperties>(18),
                                     RF_ENCODER_FRAME_RATE,           static_cast<RFProperties>(60),
                                     RF_ENCODER_VBV_BUFFER_SIZE,      static_cast<RFProperties>(20000000),
                                     RF_ENCODER_ENFORCE_HRD,          static_cast<RFProperties>(false),
                                     RF_ENCODER_IDR_PERIOD,           static_cast<RFProperties>(30),
                                     RF_ENCODER_INTRA_REFRESH_NUM_MB, static_cast<RFProperties>(0),
                                     RF_ENCODER_DEBLOCKING_FILTER,    static_cast<RFProperties>(true),
                                     RF_ENCODER_QUALITY_PRESET,       static_cast<RFProperties>(2),
                                     0 };

    rfStatus = rfCreateEncoder2(rfSession, g_stream_width, g_stream_height, encoder_props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    unsigned int rf_fbo_idx[2];

    // Register rendertargets to the RapidFire session
    bool success = (RF_STATUS_OK == rfRegisterRenderTarget(rfSession, static_cast<RFRenderTarget>(g_pRTTextures[0]), g_stream_width, g_stream_height, &rf_fbo_idx[0]));
    success &= (RF_STATUS_OK == rfRegisterRenderTarget(rfSession, static_cast<RFRenderTarget>(g_pRTTextures[1]), g_stream_width, g_stream_height, &rf_fbo_idx[1]));

    if (!success)
    {
        MessageBox(NULL, "Failed to register rendertargets", "RF Error", MB_ICONERROR | MB_OK);
        rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    // create thread that dumps encoded frames to file.
    std::thread reader(ReaderThread, rfSession, "DX11Stream.h264");

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    g_secondsPerCount = 1.0 / freq.QuadPart;
    QueryPerformanceCounter(&g_startTime);

    unsigned int uiIndex = 0;

    MSG msg = {};

    for (;;)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
        {
            g_running = false;
            break;
        }

        do
        {
            // rfEncode might fail if the queue is full.
            // In this case we need to wait until the reader thread has called rfGetEncodedFrame and removed a frame from the encoding queue.
            rfStatus = rfEncodeFrame(rfSession, rf_fbo_idx[uiIndex]);

            if (rfStatus == RF_STATUS_QUEUE_FULL)
            {
                // Give other thread a chance
                Sleep(0);
            }
        } while (rfStatus == RF_STATUS_QUEUE_FULL);

        Update();
        Draw(uiIndex);

        // Switch rendertarget to use
        uiIndex = 1 - uiIndex;
    }

    reader.join();

    rfDeleteEncodeSession(&rfSession);

    DestroyDevice();

    UnregisterClass(cClassName, hInst);

    return static_cast<int>(msg.wParam);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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


bool OpenWindow(LPCSTR cClassName_, LPCSTR cWindowName_)
{
    WNDCLASSEX		    wndclass    = {};
    const LPCSTR        cClassName  = cClassName_;
    const LPCSTR	    cWindowName = cWindowName_;

    // Register WindowClass
    wndclass.cbSize        = sizeof(WNDCLASSEX);
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.hInstance     = static_cast<HINSTANCE>(GetModuleHandle(NULL));
    wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    wndclass.lpszClassName = cClassName;
    wndclass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wndclass))
    {
        return false;
    }

    RECT wndRect = { 0, 0, static_cast<LONG>(g_stream_width), static_cast<LONG>(g_stream_height) };
    AdjustWindowRect(&wndRect, WS_OVERLAPPEDWINDOW, false);

    g_hWnd = CreateWindow(cClassName,
                          cWindowName,
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          wndRect.right - wndRect.left,
                          wndRect.bottom - wndRect.top,
                          NULL,
                          NULL,
                          static_cast<HINSTANCE>(GetModuleHandle(NULL)),
                          nullptr);

    if (!g_hWnd)
    {
        return false;
    }

    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    return true;
}


bool CreateDeviceAndSwapChain()
{
    HRESULT hr = S_OK;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
    swapchainDesc.BufferCount = 1;
    swapchainDesc.BufferDesc.Width = g_stream_width;
    swapchainDesc.BufferDesc.Height = g_stream_height;
    swapchainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.OutputWindow = g_hWnd;
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.SampleDesc.Quality = 0;
    swapchainDesc.Windowed = TRUE;

#ifdef _DEBUG
    DWORD deviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
    DWORD deviceFlags = NULL;
#endif

    hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &swapchainDesc, &g_pSwapChain, &g_pD3DDevice, nullptr, &g_pImmediateContext);
    if (hr != S_OK)
    {
        return false;
    }

    if (!Resize())
    {
        return false;
    }

    if (!g_cube.OnCreateDevice(g_pD3DDevice))
    {
        return false;
    }

    return true;
}


bool Resize()
{
    if (!g_pD3DDevice || !g_pImmediateContext || !g_pSwapChain)
    {
        return false;
    }

    if (g_pSwapChain->ResizeBuffers(1, g_stream_width, g_stream_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0))
    {
        return false;
    }

    ID3D11Texture2D* backBuffer;
    if (g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)) != S_OK)
    {
        return false;
    }

    if (g_pD3DDevice->CreateRenderTargetView(backBuffer, nullptr, &g_pRtv) != S_OK)
    {
        SAFE_RELEASE(backBuffer);
        return false;
    }
    SAFE_RELEASE(backBuffer);

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width              = g_stream_width;
    textureDesc.Height             = g_stream_height;
    textureDesc.MipLevels          = 1;
    textureDesc.ArraySize          = 1;
    textureDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage              = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthStencil;
    if (g_pD3DDevice->CreateTexture2D(&textureDesc, nullptr, &depthStencil) != S_OK)
    {
        return false;
    }

    if (g_pD3DDevice->CreateDepthStencilView(depthStencil, nullptr, &g_pDsv) != S_OK)
    {
        SAFE_RELEASE(depthStencil);
        return false;
    }

    SAFE_RELEASE(depthStencil);

    g_viewPort.TopLeftX = 0;
    g_viewPort.TopLeftY = 0;
    g_viewPort.Width    = static_cast<float>(g_stream_width);
    g_viewPort.Height   = static_cast<float>(g_stream_height);
    g_viewPort.MinDepth = 0.0f;
    g_viewPort.MaxDepth = 1.0f;

    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
    if (g_pD3DDevice->CreateTexture2D(&textureDesc, nullptr, &g_pRTTextures[0]) != S_OK)
    {
        return false;
    }

    if (g_pD3DDevice->CreateRenderTargetView(g_pRTTextures[0], nullptr, &g_pRTEncoding[0]))
    {
        return false;
    }

    if (g_pD3DDevice->CreateTexture2D(&textureDesc, nullptr, &g_pRTTextures[1]) != S_OK)
    {
        return false;
    }

    if (g_pD3DDevice->CreateRenderTargetView(g_pRTTextures[1], nullptr, &g_pRTEncoding[1]))
    {
        return false;
    }

    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(g_stream_width) / g_stream_height, 0.1f, 100.0f);
    XMStoreFloat4x4(&g_projMatrix, proj);

    return true;
}


void DestroyDevice()
{
    g_cube.OnDestroyDevice();
    SAFE_RELEASE(g_pRTEncoding[0]);
    SAFE_RELEASE(g_pRTEncoding[1]);
    SAFE_RELEASE(g_pRTTextures[0]);
    SAFE_RELEASE(g_pRTTextures[1]);
    SAFE_RELEASE(g_pRtv);
    SAFE_RELEASE(g_pDsv);
    SAFE_RELEASE(g_pSwapChain);
    SAFE_RELEASE(g_pImmediateContext);
    SAFE_RELEASE(g_pD3DDevice);
}


void Update()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    time.QuadPart -= g_startTime.QuadPart;
    double fTime = static_cast<float>(g_secondsPerCount) * time.QuadPart;

    float x = static_cast<float>(1.5 * cos(fTime));
    float y = static_cast<float>(0.75 * sin(fTime));

    XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -3, 1), XMVectorSet(x, y, 0, 1), XMVectorSet(0, 1, 0, 0));
    XMStoreFloat4x4(&g_viewMatrix, view);

    g_cube.Update(g_pImmediateContext, view * XMLoadFloat4x4(&g_projMatrix));
}


void Draw(unsigned int rtIdx)
{
    FLOAT clearColor[4] = { 76.0f / 255.0f, 76.0f / 255.0f, 127.0f / 255.0f, 1.0f };

    // draw into rendertarget with id rtIdx
    g_pImmediateContext->RSSetViewports(1, &g_viewPort);
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRTEncoding[rtIdx], g_pDsv);
    g_pImmediateContext->ClearRenderTargetView(g_pRTEncoding[rtIdx], clearColor);
    g_pImmediateContext->ClearDepthStencilView(g_pDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    g_cube.Draw(g_pImmediateContext);

    // copy rendertarget into backbuffer (not required for streaming)
    D3D11_BOX resourceSize;
    resourceSize.left = 0;
    resourceSize.right = g_stream_width;
    resourceSize.top = 0;
    resourceSize.bottom = g_stream_height;
    resourceSize.front = 0;
    resourceSize.back = 1;

    ID3D11Resource* dst;
    g_pRtv->GetResource(&dst);

    g_pImmediateContext->CopySubresourceRegion(dst, 0, 0, 0, 0, g_pRTTextures[rtIdx], 0, &resourceSize);

    SAFE_RELEASE(dst);

    g_pSwapChain->Present(1, 0);
}