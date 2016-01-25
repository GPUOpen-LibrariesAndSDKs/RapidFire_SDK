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

#include "RapidFireServer.h"

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