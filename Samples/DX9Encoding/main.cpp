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
// DX9Encoding shows how to use RapidFire to create an H264 encoded stream from a
// DX9 surface.
// First a RF session is created passing the DirectX 9 device. The session is configured
// to use the AMF encoder (HW encoding). The rendertargets that are used by the application are
// registered, now the application can render to those rendertargets and encodeFrame will use
// them as input for the encoder and return a H264 frame that is dumped to a file.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <string>

#include <d3d9.h>
#include <windows.h>
#include <windowsx.h>

#include "RFWrapper.hpp"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != nullptr)     \
   {                    \
      x->Release();     \
      x = nullptr;      \
   }
#endif

HWND                    g_hWnd = NULL;
IDirect3D9Ex*           g_pD3D9Ex = nullptr;
IDirect3DDevice9Ex*     g_pD3DDevice = nullptr;
IDirect3DVertexBuffer9* g_pVBuffer = nullptr;
IDirect3DSurface9*      g_pDeviceRts = nullptr;
IDirect3DSurface9*      g_pRts[2] = { nullptr, nullptr };

unsigned int            g_stream_width = 1280;
unsigned int            g_stream_height = 720;

bool OpenWindow(LPCSTR cClassName, LPCSTR cWindowName);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool InitD3D9(HWND hWnd);
void Draw(unsigned int rtIdx);
void ReleaseD3D9();


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    RFStatus             rfStatus = RF_STATUS_OK;
    RFEncodeSession      rfSession = nullptr;

    const RFWrapper& rfDll = RFWrapper::getInstance();

    LPCSTR cClassName  = "DX9WindowClass";
    LPCSTR cWindowName = "DX9 Encoding";

    if (!OpenWindow(cClassName, cWindowName))
    {
        MessageBox(NULL, "Failed to create window!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    if (!InitD3D9(g_hWnd))
    {
        MessageBox(NULL, "Failed to init D3D9!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    RFProperties props[] = { RF_D3D9EX_DEVICE        , (RFProperties)g_pD3DDevice,
                             RF_ENCODER              , (RFProperties)RF_AMF,
                             0 };

    // Create encoding session
    rfStatus = rfDll.rfFunc.rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    rfStatus = rfDll.rfFunc.rfCreateEncoder(rfSession, g_stream_width, g_stream_height, RF_PRESET_BALANCED);
    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF encoder!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    unsigned int rf_fbo_idx[2];

    // Register rendertargets to the RapidFire session
    bool success = (RF_STATUS_OK == rfDll.rfFunc.rfRegisterRenderTarget(rfSession, static_cast<RFRenderTarget>(g_pRts[0]), g_stream_width, g_stream_height, &rf_fbo_idx[0]));
    success &= (RF_STATUS_OK == rfDll.rfFunc.rfRegisterRenderTarget(rfSession, static_cast<RFRenderTarget>(g_pRts[1]), g_stream_width, g_stream_height, &rf_fbo_idx[1]));

    if (!success)
    {
        MessageBox(NULL, "Failed to register rendertargets", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    void* p_bit_stream = nullptr;
    unsigned int bitstream_size = 0;
    unsigned int uiIndex = 0;
    std::ofstream out_file;
    out_file.open("DX9Stream.h264", std::fstream::out | std::fstream::trunc | std::fstream::binary);
    MSG msg = {};

    rfDll.rfFunc.rfEncodeFrame(rfSession, rf_fbo_idx[uiIndex]);
    uiIndex = 1 - uiIndex;

    for (;;)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
        {
            break;
        }

        rfDll.rfFunc.rfEncodeFrame(rfSession, rf_fbo_idx[uiIndex]);

        if (rfDll.rfFunc.rfGetEncodedFrame(rfSession, &bitstream_size, &p_bit_stream) == RF_STATUS_OK)
        {
            out_file.write(static_cast<char*>(p_bit_stream), bitstream_size);
        }

        Draw(uiIndex);

        // Switch rendertarget to use
        uiIndex = 1 - uiIndex;
    }

    out_file.close();

    rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    ReleaseD3D9();

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
    WNDCLASSEX		    wndclass = {};
    const LPCSTR        cClassName = cClassName_;
    const LPCSTR	    cWindowName = cWindowName_;

    // Register WindowClass
    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = static_cast<HINSTANCE>(GetModuleHandle(NULL));
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    wndclass.lpszClassName = cClassName;
    wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

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


struct Vertex
{
    float x, y, z, w;
    DWORD color;
};


bool InitD3D9(HWND hWnd)
{
    HRESULT hr = S_OK;

    hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pD3D9Ex);
    if (hr != S_OK)
    {
        return false;
    }

    D3DPRESENT_PARAMETERS d3DPresentParams = {};
    d3DPresentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3DPresentParams.BackBufferWidth = g_stream_width;
    d3DPresentParams.BackBufferHeight = g_stream_height;
    d3DPresentParams.hDeviceWindow = hWnd;
    d3DPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3DPresentParams.Windowed = TRUE;

    hr = g_pD3D9Ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3DPresentParams, nullptr, &g_pD3DDevice);
    if (hr != S_OK)
    {
        return false;
    }

    hr = g_pD3DDevice->GetRenderTarget(0, &g_pDeviceRts);
    if (hr != S_OK)
    {
        return false;
    }

    hr = g_pD3DDevice->CreateRenderTarget(g_stream_width, g_stream_height, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &g_pRts[0], nullptr);
    if (hr != S_OK)
    {
        return false;
    }

    hr = g_pD3DDevice->CreateRenderTarget(g_stream_width, g_stream_height, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &g_pRts[1], nullptr);
    if (hr != S_OK)
    {
        return false;
    }

    Vertex vertices[] =
    {
        { 640.0f,  50.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(255, 0, 0), },
        { 990.0f, 670.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(0, 255, 0), },
        { 290.0f, 670.0f, 0.5f, 1.0f, D3DCOLOR_XRGB(0, 0, 255), },
    };

    hr = g_pD3DDevice->CreateVertexBuffer(3 * sizeof(Vertex), 0, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVBuffer, nullptr);
    if (hr != S_OK)
    {
        return false;
    }

    void* ptr;
    g_pVBuffer->Lock(0, 0, &ptr, 0);
    if (hr != S_OK)
    {
        return false;
    }

    memcpy(ptr, vertices, sizeof(vertices));
    g_pVBuffer->Unlock();
    if (hr != S_OK)
    {
        return false;
    }

    return true;
}


void Draw(unsigned int rtIdx)
{
    g_pD3DDevice->SetRenderTarget(0, g_pRts[rtIdx]);

    g_pD3DDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    g_pD3DDevice->BeginScene();

    g_pD3DDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

    g_pD3DDevice->SetStreamSource(0, g_pVBuffer, 0, sizeof(Vertex));

    g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    g_pD3DDevice->EndScene();

    // Copy surface to backbuffer and present it. (This part is not required for streaming)
    RECT test;
    test.bottom = g_stream_height;
    test.left = 0;
    test.right = g_stream_width;
    test.top = 0;
    g_pD3DDevice->StretchRect(g_pRts[rtIdx], nullptr, g_pDeviceRts, &test, D3DTEXF_NONE);

    g_pD3DDevice->Present(nullptr, nullptr, NULL, nullptr);
}

void ReleaseD3D9()
{
    SAFE_RELEASE(g_pRts[0]);
    SAFE_RELEASE(g_pRts[1]);
    SAFE_RELEASE(g_pDeviceRts);
    SAFE_RELEASE(g_pVBuffer);
    SAFE_RELEASE(g_pD3DDevice);
    SAFE_RELEASE(g_pD3D9Ex);
}