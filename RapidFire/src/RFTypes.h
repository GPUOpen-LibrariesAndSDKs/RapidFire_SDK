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

#pragma once

#include "RapidFire.h"

/* Support of multiple render targets
   1 : allow only single buffering
   2 : allow double buffering
   3 : allow triple buffering
 */
#define MAX_NUM_RENDER_TARGETS                        3

#define NUM_RESULT_BUFFERS                            3

enum RFParameterType { RF_PARAMETER_UNKNOWN = -1, RF_PARAMETER_BOOL = 0, RF_PARAMETER_INT = 1, RF_PARAMETER_UINT = 2, RF_PARAMETER_PTR = 3 };

enum RFParameterState { RF_PARAMETER_STATE_INVALID = 0, RF_PARAMETER_STATE_READY = 1, RF_PARAMETER_STATE_BLOCKED = 2 };

struct ID3D11Texture2D;
struct IDirect3DSurface9;

union RFTexture
{
    unsigned int        uiGLTexName;
    ID3D11Texture2D*    pDX11TexPtr;
    IDirect3DSurface9*  pDX9TexPtr;
    RFRenderTarget      rfRT;
};

enum RFCaptureSource
{
    RF_SOURCE_UNKNOWN               = -1,
    RF_SOURCE_RENDER_TARGET_GL      =  0,
    RF_SOURCE_RENDER_TARGET_D3D9    =  1,
    RF_SOURCE_RENDER_TARGET_D3D11   =  2,
    RF_SOURCE_DESKTOP               =  3,
    RF_SOURCE_WINDOW                =  4
};