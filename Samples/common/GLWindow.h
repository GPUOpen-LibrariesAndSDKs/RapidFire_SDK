#pragma once

#include <string>

#include <GL/glew.h>
#include <GL/wglew.h>

class GLWindow
{
public:

    GLWindow(const std::string& strWindowName, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiPosX, unsigned int uiPosY, bool bFullScreen);

    GLWindow(GLWindow&& other);

    virtual ~GLWindow();

    void    open() const;
    void    close() const;

    void    makeCurrent() const;
    void    releaseContext() const;

    void    resize(unsigned int w, unsigned int h);

    operator bool()     const            { return m_bWindowCreated; }

    HDC     getDC()     const            { return m_hDC;   }
    HWND    getWindow() const            { return m_hWND;  }
    HGLRC   getGLRC()   const            { return m_hGLRC; }

    unsigned int getWidth()  const       { return m_uiWidth; }
    unsigned int getHeight() const       { return m_uiHeight; }

private:

    bool         create();

    HDC                     m_hDC;
    HWND                    m_hWND;
    HGLRC                   m_hGLRC;

    std::string const       m_strWindowName;

    bool                    m_bWindowCreated;
    bool                    m_bFullScreen;
    unsigned int            m_uiWidth;
    unsigned int            m_uiHeight;
    unsigned int const      m_uiPosX;
    unsigned int const      m_uiPosY;

    GLWindow(GLWindow const& w);

    GLWindow operator=(GLWindow const rhs);
};
