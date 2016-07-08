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

/**
 ***************************************************************************************************
 * @file  VideoConverter.h
 * @brief AMFFVideoConverter interface declaration
 ***************************************************************************************************
 */
#ifndef __AMFVideoConverter_h__
#define __AMFVideoConverter_h__
#pragma once

#include "Component.h"

#define AMFVideoConverter L"AMFVideoConverter"

#define AMF_VIDEO_CONVERTER_OUTPUT_FORMAT       L"OutputFormat"             // Values : AMF_SURFACE_NV12 or AMF_SURFACE_BGRA or AMF_SURFACE_YUV420P
#define AMF_VIDEO_CONVERTER_MEMORY_TYPE         L"MemoryType"               // Values : AMF_MEMORY_DX11 or AMF_MEMORY_DX9 or AMF_MEMORY_UNKNOWN (get from input type)

#define AMF_VIDEO_CONVERTER_OUTPUT_SIZE         L"OutputSize"               // AMFSize  (default=0,0) width in pixels. default means no scaling
#define AMF_VIDEO_CONVERTER_OUTPUT_RECT         L"OutputRect"               // AMFRect  (default=0, 0, 0, 0) rectangle in pixels. default means no rect

#define AMF_VIDEO_CONVERTER_KEEP_ASPECT_RATIO   L"KeepAspectRatio"          // bool (default=false) Keep aspect ratio if scaling. 
#define AMF_VIDEO_CONVERTER_FILL                L"Fill"                     // bool (default=false) fill area out of ROI. 
#define AMF_VIDEO_CONVERTER_FILL_COLOR          L"FillColor"                // AMFColor 


#define AMF_VIDEO_CONVERTER_SCALE               L"ScaleType"

enum AMF_VIDEO_CONVERTER_SCALE_ENUM
{
    AMF_VIDEO_CONVERTER_SCALE_INVALID          = -1,
    AMF_VIDEO_CONVERTER_SCALE_BILINEAR          = 0,
    AMF_VIDEO_CONVERTER_SCALE_BICUBIC           = 1
};

#define AMF_VIDEO_CONVERTER_COLOR_PROFILE       L"ColorProfile"

enum AMF_VIDEO_CONVERTER_COLOR_PROFILE_ENUM
{
    AMF_VIDEO_CONVERTER_COLOR_PROFILE_UNKNOWN = -1,
    AMF_VIDEO_CONVERTER_COLOR_PROFILE_601 = 0,
    AMF_VIDEO_CONVERTER_COLOR_PROFILE_709 = 1,
    AMF_VIDEO_CONVERTER_COLOR_PROFILE_COUNT
};
#endif //#ifndef __AMFVideoConverter_h__
