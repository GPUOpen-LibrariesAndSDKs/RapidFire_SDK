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

#include <GL/glew.h>
#include <GL/wglew.h>
#include <windows.h>

#include "FireCube.h"
#include "GLRenderTarget.h"
#include "RapidFireServer.h"

HWND			g_hWnd  = NULL;
HDC				g_hDC   = NULL;
HGLRC			g_hGlrc = NULL;

bool            g_running = true;

unsigned int    g_stream_width  = 1280;
unsigned int    g_stream_height =  720;
unsigned int    g_win_width     = g_stream_width;
unsigned int    g_win_height    = g_stream_height;

bool    OpenWindow(LPCSTR cClassName, LPCSTR cWindowName );
void    CloseWindow();
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void MyDebugFunc(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
    MessageBox(NULL, message, "GL Debug Message", MB_ICONWARNING | MB_OK);
}


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
    WNDCLASSEX		    wndclass    = {};
    const LPCSTR        cClassName  = "OGL";
    const LPCSTR	    cWindowName = "GL FBO Encoding";
    RFStatus            rfStatus    = RF_STATUS_OK;
    RFEncodeSession     rfSession   = nullptr;

    // Register WindowClass
    wndclass.cbSize         = sizeof(WNDCLASSEX);
    wndclass.style          = CS_OWNDC;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.hInstance      = static_cast<HINSTANCE>(GetModuleHandle(NULL));
    wndclass.hIcon		    = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = NULL;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = cClassName;
    wndclass.hIconSm		= LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wndclass))
    {
        return FALSE;
    }
    
    if (!OpenWindow(cClassName, cWindowName))
    {
        MessageBox(NULL, "Failed to create window!", "RF Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    RFProperties props[] = { RF_ENCODER,         static_cast<RFProperties>(RF_AMF),       // Use HW H.264 encoder
                             RF_GL_DEVICE_CTX,   reinterpret_cast<RFProperties>(g_hDC),        // pass DC and GLRC to RF
                             RF_GL_GRAPHICS_CTX, reinterpret_cast<RFProperties>(g_hGlrc),
                             0 };
    
    // Create encoding session
    rfStatus = rfCreateEncodeSession(&rfSession, props);
    
    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);        
        return -1;
    }
      
    // Create encoder using the quality preset
    rfStatus = rfCreateEncoder(rfSession, g_stream_width, g_stream_height, RF_PRESET_BALANCED);

    if (rfStatus != RF_STATUS_OK)
    {
        MessageBox(NULL, "Failed to create RF Session!", "RF Error", MB_ICONERROR | MB_OK);
        rfDeleteEncodeSession(&rfSession);
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
            rfDeleteEncodeSession(&rfSession);
            return -1;
        }
    }

    // Register FBOs to RapidFire session
    bool success = (RF_STATUS_OK == rfRegisterRenderTarget(rfSession, reinterpret_cast<RFRenderTarget>(static_cast<uintptr_t>(fbo[0].getColorTex())), g_stream_width, g_stream_height, &rf_fbo_idx[0]));
    success &= (RF_STATUS_OK == rfRegisterRenderTarget(rfSession, reinterpret_cast<RFRenderTarget>(static_cast<uintptr_t>(fbo[1].getColorTex())), g_stream_width, g_stream_height, &rf_fbo_idx[1]));

    if (!success)
    {
        MessageBox(NULL, "Failed to register FBO", "RF Error", MB_ICONERROR | MB_OK);
        rfDeleteEncodeSession(&rfSession);
        return -1;
    }

    // create thread that dumps encoded frames to file.
    std::thread reader(ReaderThread, rfSession, "GLStream.h264");

    FireCube  gl_cube;

    if (!gl_cube.init())
    {
        MessageBox(NULL, "Failed to create cube", "RF Error", MB_ICONERROR | MB_OK);
        rfDeleteEncodeSession(&rfSession);
        return -1;
    }
        
    unsigned int uiIndex = 0;
    float        angle   = 0.0f;

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
            rfStatus = rfEncodeFrame(rfSession, rf_fbo_idx[uiIndex]);

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
        glViewport(0, 0, g_win_width, g_win_height);

        fbo[uiIndex].draw();

        SwapBuffers(g_hDC);

        // Switch buffer to use
        uiIndex = 1 - uiIndex;

        angle += 0.1f;
    } 

    reader.join();

    CloseWindow();

    rfDeleteEncodeSession(&rfSession);

    UnregisterClass(cClassName, hInst);
    
    return static_cast<int>(msg.wParam);
}


void Resize(unsigned int w, unsigned int h)
{
    g_win_width  = w;
    g_win_height = h;
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

        case WM_CREATE:
            return 0;

        case WM_SIZE:
            Resize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


bool OpenWindow(LPCSTR cClassName, LPCSTR cWindowName )
{
    g_hWnd = CreateWindow(cClassName, 
                          cWindowName,
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          g_win_width,
                          g_win_height,
                          NULL,
                          NULL,
                          static_cast<HINSTANCE>(GetModuleHandle(NULL)),
                          NULL);

    if (!g_hWnd)
    {
        return false;
    }

    
    static PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize           = sizeof(PIXELFORMATDESCRIPTOR); 
    pfd.nVersion        = 1; 
    pfd.dwFlags         = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL  | PFD_DOUBLEBUFFER ;
    pfd.iPixelType      = PFD_TYPE_RGBA; 
    pfd.cColorBits      = 24; 
    pfd.cRedBits        = 8; 
    pfd.cGreenBits      = 8; 
    pfd.cBlueBits       = 8; 
    pfd.cAlphaBits      = 8;
    pfd.cDepthBits      = 24; 
    pfd.cStencilBits    = 8; 
    pfd.iLayerType      = PFD_MAIN_PLANE;
    
    g_hDC = GetDC(g_hWnd);

    if (!g_hDC)
    {
        return false;
    }

    int mPixelFormat = ChoosePixelFormat(g_hDC, &pfd);

    if (!mPixelFormat)
    {
        return false;
    }

    if (!SetPixelFormat(g_hDC, mPixelFormat, &pfd))
    {
        return false;
    }

    g_hGlrc = wglCreateContext(g_hDC);

    if (!wglMakeCurrent(g_hDC, g_hGlrc))
    {
        return false;
    }

    if (glewInit() != GLEW_OK)
    {
        return false;
    }
    
    if (WGLEW_ARB_create_context)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_hGlrc);

        int attribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                          WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                          WGL_CONTEXT_PROFILE_MASK_ARB , WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#ifdef _DEBUG             
                          WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif                    
                          0}; 

        g_hGlrc = wglCreateContextAttribsARB(g_hDC, 0, attribs);

        if (g_hGlrc)
        {
            wglMakeCurrent(g_hDC, g_hGlrc);
        }
        else
        {
            return false;
        }
    }

    if (!wglMakeCurrent(g_hDC, g_hGlrc))
    {
        return false;
    }

    if (GLEW_AMD_debug_output)
    {
        glDebugMessageCallbackAMD(reinterpret_cast<GLDEBUGPROCAMD>(&MyDebugFunc), NULL);
    }

    glViewport(0, 0, g_stream_width, g_stream_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(60.0f, static_cast<float>(g_stream_width) / static_cast<float>(g_stream_height), 0.1f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    ShowWindow(g_hWnd, SW_SHOWDEFAULT);

    UpdateWindow(g_hWnd);

    return true;
}


void CloseWindow()
{
    if (g_hGlrc)
    {
        wglMakeCurrent(g_hDC, NULL);

        wglDeleteContext(g_hGlrc);

        ReleaseDC(g_hWnd, g_hDC);

        DestroyWindow(g_hWnd);
    }
}