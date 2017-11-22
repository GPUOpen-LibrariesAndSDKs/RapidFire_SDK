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
// registered, the application then renders into an offscreen render target and
// uses that to composite the render target used for encoding.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <string>
#include <thread>

#include <windows.h>

#include "..\..\external\AMF\include\components\VideoEncoderVCE.h"
#include "..\common\DX11Window.h"
#include "..\common\DX11RenderTarget.h"
#include "..\common\Timer.h"
#include "StretchRectShader.h"
#include "Cube.h"
#include "RFWrapper.hpp"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != nullptr)     \
   {                    \
      x->Release();     \
      x = nullptr;      \
   }
#endif

DX11RenderTarget		g_RTSource;
DX11RenderTarget		g_RTEncoding[2];
StretchRectShader       g_stretchRectShader;
Cube					g_cube;
Timer                   g_timer;

bool                    g_running = true;
unsigned int            g_stream_width = 1280;
unsigned int            g_stream_height = 720;
DX11Window              g_win;

bool CreateDX11Resources();
void DestroyDX11Resources();
void Update();
void Draw(unsigned int rtIdx);

const RFWrapper& rfDll = RFWrapper::getInstance();

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
        rf_status = rfDll.rfFunc.rfGetEncodedFrame(session, &bitstream_size, &p_bit_stream);

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

    if (!g_win.create("DirectX 11 Encoding", g_stream_width, g_stream_height, CW_USEDEFAULT, CW_USEDEFAULT))
    {
        MessageBox(NULL, "Failed to create output window!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    g_win.open();

    if (!CreateDX11Resources())
    {
        MessageBox(NULL, "Failed to create DirectX 11 Resources!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    RFProperties props[] = { RF_ENCODER,      static_cast<RFProperties>(RF_AMF),       // Use HW H.264 encoder
                             RF_D3D11_DEVICE, reinterpret_cast<RFProperties>(g_win.getDevice()), // pass device to RF
                             0 };

    // Create encoding session
    rfStatus = rfDll.rfFunc.rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    // Set encoder paramters to the balanced preset manually for rfCreateEncoder2
    RFProperties encoder_props[] = { RF_ENCODER_FORMAT,               static_cast<RFProperties>(RF_NV12),
                                     RF_ENCODER_BITRATE,              static_cast<RFProperties>(10000000),
                                     RF_ENCODER_PEAK_BITRATE,         static_cast<RFProperties>(10000000),
                                     RF_ENCODER_RATE_CONTROL_METHOD,  static_cast<RFProperties>(AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR),
                                     RF_ENCODER_MIN_QP,               static_cast<RFProperties>(18),
                                     RF_ENCODER_MAX_QP,               static_cast<RFProperties>(51),
                                     RF_ENCODER_FRAME_RATE,           static_cast<RFProperties>(60),
                                     RF_ENCODER_VBV_BUFFER_SIZE,      static_cast<RFProperties>(1000000),
                                     0 };

    rfStatus = rfDll.rfFunc.rfCreateEncoder2(rfSession, g_stream_width, g_stream_height, encoder_props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    unsigned int rf_fbo_idx[2];

    // Register rendertargets to the RapidFire session
    bool success = (RF_STATUS_OK == rfDll.rfFunc.rfRegisterRenderTarget(rfSession, static_cast<RFRenderTarget>(g_RTEncoding[0].getRenderTargetTexture()), g_stream_width, g_stream_height, &rf_fbo_idx[0]));
    success &= (RF_STATUS_OK == rfDll.rfFunc.rfRegisterRenderTarget(rfSession, static_cast<RFRenderTarget>(g_RTEncoding[1].getRenderTargetTexture()), g_stream_width, g_stream_height, &rf_fbo_idx[1]));

    if (!success)
    {
        MessageBox(NULL, "Failed to register rendertargets", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    // create thread that dumps encoded frames to file.
    std::thread reader(ReaderThread, rfSession, "DX11Stream.h264");

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
            rfStatus = rfDll.rfFunc.rfEncodeFrame(rfSession, rf_fbo_idx[uiIndex]);

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

    rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    DestroyDX11Resources();

    return static_cast<int>(msg.wParam);
}


bool CreateDX11Resources()
{
    if (!g_stretchRectShader.create(g_win.getDevice()))
    {
        return false;
    }

    if (!g_RTSource.create(g_win.getDevice(), g_stream_width, g_stream_height, DXGI_FORMAT_B8G8R8A8_UNORM))
    {
        return false;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (!g_RTEncoding[i].create(g_win.getDevice(), g_stream_width, g_stream_height, DXGI_FORMAT_B8G8R8A8_UNORM))
        {
            return false;
        }
    }

    if (!g_cube.create(g_win.getDevice(), g_stream_width, g_stream_height))
    {
        return false;
    }

    return true;
}


void DestroyDX11Resources()
{
    g_cube.release();
    g_RTSource.release();
    g_RTEncoding[0].release();
    g_RTEncoding[1].release();
    g_stretchRectShader.release();
}


void Update()
{
    g_cube.update(g_win.getContext(), g_timer.getTime());
}


void Draw(unsigned int rtIdx)
{
    FLOAT clearColorSource[4] = { 76.0f / 255.0f, 76.0f / 255.0f, 127.0f / 255.0f, 1.0f };
    auto context = g_win.getContext();

    // draw into source rendertarget with id rtIdx
    context->RSSetViewports(1, &g_win.getViewPort());
    g_RTSource.set(context);
    g_RTSource.clear(context, clearColorSource, 1.0f);

    g_cube.draw(context);

    // composite the render target for encoding
    FLOAT clearColorEncoding[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    g_RTEncoding[rtIdx].set(context);
    g_RTEncoding[rtIdx].clear(context, clearColorEncoding, 1.0f);

    float fTime = g_timer.getTime();
    RECT destRect;
    destRect.bottom = (g_stream_height * 7) / 8 + static_cast<LONG>(cos(fTime) * (g_stream_height / 8));
    destRect.left = g_stream_width / 8 + static_cast<LONG>(sin(fTime) * (g_stream_width / 8));
    destRect.right = (g_stream_width * 7) / 8 - static_cast<LONG>(sin(fTime) * (g_stream_width / 8));
    destRect.top = g_stream_height / 8 - static_cast<LONG>(cos(fTime) * g_stream_height / 8);

    RECT srcRect;
    srcRect.bottom = g_stream_height;
    srcRect.left = 0;
    srcRect.right = g_stream_width;
    srcRect.top = 0;

    g_stretchRectShader.stretchRect(context, g_RTSource.getShaderResource(), &srcRect, g_RTEncoding[rtIdx].getRenderTarget(), &destRect);

    // copy rendertarget into backbuffer of the swapchain (this part is not required for streaming)
    D3D11_BOX resourceSize;
    resourceSize.left = 0;
    resourceSize.right = g_stream_width;
    resourceSize.top = 0;
    resourceSize.bottom = g_stream_height;
    resourceSize.front = 0;
    resourceSize.back = 1;

    ID3D11Resource* dst;
    g_win.getRenderTarget()->GetResource(&dst);

    g_win.getContext()->CopySubresourceRegion(dst, 0, 0, 0, 0, g_RTEncoding[rtIdx].getRenderTargetTexture(), 0, &resourceSize);

    SAFE_RELEASE(dst);

    g_win.getSwapChain()->Present(1, 0);
}