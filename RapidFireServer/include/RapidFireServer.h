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

/*****************************************************************************
* RapidFireServer.h 
* * File Version 1.0.0 (CL 36199) Feb 12th 2015
* * File Version 1.0.1 (CL 36735) September 17th 2015
* * File Version 1.1.0.1          January 25th 2016
* * File Version 1.1.0.6          May 11th 2016
*****************************************************************************/

#ifndef RAPIDFIRE_H_
#define RAPIDFIRE_H_

#if defined WIN32 || defined _WIN32

#ifndef RAPIDFIRE_API
#define RAPIDFIRE_API __stdcall
#endif

#ifdef  _WIN64
typedef __int64             intptr_t;
#else
typedef __w64 int           intptr_t;
#endif

#else // if defined WIN32 || defined _WIN32

#define RAPIDFIRE_API

#include "stdint.h"

#endif // if defined WIN32 || defined _WIN32

typedef intptr_t            RFProperties;

typedef void*               RFEncodeSession;  
typedef void*               RFRenderTarget;

/**************************************************************************
* The RapidFire Server API status *
**************************************************************************/
typedef enum RFStatus
{
    RF_STATUS_OK                          = 0,
    RF_STATUS_FAIL                        = 1,
    RF_STATUS_MEMORY_FAIL                 = 2,
    RF_STATUS_RENDER_TARGET_FAIL          = 3,
    RF_STATUS_OPENGL_FAIL                 = 4,
    RF_STATUS_OPENCL_FAIL                 = 5,
    RF_STATUS_DOPP_FAIL                   = 6,
                                          
    RF_STATUS_AMF_FAIL                    = 8,
                                          
    RF_STATUS_QUEUE_FULL                  = 10,
    RF_STATUS_NO_ENCODED_FRAME            = 11,
                                          
    RF_STATUS_PARAM_ACCESS_DENIED         = 13,
    RF_STATUS_MOUSEGRAB_FAIL              = 14,
    RF_STATUS_MOUSEGRAB_NO_CHANGE         = 15,
    RF_STATUS_DOPP_NO_UPDATE              = 16,
                                          
    RF_STATUS_INVALID_SESSION             = 30,
    RF_STATUS_INVALID_CONTEXT             = 31,
    RF_STATUS_INVALID_TEXTURE             = 32,
    RF_STATUS_INVALID_DIMENSION           = 33,
    RF_STATUS_INVALID_INDEX               = 34,
    RF_STATUS_INVALID_FORMAT              = 35,
    RF_STATUS_INVALID_CONFIG              = 36,
    RF_STATUS_INVALID_ENCODER             = 37,
    RF_STATUS_INVALID_AUDIO_CODEC         = 38,
    RF_STATUS_INVALID_RENDER_TARGET       = 39,
    RF_STATUS_INVALID_CAPTURE_SOURCE      = 40,
    RF_STATUS_INVALID_DESKTOP_ID          = 41,
    RF_STATUS_INVALID_WINDOW              = 42,
    RF_STATUS_INVALID_OPENGL_CONTEXT      = 43,
    RF_STATUS_INVALID_D3D_DEVICE          = 44,
    RF_STATUS_INVALID_D3D_OBJECT          = 45,
    RF_STATUS_INVALID_OPENCL_ENV          = 46,
    RF_STATUS_INVALID_OPENCL_CONTEXT      = 47,
    RF_STATUS_INVALID_OPENCL_MEMOBJ       = 48,
    RF_STATUS_INVALID_SESSION_PROPERTIES  = 49,
    RF_STATUS_INVALID_ENCODER_PARAMETER   = 50,
} RFStatus;


/**************************************************************************
*  Session parameters *
**************************************************************************/
typedef enum RFSessionParams
{
    RF_GL_GRAPHICS_CTX                = 0x1001,
    RF_GL_DEVICE_CTX                  = 0x1002,
    RF_D3D9_DEVICE                    = 0x1003,
    RF_D3D9EX_DEVICE                  = 0x1004,
    RF_D3D11_DEVICE                   = 0x1005,
    RF_DESKTOP                        = 0x1007,
    RF_DESKTOP_DSP_ID                 = 0x1008,
    RF_DESKTOP_UPDATE_ON_CHANGE       = 0x1009,
    RF_DESKTOP_BLOCK_UNTIL_CHANGE     = 0x1010,
    RF_WINDOW                         = 0x1011,
    RF_FLIP_SOURCE                    = 0x1012,
    RF_ASYNC_SOURCE_COPY              = 0x1013,
    RF_ENCODER                        = 0x1014,
    RF_ENCODER_BLOCKING_READ          = 0x1015,
    RF_MOUSE_DATA                     = 0x1016,
    RF_DESKTOP_INTERNAL_DSP_ID        = 0x1017,
} RFSessionParams;


/**************************************************************************
* The video encoder parameters *
**************************************************************************/
typedef enum RFEncoderParams
{
    RF_ENCODER_FORMAT                       = 0x1111,
    RF_ENCODER_PROFILE                      = 0x1112,
    RF_ENCODER_LEVEL                        = 0x1113,
    RF_ENCODER_USAGE                        = 0x1114,
    RF_ENCODER_COMMON_LOW_LATENCY_INTERNAL  = 0x1115,

    RF_ENCODER_BITRATE                      = 0x1121,
    RF_ENCODER_PEAK_BITRATE                 = 0x1122,
    RF_ENCODER_RATE_CONTROL_METHOD          = 0x1123,
    RF_ENCODER_MIN_QP                       = 0x1124,
    RF_ENCODER_MAX_QP                       = 0x1125,
    RF_ENCODER_GOP_SIZE                     = 0x1126,
    RF_ENCODER_VBV_BUFFER_SIZE              = 0x1127,
    RF_ENCODER_VBV_BUFFER_FULLNESS          = 0x1128,
    RF_ENCODER_ENFORCE_HRD                  = 0x1129,
    RF_ENCODER_FRAME_RATE                   = 0x1130,
    RF_ENCODER_FRAME_RATE_DEN               = 0x1131,

    RF_ENCODER_IDR_PERIOD                   = 0x1141,
    RF_ENCODER_INTRA_REFRESH_NUM_MB         = 0x1142,
    RF_ENCODER_DEBLOCKING_FILTER            = 0x1143,
    RF_ENCODER_NUM_SLICES_PER_FRAME         = 0x1144,
    RF_ENCODER_QUALITY_PRESET               = 0x1145,

    RF_ENCODER_HALF_PIXEL                   = 0x1152,
    RF_ENCODER_QUARTER_PIXEL				= 0x1153,

    RF_DIFF_ENCODER_BLOCK_S              	= 0x1154,
    RF_DIFF_ENCODER_BLOCK_T                	= 0x1155,
    RF_DIFF_ENCODER_LOCK_BUFFER             = 0x1156,

    // Pre Submit parameters
    RF_ENCODER_FORCE_INTRA_REFRESH          = 0x1061,   
    RF_ENCODER_FORCE_I_FRAME                = 0x1062,
    RF_ENCODER_FORCE_P_FRAME                = 0x1063,
    RF_ENCODER_INSERT_SPS                   = 0x1064,
    RF_ENCODER_INSERT_PPS                   = 0x1065,
    RF_ENCODER_INSERT_AUD                   = 0x1066,

    // Read only parameter
    RF_ENCODER_WIDTH                        = 0x1081,
    RF_ENCODER_HEIGHT                       = 0x1082,
    RF_ENCODER_OUTPUT_WIDTH                 = 0x1083,
    RF_ENCODER_OUTPUT_HEIGHT                = 0x1084,
} RFEncoderParams;


/**
*******************************************************************************
* @typedef RFBitmapBuffer
* @brief This structure stores the bitmap data.
*
* @uiWidth:  Width of the bitmap.
* @uiHeight: Height of the bitmap.
* @uiPitch:  Pitch of the bitmap.
* @uiBitsPerPixel: Bits per pixel of the bitmap.
* @pPixels:  Data of the bitmap.
*
*******************************************************************************
*/
typedef struct
{
    unsigned int	uiWidth;
    unsigned int	uiHeight;
    unsigned int	uiPitch;
    unsigned int	uiBitsPerPixel;
    void*			pPixels;
} RFBitmapBuffer;

/**
*******************************************************************************
* @typedef RFMouseData
* @brief This structure is used to return the cursor shape data.
*
* @iVisible: 1 if the cursor is visible and 0 otherwise.
* @uiXHot:   X position of hot spot.
* @uiYHot:   Y postiion of hot spot.
* @mask:	 The cursor bitmask bitmap. If the cursor is monochrome, this bitmask
*			 is formatted so that the upper half is the cursor AND bitmask and the lower
*			 half is the XOR bitmask.
*			 If the cursor is colored, this mask defines the AND bitmask of the cursor.
* @color:	 The cursor color bitmap containing the color data.
*			 This member can be optional. If the cursor is monochrome color.pPixels is NULL.
*			 For pixels with false in the mask bitmap the color is directly blended with the destination pixel.
*			 For pixels with true in the mask bitmap the color is XORed with the color of the destination pixel.
*
*******************************************************************************
*/
typedef struct
{
    int             iVisible;
    unsigned int    uiXHot;
    unsigned int    uiYHot;
    RFBitmapBuffer	mask;
    RFBitmapBuffer	color;
} RFMouseData;

/**
*******************************************************************************
* @enum RFFormat
* @brief This is the data format of the input for the encoder.
*
* @RF_RGBA8:
* @RF_ARGB8:
* @RF_BGRA8: 32-bit RGB with Alpha, each pixel is represented by one byte each
*            for the red, green, blue, and alpha channels.
* @RF_NV12:  8-bit Y plane followed by an interleaved U/V plane with 2x2 subsampling.
*
*******************************************************************************
*/
typedef enum RFFormat
{ 
    RF_FORMAT_UNKNOWN = -1,
    RF_RGBA8          =  0,
    RF_ARGB8          =  1,
    RF_BGRA8          =  2,
    RF_NV12           =  3
} RFFormat;

/**
*******************************************************************************
* @enum RFEncodePreset
* @brief This is the preset of the encoder: trade off compression efficiency
*        against encoding speed.
*
* @RF_PRESET_NONE:     No preset is used. 
* @RF_PRESET_FAST:     Fast encoding.
* @RF_PRESET_BALANCED: Balanced between quality and speed.
* @RF_PRESET_QUALITY:  High video quality.
*
*******************************************************************************
*/
typedef enum RFEncodePreset
{
    RF_PRESET_NONE     = -1,
    RF_PRESET_FAST     =  0,
    RF_PRESET_BALANCED =  1,
    RF_PRESET_QUALITY  =  2
} RFEncodePreset;

/**
*******************************************************************************
* @enum RFEncoderID
* @brief This is the type of the encoder.
*
* @RF_AMF:        AMD Media Foundation library encoder (HW).
* @RF_IDENTITY:   Identity encoder which returns the captured texture.
* @RF_DIFFERENCE: Difference encoder returns a difference map with 1 where the source image has changed and 0 otherwise.
*
*******************************************************************************
*/
typedef enum RFEncoderID
{
    RF_ENCODER_UNKNOWN = -1,
    RF_AMF             =  0,
    RF_IDENTITY        =  1,
    RF_DIFFERENCE      =  2
} RFEncoderID;

/**
*******************************************************************************
* @enum RFRenderTargetState
* @brief This is the state of a render target.
*
* @RF_STATE_INVALID: The render target is not known or not registered yet.
* @RF_STATE_FREE:    The render target was successfully registered and is currently
*                    not used by the API. It can be used by the application.
* @RF_STATE_BLOCKED: The render target was submitted for encoding and is currently
*                    in use by the API. It should not be used by the application.
*
*******************************************************************************
*/
typedef enum RFRenderTargetState
{
    RF_STATE_INVALID = 0,
    RF_STATE_FREE    = 1,
    RF_STATE_BLOCKED = 2
} RFRenderTargetState;

/**
*******************************************************************************
* @enum RFNotification
* @brief RF uses events to get signaled on mouse shape changes or on desktop
*        changes. Functions that refer to those events need to specify them
*        using this enum.
*
* @RFDesktopNotification: Desktop notification event. The event is only present
*                         if the session was created with the flag RF_DESKTOP_UPDATE_ON_CHANGE
*                         or RF_DESKTOP_BLOCK_UNTIL_CHANGE  
*
* @RFMouseShapeNotification: Mouse shape notification event.The event is only 
*                            present if the session was created using the flag
*                            RF_MOUSE_DATA.
*
*******************************************************************************
*/
typedef enum RFNotification
{
    RFDesktopNotification    = 1,
    RFMouseShapeNotification = 2
} RFNotification;

#ifdef __cplusplus
extern "C" {
#endif

/**
*******************************************************************************
* @fn rfCreateEncodeSession
* @brief This function creates an encode session.
*        It also creates OpenCL environment based on input properties.
*
* @param[out] session:   The created encoding session.
* @param[in] properties: Specifies a list of session property names and their
*                        corresponding values.
*                        The list is terminated with 0.
*
* @return RFEncodeSession: RF_STATUS_OK if successful; otherwise an error code and session is set to NULL.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfCreateEncodeSession(RFEncodeSession* session, RFProperties* properties);

/**
*******************************************************************************
* @fn rfDeleteEncodeSession
* @brief This function deletes an encoding session and frees all associated resources.
*
* @param[in] session: The encoding session to be deleted.
*
* @return void: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfDeleteEncodeSession(RFEncodeSession* session);

/**
*******************************************************************************
* @fn rfCreateEncoder
* @brief This function creates the encoder and the resources like intermediate buffers.
*        It initializes all parameters based on the selected preset.
*
* @param[in] session:  The encoding session.
* @param[in] uiWidth:  The width of the encoded stream.
* @param[in] uiHeight: The height of the encoded stream.
* @param[in] preset:   The preset for the encoder.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfCreateEncoder(RFEncodeSession session, unsigned int uiWidth, unsigned int uiHeight, const RFEncodePreset preset);

/**
*******************************************************************************
* @fn rfCreateEncoder2
* @brief This function creates the encoder by passing the static and dynamic properties:
*        Static:  Profile, Level, Bitrate, FPS ...
*        Dynamic: Rate Control Method, Rate Control Options, I Frame Quantization,
*                 P Frame Quantization, B Frame Quantization, GOP Size,
*                 VBV Buffer Size, IDR Period, Number Of Slices Per Frame,
*                 Force Intra Refresh ...
*
* @param[in] session:    The encoding session.
* @param[in] uiWidth:    The width of the encoded stream.
* @param[in] uiHeight:   The height of the encoded stream.
* @param[in] properties: Specifies a list of encoder property names and their
*                        corresponding values.
*                        The list is terminated with 0.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfCreateEncoder2(RFEncodeSession session, unsigned int uiWidth, unsigned int uiHeight, const RFProperties* properties);

/**
*******************************************************************************
* @fn rfRegisterRenderTarget
* @brief This function registers a render target that is created by the user
*        and returns the index used for this render target in idx.
*        The render target must have the same dimesnions as the encoder.
*
* @param[in] session:      The encoding session.
* @param[in] renderTarget: The handle of the render target.
* @param[in] uiRTWidth:    The width of the render target.
* @param[in] uiRTHeight:   The height of the render target.
* @param[out] idx:         The index used for this render target.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfRegisterRenderTarget(RFEncodeSession session, RFRenderTarget renderTarget, unsigned int uiRTWidth, unsigned int uiRTHeight, unsigned int* idx);

/**
*******************************************************************************
* @fn rfRemoveRenderTarget
* @brief This function removes a registered render target with the index idx
*        which is no longer used in the session.
*
* @param[in] session: The encoding session.
* @param[in] idx:     The index of the render target which is no longer used in session s.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfRemoveRenderTarget(RFEncodeSession session, unsigned int idx);

/**
*******************************************************************************
* @fn rfGetRenderTargetState
* @brief This function gets the state of render target whose index is idx.
*
* @param[in] session: The encoding session.
* @param[out] state:  RF_STATE_INVALID if the render target is not known;
*                     RF_STATE_FREE if it is not used by the API;
*                     RF_STATE_BLOCKED if it is currently in use by the API.
* @param[in] idx:     The index of the render target.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfGetRenderTargetState(RFEncodeSession session, RFRenderTargetState* state, unsigned int idx);

/**
*******************************************************************************
* @fn rfResizeSession
* @brief This function resizes the session and the encoder. Rendertargets that
* are registered by the application won't be resized automatically.
*
* @param[in] session: The encoding session.
* @param[in] uiWidth: New width
* @param[in] uiWidth: New height
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfResizeSession(RFEncodeSession session, unsigned int uiWidth, unsigned int uiHeight);

/**
*******************************************************************************
* @fn rfEncodeFrame
* @brief This function is called once the application has finished rendering
*        into the render target with id idx. This render target will then be encoded.
*
* @param[in] session: The encoding session.
* @param[in] idx:     The index of the render target which will be encoded.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfEncodeFrame(RFEncodeSession session, unsigned int idx);

/**
*******************************************************************************
* @fn rfGetEncodedFrame
* @brief This function returns the encoded frame in pBitStream and
*        the size (in bytes) of the encoded frame in uiSize.
*
* @param[in] session:     The encoding session.
* @param[out] uiSize:     The size (in bytes) of the bit stream.
* @param[out] pBitStream: Pointer to the bit stream of the encoded frame.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfGetEncodedFrame(RFEncodeSession session, unsigned int* uiSize, void** pBitStream);

/**
*******************************************************************************
* @fn  rfGetSourceFrame
* @brief  The function return the image that was used as input for the encoder.
*         If a color space conversion is required for the encoder, the returned 
*         image contains the result of the color space conversion.
*         The function needs to be called prior to rfGetEncodedFrame to guarantee
*         that the returned image is the source of the encoded image returned by
*         rfGetEncodedFrame.
*
* @param[in] session:     The encoding session.
* @param[out] uiSize:     The size (in bytes) of the source image.
* @param[out] pBitStream: Pointer to the source image.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfGetSourceFrame(RFEncodeSession session, unsigned int* uiSize, void** pBitStream);

/**
*******************************************************************************
* @fn rfSetEncodeParameter
* @brief This function changes encoding parameters that does not require
*        a re-creation of the encoder, e.g. adapting the encoding quality to the
*        available network bandwidth. The available parameters depend on the selected encoder.
*        This function should be called after rfCreateEncoder/rfCreateEncoder2,
*        otherwise it does not have any effect.
*
* @param[in] session:  The encoding session.
* @param[in] property: The encoding property to change, e.g. RF_ENCODER_BITRATE
* @param[in] value:    The new value of the encoding property.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfSetEncodeParameter(RFEncodeSession session, const int property, RFProperties value);

/**
*******************************************************************************
* @fn rfGetEncodeParameter
* @brief This function reads the value of parameter param
*
* @param[in] session:  The encoding session.
* @param[in] property: The encoding parameter to read, e.g. RF_ENCODER_BITRATE
* @param[out] value:   The currently used value of the encoding parameter.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfGetEncodeParameter(RFEncodeSession session, const int property, RFProperties* value);

/**
*******************************************************************************
* @fn rfGetMouseData
* @brief This function returns mouse shape data. To use it the session needs to 
* be created with the RF_MOUSE_DATA set to true.
*
* @param[in] session:             The encoding session.
* @param[in] iWaitForShapeChange: If set to 1 the call blocks until the mouse shape changed
* @param[out] mouseData:          Mouse shape data
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfGetMouseData(RFEncodeSession session, int iWaitForShapeChange, RFMouseData* mouseData);

/**
*******************************************************************************
* @fn rfReleaseEvent
* @brief This function signals a notification event.
*
* @param[in] session:        The encoding session.
* @param[in] rfNotification: Specifies which event to release.
*
* @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
*******************************************************************************
*/
RFStatus RAPIDFIRE_API rfReleaseEvent(RFEncodeSession session, RFNotification const rfNotification);

#ifdef __cplusplus
};
#endif

#endif // ifndef RAPIDFIRE_H_