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
// Desktop encoding shows how to use RapidFire to grab the desktop and encode it to
// a H264 stream. The stream is dumped to a file.
// First a session is created which is configured to grab the desktop and to use
// the AMF encoder (HW encoder).
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <thread>

#include <windows.h>
#include <dxgi.h>

#include "RFWrapper.hpp"
#include "../common/Timer.h"
#include <..\..\external\AMF\include\components\VideoEncoderHEVC.h>

#define NUM_FRAMES 3600 // record desktop for one minute

using namespace std;

bool                    g_running = true;
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

int main(int argc, char** argv)
{
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    RFProperties props[] = { RF_ENCODER,                  static_cast<RFProperties>(RF_AMF),
                             RF_DESKTOP_DSP_ID,           static_cast<RFProperties>(1),
                             0 };

    rfStatus = rfDll.rfFunc.rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        cerr << "Failed to create desktop encoding session!" << endl;
        return -1;
    }

    cout << "Created desktop session" << endl;

    // Get dimension of primary display
    unsigned int uiStreamWidth  = GetSystemMetrics(SM_CXSCREEN);
    unsigned int uiStreamHeight = GetSystemMetrics(SM_CYSCREEN);

    // Make sure dimension does not exceed 1080p.
    // This is the max resolution supported by the H.264 level 4.2 used in the preset.
    if (uiStreamWidth > 1920)
    {
        uiStreamWidth = 1920;
    }

    if (uiStreamHeight > 1080)
    {
        uiStreamHeight = 1080;
    }

    cout << "   Stream width  " << uiStreamWidth << endl;
    cout << "   Stream height " << uiStreamHeight << endl;

    // Create encoder and define the size of the stream.
    // RF will scale the desktop to the screen size.
    rfStatus = rfDll.rfFunc.rfCreateEncoder(rfSession, uiStreamWidth, uiStreamHeight, RF_PRESET_BALANCED);

    if (rfStatus != RF_STATUS_OK)
    {
        cerr << "Failed to create HW encoder!" << endl;
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    cout << "Created encoder" << endl;

    IDXGIFactory* dxgiFactory;
    auto hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&dxgiFactory);
    if (hr != S_OK)
    {
        cerr << "Failed to create dxgi factory!" << endl;
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    IDXGIAdapter* pAdapter;
    hr = dxgiFactory->EnumAdapters(0, &pAdapter);
    if (hr != S_OK)
    {
        cerr << "Failed to get dxgi adapter!" << endl;
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    IDXGIOutput* output;
    hr = pAdapter->EnumOutputs(0, &output);
    if (hr != S_OK)
    {
        cerr << "Failed to get display output!" << endl;
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    std::thread reader(ReaderThread, rfSession, "Desktop.h264");

    unsigned int uiBitStreamSize = 0;
    void*        pBitStreamdata = nullptr;

    cout << "Starting to encode " << NUM_FRAMES << " frames" << endl;

    for (int i = 0; i < NUM_FRAMES; ++i)
    {
        // Wait for the VSync so we are capturing the desktop right after it
        output->WaitForVBlank();

        // Wait 1ms to give the DWM time to flip
        Sleep(1);

        while(rfDll.rfFunc.rfEncodeFrame(rfSession, 0) == RF_STATUS_QUEUE_FULL)
        {
            // Encoding queue is full,
            // Give the consumer thread time to process output
            Sleep(0);
        }
    }

    g_running = false;
    reader.join();

    rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    cout << "Dumped frames to file" << endl;

    return 0;
}