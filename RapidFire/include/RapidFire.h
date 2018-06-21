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
* RapidFire.h
* * File Version 1.0.0 (CL 36199) Feb 12th 2015
* * File Version 1.0.1 (CL 36735) September 17th 2015
* * File Version 1.1.0.1          January 25th 2016
* * File Version 1.1.0.19         September 26th 2016
* * File Version 1.2.0.0          November 3rd 2017
* * File Version 1.2.1.0          April 20th 2018
* * File Version 1.2.1.62         June 21st 2018
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
* The RapidFire API status *
**************************************************************************/
typedef enum RFStatus
{
    RF_STATUS_OK                          = 0,
    RF_STATUS_FAIL                        = -1,
    RF_STATUS_MEMORY_FAIL                 = -2,
    RF_STATUS_RENDER_TARGET_FAIL          = -3,
    RF_STATUS_OPENGL_FAIL                 = -4,
    RF_STATUS_OPENCL_FAIL                 = -5,
    RF_STATUS_DOPP_FAIL                   = -6,
    RF_STATUS_AMF_FAIL                    = -8,

    RF_STATUS_QUEUE_FULL                  = -10,
    RF_STATUS_NO_ENCODED_FRAME            = -11,

    RF_STATUS_PARAM_ACCESS_DENIED         = -13,
    RF_STATUS_MOUSEGRAB_NO_CHANGE         = -15,
    RF_STATUS_DOPP_NO_UPDATE              = -16,

    RF_STATUS_INVALID_SESSION             = -30,
    RF_STATUS_INVALID_CONTEXT             = -31,
    RF_STATUS_INVALID_TEXTURE             = -32,
    RF_STATUS_INVALID_DIMENSION           = -33,
    RF_STATUS_INVALID_INDEX               = -34,
    RF_STATUS_INVALID_FORMAT              = -35,
    RF_STATUS_INVALID_CONFIG              = -36,
    RF_STATUS_INVALID_ENCODER             = -37,
    RF_STATUS_INVALID_PARAMETER           = -38,
    RF_STATUS_INVALID_RENDER_TARGET       = -39,
    RF_STATUS_INVALID_DESKTOP_ID          = -41,
    RF_STATUS_INVALID_OPENGL_CONTEXT      = -43,
    RF_STATUS_INVALID_D3D_DEVICE          = -44,
    RF_STATUS_INVALID_OPENCL_ENV          = -46,
    RF_STATUS_INVALID_OPENCL_CONTEXT      = -47,
    RF_STATUS_INVALID_OPENCL_MEMOBJ       = -48,
    RF_STATUS_INVALID_SESSION_PROPERTIES  = -49,
    RF_STATUS_INVALID_ENCODER_PARAMETER   = -50,
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
    RF_FLIP_SOURCE                    = 0x1012,
    RF_ASYNC_SOURCE_COPY              = 0x1013,
    RF_ENCODER                        = 0x1014,
    RF_ENCODER_BLOCKING_READ          = 0x1015,
    RF_MOUSE_DATA                     = 0x1016,
    RF_DESKTOP_INTERNAL_DSP_ID        = 0x1017,
} RFSessionParams;


/**************************************************************************
* Encoder parameters *
**************************************************************************/
typedef enum RFEncoderParams
{
    RF_ENCODER_CODEC                        = 0x1101,
    RF_ENCODER_FORMAT                       = 0x1111,

    // AVC encoding parameters
    RF_ENCODER_PROFILE                      = 0x1112,
    RF_ENCODER_LEVEL                        = 0x1113,
    RF_ENCODER_USAGE                        = 0x1114,
    RF_ENCODER_COMMON_LOW_LATENCY_INTERNAL  = 0x1115,

    RF_ENCODER_BITRATE                      = 0x1121,
    RF_ENCODER_PEAK_BITRATE                 = 0x1122,
    RF_ENCODER_RATE_CONTROL_METHOD          = 0x1123,
    RF_ENCODER_MIN_QP                       = 0x1124,
    RF_ENCODER_MAX_QP                       = 0x1125,
    RF_ENCODER_VBV_BUFFER_SIZE              = 0x1127,
    RF_ENCODER_VBV_BUFFER_FULLNESS          = 0x1128,
    RF_ENCODER_ENFORCE_HRD                  = 0x1129,
    RF_ENCODER_ENABLE_VBAQ                  = 0x1132,
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

    // AVC Pre Submit parameters
    RF_ENCODER_FORCE_INTRA_REFRESH          = 0x1061,
    RF_ENCODER_FORCE_I_FRAME                = 0x1062,
    RF_ENCODER_FORCE_P_FRAME                = 0x1063,
    RF_ENCODER_INSERT_SPS                   = 0x1064,
    RF_ENCODER_INSERT_PPS                   = 0x1065,
    RF_ENCODER_INSERT_AUD                   = 0x1066,

    // HEVC encoding parameters
    RF_ENCODER_HEVC_USAGE                           = 0x1300,
    RF_ENCODER_HEVC_PROFILE                         = 0x1301,
    RF_ENCODER_HEVC_LEVEL                           = 0x1302,
    RF_ENCODER_HEVC_TIER                            = 0x1303,

    RF_ENCODER_HEVC_RATE_CONTROL_METHOD             = 0x1305,
    RF_ENCODER_HEVC_FRAMERATE                       = 0x1306,
    RF_ENCODER_HEVC_FRAMERATE_DEN                   = 0x1307,
    RF_ENCODER_HEVC_VBV_BUFFER_SIZE                 = 0x1308,
    RF_ENCODER_HEVC_INITIAL_VBV_BUFFER_FULLNESS     = 0x1309,
    RF_ENCODER_HEVC_RATE_CONTROL_PREANALYSIS_ENABLE = 0x1310,
    RF_ENCODER_HEVC_ENABLE_VBAQ                     = 0x1311,

    RF_ENCODER_HEVC_TARGET_BITRATE                  = 0x1312,
    RF_ENCODER_HEVC_PEAK_BITRATE                    = 0x1313,
    RF_ENCODER_HEVC_MIN_QP_I                        = 0x1314,
    RF_ENCODER_HEVC_MAX_QP_I                        = 0x1315,
    RF_ENCODER_HEVC_MIN_QP_P                        = 0x1316,
    RF_ENCODER_HEVC_MAX_QP_P                        = 0x1317,
    RF_ENCODER_HEVC_QP_I                            = 0x1318,
    RF_ENCODER_HEVC_QP_P                            = 0x1319,
    RF_ENCODER_HEVC_ENFORCE_HRD                     = 0x1320,
    RF_ENCODER_HEVC_MAX_AU_SIZE                     = 0x1321,
    RF_ENCODER_HEVC_FILLER_DATA_ENABLE              = 0x1322,
    RF_ENCODER_HEVC_RATE_CONTROL_SKIP_FRAME_ENABLE  = 0x1323,

    RF_ENCODER_HEVC_HEADER_INSERTION_MODE           = 0x1324,
    RF_ENCODER_HEVC_GOP_SIZE                        = 0x1325,
    RF_ENCODER_HEVC_NUM_GOPS_PER_IDR                = 0x1326,
    RF_ENCODER_HEVC_DE_BLOCKING_FILTER_DISABLE      = 0x1327,
    RF_ENCODER_HEVC_SLICES_PER_FRAME                = 0x1328,

    RF_ENCODER_HEVC_QUALITY_PRESET                  = 0x1329,

    RF_ENCODER_HEVC_MOTION_HALF_PIXEL               = 0x1330,
    RF_ENCODER_HEVC_MOTION_QUARTERPIXEL             = 0x1331,

    // HEVC Pre Submit parameters
    RF_ENCODER_HEVC_FORCE_INTRA_REFRESH             = 0x1201,
    RF_ENCODER_HEVC_FORCE_I_FRAME                   = 0x1202,
    RF_ENCODER_HEVC_FORCE_P_FRAME                   = 0x1203,
    RF_ENCODER_HEVC_INSERT_HEADER                   = 0x1204,
    RF_ENCODER_HEVC_INSERT_AUD                      = 0x1205,

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
* @uiXHot:   X position of cursor hot spot.
* @uiYHot:   Y postiion of cursor hot spot.
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
* @typedef RFMouseData2
* @brief This structure is used to return cursor shape data.
*        The pPixels buffer contains the cursor shape data in a format
*        that is compatible with the DXGKARG_SETPOINTERSHAPE struct
*        used in the kernel mode driver function DxgkddiSetpointershape.
*
* @iVisible: 1 if the cursor is visible and 0 otherwise.
* @uiXHot:   X position of cursor hot spot.
* @uiYHot:   Y postiion of cursor hot spot.
* @uiFlags:  1 = Monochrome, 2 = Color, 4 = MaskedColor
* @pShape:   RFBitmapBuffer containing the cursor shape data.
*
*******************************************************************************
*/
typedef struct
{
    int             iVisible;
    unsigned int    uiXHot;
    unsigned int    uiYHot;
    unsigned int    uiFlags;
    RFBitmapBuffer  pShape;
} RFMouseData2;

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
* @enum RFVideoEncoder
* @brief This is the codec of the video encoder.
*
* @RF_VIDEO_CODEC_AVC:         The AVC / H.264 codec is used.
* @RF_VIDEO_CODEC_HEVC:        The HEVC / H.265 codec is used.
*
*******************************************************************************
*/
typedef enum RFVideoCodec
{
    RF_VIDEO_CODEC_NONE    = -1,
    RF_VIDEO_CODEC_AVC     =  0,
    RF_VIDEO_CODEC_HEVC    =  1
} RFVideoCodec;

/**
*******************************************************************************
* @enum RFEncodePreset
* @brief This is the preset of the video encoder: trade off compression efficiency
*        against encoding speed.
*
* @RF_PRESET_NONE:          No preset is used.
* @RF_PRESET_FAST:          Fast encoding with AVC.
* @RF_PRESET_BALANCED:      Balanced encoding between quality and speed with AVC.
* @RF_PRESET_QUALITY:       High video encoding quality with AVC.
* @RF_PRESET_HEVC_FAST:     Fast encoding with HEVC.
* @RF_PRESET_HEVC_BALANCED: Balanced encoding between quality and speed with HEVC.
* @RF_PRESET_HEVC_QUALITY:  High video encoding quality with HEVC.
*
*******************************************************************************
*/
typedef enum RFEncodePreset
{
    RF_PRESET_NONE          = -1,
    RF_PRESET_FAST          =  0,
    RF_PRESET_BALANCED      =  1,
    RF_PRESET_QUALITY       =  2,
    RF_PRESET_HEVC_FAST     =  3,
    RF_PRESET_HEVC_BALANCED =  4,
    RF_PRESET_HEVC_QUALITY  =  5
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
    RF_STATE_INVALID = -1,
    RF_STATE_FREE    =  0,
    RF_STATE_BLOCKED =  1
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
    * @brief This function creates an encoding session.
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
    RFStatus RAPIDFIRE_API rfCreateEncodeSession(RFEncodeSession* session, const RFProperties* properties);

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
    RFStatus RAPIDFIRE_API rfCreateEncoder(RFEncodeSession session, const unsigned int uiWidth, const unsigned int uiHeight, const RFEncodePreset preset);

    /**
    *******************************************************************************
    * @fn rfCreateEncoder2
    * @brief This function creates the encoder by passing the static and dynamic properties:
    *        Static:  Profile, Level, Bitrate, FPS ...
    *        Dynamic: Rate Control Method, Rate Control Options, I Frame Quantization,
    *                 P Frame Quantization, B Frame Quantization
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
    RFStatus RAPIDFIRE_API rfCreateEncoder2(RFEncodeSession session, const unsigned int uiWidth, const unsigned int uiHeight, const RFProperties* properties);

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
    RFStatus RAPIDFIRE_API rfRegisterRenderTarget(RFEncodeSession session, const RFRenderTarget renderTarget, const unsigned int uiRTWidth, const unsigned int uiRTHeight, unsigned int* idx);

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
    RFStatus RAPIDFIRE_API rfRemoveRenderTarget(RFEncodeSession session, const unsigned int idx);

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
    RFStatus RAPIDFIRE_API rfGetRenderTargetState(RFEncodeSession session, RFRenderTargetState* state, const unsigned int idx);

    /**
    *******************************************************************************
    * @fn rfResizeSession
    * @brief Resizes the session and the encoder if the encoder supports resizing.
    * Before calling rfResizeSession the encoding queue must be empty.
    * Otherwise submitting additional frames to the encoding queue might fail.
    * Render targets that are registered by the application will be removed
    * and have to be registered again with the new size.
    *
    * @param[in] session: The encoding session.
    * @param[in] uiWidth: New width
    * @param[in] uiWidth: New height
    *
    * @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
    *******************************************************************************
    */
    RFStatus RAPIDFIRE_API rfResizeSession(RFEncodeSession session, const unsigned int uiWidth, const unsigned int uiHeight);

    /**
    *******************************************************************************
    * @fn rfEncodeFrame
    * @brief This function is called once the application has finished rendering
    *        into the render target with id idx. This render target will then be encoded.
    *
    * @param[in] session: The encoding session.
    * @param[in] idx:     The index of the render target which will be encoded.
    *                     (ignored for encoding sessions with a desktop set as source)
    *
    * @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
    *******************************************************************************
    */
    RFStatus RAPIDFIRE_API rfEncodeFrame(RFEncodeSession session, const unsigned int idx);

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
    RFStatus RAPIDFIRE_API rfSetEncodeParameter(RFEncodeSession session, const int property, const RFProperties value);

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
    * @brief This function returns the mouse shape data. To use it the session needs to
    * be created with the RF_MOUSE_DATA set to true.
    *
    * @param[in] session:             The encoding session.
    * @param[in] iWaitForShapeChange: If set to 1 the call blocks until the mouse shape changed
    * @param[out] mouseData:          Mouse shape data
    *
    * @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
    *******************************************************************************
    */
    RFStatus RAPIDFIRE_API rfGetMouseData(RFEncodeSession session, const int iWaitForShapeChange, RFMouseData* mouseData);

    /**
    *******************************************************************************
    * @fn rfGetMouseData2
    * @brief This function returns the mouse shape data. To use it the session needs to
    * be created with the RF_MOUSE_DATA set to true.
    *
    * @param[in] session:             The encoding session.
    * @param[in] iWaitForShapeChange: If set to 1 the call blocks until the mouse shape changed
    * @param[out] mouseData:          Mouse shape data
    *
    * @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
    *******************************************************************************
    */
    RFStatus RAPIDFIRE_API rfGetMouseData2(RFEncodeSession session, const int iWaitForShapeChange, RFMouseData2* mouseData);

    /**
    *******************************************************************************
    * @fn rfReleaseEvent
    * @brief This function signals a notification event.
    * This function can be used to unblock a thread that
    * is waiting for an event to be signaled.
    *
    * @param[in] session:        The encoding session.
    * @param[in] rfNotification: Specifies which event to release.
    *
    * @return RFStatus: RF_STATUS_OK if successful; otherwise an error code.
    *******************************************************************************
    */
    RFStatus RAPIDFIRE_API rfReleaseEvent(RFEncodeSession session, const RFNotification rfNotification);

#ifdef __cplusplus
};
#endif

#endif // ifndef RAPIDFIRE_H_