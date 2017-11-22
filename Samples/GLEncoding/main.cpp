/////////////////////////////////////////////////////////////////////////////////////////
//
// GLEncoding shows how to use RapidFire to create an H264 encoded stream from a
// GL Frame-buffer.
// First a RF session is created passing the rendering context. the session is configure
// to use the AMF encoder (HW encoding). The FBOs that are used by the application are
// registered, now the application can render to those FBOs and encodeFrame will use
// them as input for the encoder and return a H264 frame that is dumped to a file.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <string>
#include <thread>

#include <windows.h>

#include "FireCube.h"
#include "GLRenderTarget.h"
#include "GLWindow.h"
#include "RFWrapper.hpp"

bool            g_running = true;

unsigned int    g_stream_width  = 1280;
unsigned int    g_stream_height =  720;

const RFWrapper& rfDll = RFWrapper::getInstance();

void ReaderThread(const RFEncodeSession& session, const std::string& file_name)
{
    void*           p_bit_stream   = nullptr;
    unsigned int    bitstream_size = 0;
    RFStatus        rf_status      = RF_STATUS_OK;
    std::ofstream   out_file;

    if (file_name.size() > 0)
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
    RFStatus        rfStatus = RF_STATUS_OK;
    RFEncodeSession rfSession = nullptr;

    GLWindow win("OpenGL Encoding", g_stream_width, g_stream_height, CW_USEDEFAULT, CW_USEDEFAULT, false, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);

    if (!win)
    {
        MessageBox(NULL, "Failed to create output window!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    win.open();

    RFProperties props[] = { RF_ENCODER,         static_cast<RFProperties>(RF_AMF),       // Use HW H.264 encoder
                             RF_GL_DEVICE_CTX,   reinterpret_cast<RFProperties>(win.getDC()),        // pass DC and GLRC to RF
                             RF_GL_GRAPHICS_CTX, reinterpret_cast<RFProperties>(win.getGLRC()),
                             0 };

    // Create encoding session
    rfStatus = rfDll.rfFunc.rfCreateEncodeSession(&rfSession, props);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    // Create encoder using the quality preset
    rfStatus = rfDll.rfFunc.rfCreateEncoder(rfSession, g_stream_width, g_stream_height, RF_PRESET_BALANCED);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    GLRenderTarget  fbo[2];
    unsigned int    rf_fbo_idx[2];

    // Create FBOs that will be encoded by the RF encoder
    for (int i = 0; i < 2; ++i)
    {
        if (!fbo[i].createBuffer(g_stream_width, g_stream_height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE))
        {
            MessageBox(NULL, "Failed to create FBO", "RF Error", MB_ICONERROR | MB_OK);
            rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
            return -1;
        }
    }

    // Register FBOs to RapidFire session
    bool success = (RF_STATUS_OK == rfDll.rfFunc.rfRegisterRenderTarget(rfSession, reinterpret_cast<RFRenderTarget>(static_cast<uintptr_t>(fbo[0].getColorTex())), g_stream_width, g_stream_height, &rf_fbo_idx[0]));
    success &= (RF_STATUS_OK == rfDll.rfFunc.rfRegisterRenderTarget(rfSession, reinterpret_cast<RFRenderTarget>(static_cast<uintptr_t>(fbo[1].getColorTex())), g_stream_width, g_stream_height, &rf_fbo_idx[1]));

    if (!success)
    {
        MessageBox(NULL, "Failed to register FBO", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    // create thread that dumps encoded frames to file.
    std::thread reader(ReaderThread, rfSession, "GLStream.h264");

    FireCube  gl_cube;

    if (!gl_cube.init())
    {
        MessageBox(NULL, "Failed to create cube", "RF Error", MB_ICONERROR | MB_OK);
        rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    unsigned int uiIndex = 0;
    float        angle   = 0.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(60.0f, static_cast<float>(g_stream_width) / static_cast<float>(g_stream_height), 0.1f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

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
            // In this case we need to wait until the reader thread has called rfGetEncodedFrame and removed a frame from the queue.
            rfStatus = rfDll.rfFunc.rfEncodeFrame(rfSession, rf_fbo_idx[uiIndex]);

            if (rfStatus == RF_STATUS_QUEUE_FULL)
            {
                // Give other thread a chance
                Sleep(0);
            }

        } while (rfStatus == RF_STATUS_QUEUE_FULL);

        // Render scene to FBO
        fbo[uiIndex].bind();

        glViewport(0, 0, g_stream_width, g_stream_height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPushMatrix();
        glTranslatef(0.0f, 0.0f, -2.0f);
        glRotatef(angle, 1.0f, 1.0f, 0.0f);

        gl_cube.draw();

        glPopMatrix();

        fbo[uiIndex].unbind();

        // Blit FBO to window
        glViewport(0, 0, win.getWidth(), win.getHeight());

        fbo[uiIndex].draw();

        SwapBuffers(win.getDC());

        // Switch buffer to use
        uiIndex = 1 - uiIndex;

        angle += 0.1f;
    }

    reader.join();

    rfDll.rfFunc.rfDeleteEncodeSession(&rfSession);

    return static_cast<int>(msg.wParam);
}