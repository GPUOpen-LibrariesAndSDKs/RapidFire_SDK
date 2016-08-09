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

#include <Windows.h>

#include "GLWindow.h"

static void MyDebugFunc(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
#ifdef _DEBUG
    __debugbreak();
#endif
    MessageBox(NULL, message, "GL Debug Message", MB_ICONWARNING | MB_OK);
}

#define GLWINDOW_CLASSNAME "GLWindowClass"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


GLWindow::GLWindow(const std::string& strWindowName, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiPosX, unsigned int uiPosY, bool bFullScreen)
    : m_strWindowName(strWindowName)
    , m_uiWidth(uiWidth)
    , m_uiHeight(uiHeight)
    , m_uiPosX(uiPosX)
    , m_uiPosY(uiPosY)
    , m_bFullScreen(bFullScreen)
    , m_bMinimized(false)
    , m_hDC(NULL)
    , m_hWND(NULL)
    , m_hGLRC(NULL)
{
    create(WGL_CONTEXT_CORE_PROFILE_BIT_ARB);
}


GLWindow::GLWindow(const std::string& strWindowName, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiPosX, unsigned int uiPosY, bool bFullScreen, int32_t glContextProfileMaskARB)
    : m_strWindowName(strWindowName)
    , m_uiWidth(uiWidth)
    , m_uiHeight(uiHeight)
    , m_uiPosX(uiPosX)
    , m_uiPosY(uiPosY)
    , m_bFullScreen(bFullScreen)
    , m_bMinimized(false)
    , m_hDC(NULL)
    , m_hWND(NULL)
    , m_hGLRC(NULL)
{
    create(glContextProfileMaskARB);
}



GLWindow::GLWindow(GLWindow&& other)
    : m_strWindowName(std::move(other.m_strWindowName))
    , m_uiWidth(other.m_uiWidth)
    , m_uiHeight(other.m_uiHeight)
    , m_uiPosX(other.m_uiPosX)
    , m_uiPosY(other.m_uiPosY)
    , m_bFullScreen(other.m_bFullScreen)
    , m_bMinimized(other.m_bMinimized)
    , m_hWND(other.m_hWND)
    , m_hDC(other.m_hDC)
    , m_hGLRC(other.m_hGLRC)
{
    other.m_hWND  = NULL;
    other.m_hDC   = NULL;
    other.m_hGLRC = NULL;
}


GLWindow::~GLWindow(void)
{
    if (m_hGLRC)
    {
        wglMakeCurrent(m_hDC, NULL);

        wglDeleteContext(m_hGLRC);
    }

    if (m_hWND)
    {
        ReleaseDC(m_hWND, m_hDC);

        DestroyWindow(m_hWND);
    }
}


bool GLWindow::create(int32_t glContextProfileMaskARB)
{
    WNDCLASSEX wndClass = {};
    if (!GetClassInfoEx(static_cast<HINSTANCE>(GetModuleHandle(NULL)), GLWINDOW_CLASSNAME, &wndClass))
    {
        wndClass.cbSize        = sizeof(WNDCLASSEX);
        wndClass.style         = CS_OWNDC;
        wndClass.lpfnWndProc   = WndProc;
        wndClass.hInstance     = static_cast<HINSTANCE>(GetModuleHandle(NULL));
        wndClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
        wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndClass.lpszClassName = GLWINDOW_CLASSNAME;
        wndClass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

        if (!RegisterClassEx(&wndClass))
        {
            return false;
        }
    }

    RECT wndRect = { 0, 0, static_cast<LONG>(m_uiWidth), static_cast<LONG>(m_uiHeight) };
    AdjustWindowRect(&wndRect, WS_OVERLAPPEDWINDOW, false);

    m_hWND = CreateWindow(GLWINDOW_CLASSNAME,
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

    static PIXELFORMATDESCRIPTOR pfd = {};

    pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR); 
    pfd.nVersion     = 1; 
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA; 
    pfd.cColorBits   = 24; 
    pfd.cRedBits     = 8; 
    pfd.cGreenBits   = 8; 
    pfd.cBlueBits    = 8; 
    pfd.cAlphaBits   = 8;
    pfd.cDepthBits   = 24; 
    pfd.cStencilBits = 8; 
    pfd.iLayerType   = PFD_MAIN_PLANE;

    m_hDC = GetDC(m_hWND);

    if (!m_hDC)
    {
        return false;
    }

    int mPixelFormat = ChoosePixelFormat(m_hDC, &pfd);

    if (!mPixelFormat)
    {
        return false;
    }

    if (!SetPixelFormat(m_hDC, mPixelFormat, &pfd))
    {
        return false;
    }

    m_hGLRC = wglCreateContext(m_hDC);

    if (!wglMakeCurrent(m_hDC, m_hGLRC))
    {
        return false;
    }

    if (glewInit() != GLEW_OK)
    {
        return false;
    }

    if (WGLEW_ARB_create_context)
    {
        wglMakeCurrent(m_hDC, NULL);
        wglDeleteContext(m_hGLRC);

        int32_t attribs[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                          WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                          WGL_CONTEXT_PROFILE_MASK_ARB , glContextProfileMaskARB,
#ifdef _DEBUG             
                          WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif                    
                          0 };

        m_hGLRC = wglCreateContextAttribsARB(m_hDC, 0, attribs);

        if (m_hGLRC)
        {
            wglMakeCurrent(m_hDC, m_hGLRC);
        }
        else
        {
            return false;
        }
    }

    if (!wglMakeCurrent(m_hDC, m_hGLRC))
    {
        return false;
    }

    if (GLEW_AMD_debug_output)
    {
        glDebugMessageCallbackAMD(reinterpret_cast<GLDEBUGPROCAMD>(&MyDebugFunc), nullptr);
    }

    return true;
}


void GLWindow::open() const
{
    if (m_hWND)
    {
        ShowWindow(m_hWND, SW_SHOWDEFAULT);

        UpdateWindow(m_hWND);

        SetWindowLongPtr(m_hWND, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
}


void GLWindow::resize(unsigned int w, unsigned int h)
{
    m_uiWidth  = w;
    m_uiHeight = h;
}


void GLWindow::makeCurrent() const
{
    if (m_hWND)
    {
        wglMakeCurrent(m_hDC, m_hGLRC);
    }
}


void GLWindow::releaseContext() const
{
    if (m_hDC)
    {
        wglMakeCurrent(m_hDC, NULL);
    }
}


void GLWindow::close() const
{
    if (m_hWND)
    {
        ShowWindow(m_hWND, SW_HIDE);

        UpdateWindow(m_hWND);
    }
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    GLWindow* pWin = reinterpret_cast<GLWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

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

        case WM_SIZE:
            pWin->resize(LOWORD(lParam), HIWORD(lParam));
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
            if (wParam == SIZE_MINIMIZED)
            {
                pWin->minimized();
            }
            else
            {
                pWin->restored();
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}