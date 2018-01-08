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
// Desktop Capture shows how to use RapidFire to grab the desktop.
//
// First a desktop session is created. The session is configured to use blocking calls.
// In this case the encode function will block until the desktop has changed.
// Since we do not want to compress the desktop, the identity encoder is used which
// will just return the RGBA data.
// The resulting pixels are dumped to an rgb file.
// To limit the file size, the desktop is scaled down by 50%.
/////////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <sstream>

#include <windows.h>

#include "RFWrapper.hpp"

#define NUM_FRAMES 3

using namespace std;

extern void writeRGBAimage(unsigned char* outbuf, int xsize, int ysize, const char* fname);


int main(int argc, char** argv)
{
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    const RFWrapper& rfDll = RFWrapper::getInstance();

    RFProperties props[] = { RF_ENCODER,                    static_cast<RFProperties>(RF_IDENTITY),
                             RF_DESKTOP,                    static_cast<RFProperties>(1),
                             RF_DESKTOP_BLOCK_UNTIL_CHANGE, static_cast<RFProperties>(1),
                             RF_FLIP_SOURCE,                static_cast<RFProperties>(1),
                             0 };

    rfStatus = rfDll.rfFunc.rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        cerr << "Failed to create desktop identity encoding session!" << endl;
        return -1;
    }

    cout << "Created desktop session!" << endl;

    unsigned int uiStreamWidth  = GetSystemMetrics(SM_CXSCREEN);
    unsigned int uiStreamHeight = GetSystemMetrics(SM_CYSCREEN);

    // Scale down resolution
    uiStreamWidth  /= 2;
    uiStreamHeight /= 2;

    cout << "   Stream width  " << uiStreamWidth << endl;
    cout << "   Stream height " << uiStreamHeight << endl;

    // create identity encoder and set format to RGBA8.
    RFProperties encoderProps[] = { RF_ENCODER_FORMAT, RF_RGBA8, 0 };

    rfStatus = rfDll.rfFunc.rfCreateEncoder2(rfSession, uiStreamWidth, uiStreamHeight, encoderProps);

    if (rfStatus != RF_STATUS_OK)
    {
        cerr << "Failed to create identity encoder!" << endl;
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    cout << "Created identity encoder" << endl;

    unsigned int uiBitStreamSize = 0;
    void*        pBitStreamdata  = nullptr;

    cout << "Starting to encode " << NUM_FRAMES << " frames" << endl;

    for (int i = 0; i < NUM_FRAMES; ++i)
    {
        if (rfDll.rfFunc.rfEncodeFrame(rfSession, 0) == RF_STATUS_OK)
        {
            // Check if encoded frame is ready
            if (rfDll.rfFunc.rfGetEncodedFrame(rfSession, &uiBitStreamSize, &pBitStreamdata) == RF_STATUS_OK)
            {
                if (uiBitStreamSize > 0)
                {
                    stringstream  path;

                    path << "RGBA8_frame_" << i << ".rgb";

                    cout << "\nDumping " << path.str();

                    // export rgba images to .rgb files that can be viewed with gimp
                    writeRGBAimage(static_cast<unsigned char*>(pBitStreamdata), uiStreamWidth, uiStreamHeight, path.str().c_str());
                }
            }
        }
    }

    rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    cout << "\nDumped frames to file" << endl;

    return 0;
}