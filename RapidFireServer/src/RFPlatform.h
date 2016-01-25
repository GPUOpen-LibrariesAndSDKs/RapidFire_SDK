/*****************************************************************************
* Copyright (C) 2013 Advanced Micro Devices, Inc.
* All rights reserved.
*
* This software is provided by the copyright holders and contributors "As is"
* And any express or implied warranties, including, but not limited to, the
* implied warranties of merchantability, non-infringement, and fitness for a
* particular purpose are disclaimed. In no event shall the copyright holder or
* contributors be liable for any direct, indirect, incidental, special,
* exemplary, or consequential damages (including, but not limited to,
* procurement of substitute goods or services; loss of use, data, or profits;
* or business interruption) however caused and on any theory of liability,
* whether in contract, strict liability, or tort (including negligence or
* otherwise) arising in any way out of the use of this software, even if
* advised of the possibility of such damage.
*****************************************************************************/
#pragma once

#if defined WIN32 || defined _WIN32

#include <D3D11.h>
#include <d3d9.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <windows.h>

#define DeviceCtx       HDC
#define GraphicsCtx     HGLRC
#define Window          HWND

#define D3D9Device      IDirect3DDevice9*
#define D3D11Device     ID3D11Device*

#else // if defined WIN32 || defined _WIN32

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GL/glxew.h"
#include <X11/keysym.h>
#include <X11/Xlib.h>

#define DeviceCtx       Display*
#define GraphicsCtx     GLXContext

#endif // defined WIN32 || defined _WIN32