/*
 ***************************************************************************************************
 *
 * Copyright (c) 2013 Advanced Micro Devices, Inc. (unpublished)
 *
 *  All rights reserved.  This notice is intended as a precaution against inadvertent publication and
 *  does not imply publication or any waiver of confidentiality.  The year included in the foregoing
 *  notice is the year of creation of the work.
 *
 ***************************************************************************************************
 */
/**
 ***************************************************************************************************
 * @file  VideoProcessor.h
 * @brief AMFVideoProcessor interface declaration
 ***************************************************************************************************
 */
#ifndef __AMFVideoProcessor_h__
#define __AMFVideoProcessor_h__
#pragma once

#include "Component.h"

#define AMFVideoProcessor L"AMFVideoProcessor"

enum AMF_VIDEO_PROCESSOR_SCALE_ENUM
{
    AMF_VIDEO_PROCESSOR_SCALE_BILINEAR = 0,
    AMF_VIDEO_PROCESSOR_SCALE_BICUBIC,
};

enum AMF_VIDEO_PROCESSOR_DYNAMIC_RANGE_ENUM
{
    AMF_VIDEO_PROCESSOR_DYNAMIC_RANGE_FULL = 0,                                                      // Value for AMF_VIDEO_PROCESSOR_DYNAMIC_RANGE (0-255)
    AMF_VIDEO_PROCESSOR_DYNAMIC_RANGE_LIMITED,                                                       // Value for AMF_VIDEO_PROCESSOR_DYNAMIC_RANGE (16-235)
};

enum AMF_VIDEO_PROCESSOR_DEINTERLACING_MODE_ENUM
{
    AMF_VIDEO_PROCESSOR_DEINTERLACING_AUTOMATIC = 0,
    AMF_VIDEO_PROCESSOR_DEINTERLACING_WEAVE,
    AMF_VIDEO_PROCESSOR_DEINTERLACING_BOB,
    AMF_VIDEO_PROCESSOR_DEINTERLACING_ADAPTIVE,
    AMF_VIDEO_PROCESSOR_DEINTERLACING_MOTION_ADAPTIVE,
    AMF_VIDEO_PROCESSOR_DEINTERLACING_VECTOR_ADAPTIVE,
};

//output format
#define AMF_VIDEO_PROCESSOR_OUTPUT_FORMAT                    L"OutputFormat"                        // Values : AMF_SURFACE_NV12 or AMF_SURFACE_BGRA
#define AMF_VIDEO_PROCESSOR_MEMORY_TYPE                      L"MemoryType"                          // Values : AMF_MEMORY_DX11 or AMF_MEMORY_DX9 or AMF_MEMORY_UNKNOWN (get from input type)

//blt settings
#define AMF_VIDEO_PROCESSOR_SCALE_TYPE                       L"ScaleType"                           // INT (default=AMF_VIDEO_PROCESSOR_SCALE_BILINEAR)  values - see below Scale effect type
#define AMF_VIDEO_PROCESSOR_SCALE_WIDTH                      L"OutputWidth"                         // INT (default=no scaling)  width in pixels. 0 means default
#define AMF_VIDEO_PROCESSOR_SCALE_HEIGHT                     L"OutputHeight"                        // INT (default=no scaling)  Height in pixels. 0 means default

#define AMF_VIDEO_PROCESSOR_BRIGHTNESS                       L"Brightness"                          // DOUBLE (default=0) min=-100 max=100 Brightness effect
#define AMF_VIDEO_PROCESSOR_CONTRAST                         L"Contrast"                            // DOUBLE (default=1) min=0 max=2 Contrast effect
#define AMF_VIDEO_PROCESSOR_HUE                              L"Hue"                                 // DOUBLE (default=0) min=-30 max=30 Hue effect
#define AMF_VIDEO_PROCESSOR_SATURATION                       L"Saturation"                          // DOUBLE (default=1) min=0 max=2 Saturation effect
#define AMF_VIDEO_PROCESSOR_GAMMA                            L"Gamma"                               // DOUBLE (default=1.0) min=0.5 max=2.5 Gamma corection strength

#define AMF_VIDEO_PROCESSOR_DYNAMIC_RANGE                    L"DynamicRange"                        // INT (default=AMF_VIDEO_PROCESSOR_DYNAMIC_RANGE_NONE)  values - see below Dynamic range effect type

//deinterlace
#define AMF_VIDEO_PROCESSOR_DEINTERLACING_MODE               L"DeinterlacingMode"                   // INT (default=AMF_VIDEO_PROCESSOR_DEINTERLACING_AUTOMATIC)  values - see below De-interlacing effect type
#define AMF_VIDEO_PROCESSOR_PULLDOWN_DETECTION_ENABLE        L"PulldownDetectionEnable"             // BOOL (default=TRUE)  enables=TRUE / disables=FALSE De-interlacing Pulldown detection

//denoise
#define AMF_VIDEO_PROCESSOR_TEMPORAL_DENOISE_ENABLE          L"TemporalDenoiseEnable"               // BOOL (default=FALSE)  enables=TRUE / disables=FALSE De-noise effect
#define AMF_VIDEO_PROCESSOR_TEMPORAL_DENOISE_STRENGTH        L"TemporalDenoiseStrength"             // INT (default=50) min=1 max=100 De-noise strength

//compression artifacts
#define AMF_VIDEO_PROCESSOR_DEBLOCKING_ENABLE                L"DeblockingEnable"                    // BOOL (default=FALSE)  enables=TRUE / disables=FALSE de-blocking effect
#define AMF_VIDEO_PROCESSOR_DEBLOCKING_STRENGTH              L"DeblockingStrength"                  // INT (default=50) min=0 max=100 de-blocking effect strength

#define AMF_VIDEO_PROCESSOR_MOSQUITO_NOISE_REDUCTION_ENABLE   L"MosquitoNoiseReductionEnable"       // BOOL (default=FALSE)  enables=TRUE / disables=FALSE Mosquito Noise removal effect
#define AMF_VIDEO_PROCESSOR_MOSQUITO_NOISE_REDUCTION_STRENGTH L"MosquitoNoiseReductionStrength"     // INT (default=68) min=0 max=100 Mosquito Noise removal strength

#define AMF_VIDEO_PROCESSOR_FALSE_CONTOURS_REMOVAL_ENABLE    L"FalseContoursRemovalEnable"          // BOOL (default=FALSE)  enables=TRUE / disables=FALSE False contour reduction effect
#define AMF_VIDEO_PROCESSOR_FALSE_CONTOURS_REMOVAL_STRENGTH  L"FalseContoursRemovalStrength"        // INT (default=50) min=0 max=100 FalseContours correction strength

//color management
#define AMF_VIDEO_PROCESSOR_COLOR_VIBRANCE_ENABLE            L"ColorVibranceEnable"                 // BOOL (default=FALSE)  enables=TRUE / disables=FALSE Color vibrance effect
#define AMF_VIDEO_PROCESSOR_COLOR_VIBRANCE_STRENGTH          L"ColorVibranceStrength"               // INT (default=50) min=0 max=100 Color vibrance strength

#define AMF_VIDEO_PROCESSOR_SKINTONE_CORRECTION_ENABLE       L"SkinToneCorrectionEnable"            // BOOL (default=FALSE)  enables=TRUE / disables=FALSE SkinTone correction effect
#define AMF_VIDEO_PROCESSOR_SKINTONE_CORRECTION_STRENGTH     L"SkinToneCorrectionStrength"          // INT (default=50) min=0 max=100 SkinTone correction strength

#define AMF_VIDEO_PROCESSOR_BRIGHTER_WHITES_ENABLE           L"BrighterWhitesEnable"                // BOOL (default=FALSE)  enables=TRUE / disables=FALSE Brighter Whites effect

//edge enchancement
#define AMF_VIDEO_PROCESSOR_EDGE_ENHANCEMENT_ENABLE          L"EdgeEnhancementEnable"               // BOOL (default=FALSE)  enables=TRUE / disables=FALSE Edge enhancement effect
#define AMF_VIDEO_PROCESSOR_EDGE_ENHANCEMENT_STRENGTH        L"EdgeEnhancementStrength"             // INT (default=50) min=1 max=100 Edge enhancement strength

//dynamic contrast
#define AMF_VIDEO_PROCESSOR_DYNAMIC_CONTRAST_ENABLE          L"DynamicContrastEnable"               // BOOL (default=FALSE)  enables=TRUE / disables=FALSE Dynamic contrast

//demo mode
#define AMF_VIDEO_PROCESSOR_DEMOMODE_ENABLE                  L"DemoModeEnable"                      // BOOL (default=FALSE)  enables=TRUE / disables=FALSE

#define AMF_VIDEO_PROCESSOR_FRAMERATE_CONVERSION_ENABLE      L"FrameRateConversion"                 // BOOL (default=FALSE)  enables=TRUE / disables=FALSE

//steady video
#define AMF_VIDEO_PROCESSOR_STEADY_VIDEO_ENABLE              L"SteadyVideoEnable"                   // BOOL (default=FALSE)  enables=TRUE / disables=FALSE steady video effect
#define AMF_VIDEO_PROCESSOR_STEADY_VIDEO_STRENGTH            L"SteadyVideoStrength"                 // INT (default=3) min=0 max=3 steady video effect strength
#define AMF_VIDEO_PROCESSOR_STEADY_VIDEO_DELAY               L"SteadyVideoDelay"                    // INT (default=1) min=0 max=6 steady video effect delay
#define AMF_VIDEO_PROCESSOR_STEADY_VIDEO_ZOOM                L"SteadyVideoZoom"                     // INT (default=100) min=90 max=100 steady video effect delay


#endif //#ifndef __AMFVideoProcessor_h__
