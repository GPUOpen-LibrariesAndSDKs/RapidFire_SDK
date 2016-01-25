/////////////////////////////////////////////////////////////////////////////////////////
// 
// Desktop Capture shows how to use RapidFire to grab the desktop
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

#include "RapidFireServer.h"

#define NUM_FRAMES 3

using namespace std;

extern void writeRGBAimage(unsigned char* outbuf, int xsize, int ysize, const char* fname);


int main(int argc, char** argv)
{
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    RFProperties props[] = { RF_ENCODER,                    static_cast<RFProperties>(RF_IDENTITY),
                             RF_DESKTOP,                    static_cast<RFProperties>(1),
                             RF_DESKTOP_BLOCK_UNTIL_CHANGE, static_cast<RFProperties>(1),
                             0 };

    rfStatus = rfCreateEncodeSession(&rfSession, props);

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

    // create identity encoder and request ARGB8 format.
    RFProperties encoderProps[] = { RF_ENCODER_FORMAT, RF_RGBA8, 0 };

    rfStatus = rfCreateEncoder2(rfSession, uiStreamWidth, uiStreamHeight, encoderProps);

    if (rfStatus != RF_STATUS_OK)
    {
        cerr << "Failed to create identity encoder!" << endl;

        rfDeleteEncodeSession(&rfSession);

        return -1;
    }

    cout << "Created identity encoder" << endl;

    unsigned int uiBuffer        = 0;
    unsigned int uiBitStreamSize = 0;
    void*        pBitStreamdata  = nullptr;

    cout << "Starting to encode " << NUM_FRAMES << " frames" << endl;

    for (int i = 0; i < NUM_FRAMES; ++i)
    {
        if (rfEncodeFrame(rfSession, uiBuffer) == RF_STATUS_OK)
        {
            // Check if encoded frame is ready
            if (rfGetEncodedFrame(rfSession, &uiBitStreamSize, &pBitStreamdata) == RF_STATUS_OK)
            {
                if (uiBitStreamSize > 0)
                {
                    stringstream  path;

                    path << "RGBA8_frame_" << i << ".rgb";

                    cout << "\nDumping " << path.str();

                    // export argb images to .rgb files that can be viewed with gimp
                    writeRGBAimage(static_cast<unsigned char*>(pBitStreamdata), uiStreamWidth, uiStreamHeight, path.str().c_str());
                }
            }
        }

        uiBuffer = 1 - uiBuffer;
    }

    rfDeleteEncodeSession(&rfSession);

    cout << "Dumped frames to file" << endl;

    return 0;
}