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

#include <fstream>

#include "string.h"
#include "stdio.h"
#include "RFConfigEncode.h"
#include "RFError.h"
#include "RFUtils.h"

#include "RFSession.h"

static OVConfigCtrl gConfigCtrl;

// configuration mapping table
static OVConfigMap gConfigTable[] =
{
    // EncodeSpecifications
    { "pictureHeight",               gConfigCtrl.Height,                                 144 },
    { "pictureWidth",                gConfigCtrl.Width,                                  176 },
    { "EncodeMode",                  gConfigCtrl.EncodeMode,                             1 },
    { "level",                       gConfigCtrl.ProfileLevel.level,                     30 },
    { "profile",                     gConfigCtrl.ProfileLevel.profile,                   66 },
    { "pictureFormat",               gConfigCtrl.PictFormat,                             1 },
    { "requestedPriority",           gConfigCtrl.Priority,                               1 },

    // ConfigPicCtl
    { "useConstrainedIntraPred",     gConfigCtrl.PictControl.useConstrainedIntraPred,    0 },
    { "CABACEnable",                 gConfigCtrl.PictControl.cabacEnable,                0 },
    { "CABACIDC",                    gConfigCtrl.PictControl.cabacIDC,                   0 },
    { "loopFilterDisable",           gConfigCtrl.PictControl.loopFilterDisable,          0 },
    { "encLFBetaOffset",             gConfigCtrl.PictControl.encLFBetaOffset,            0 },
    { "encLFAlphaC0Offset",          gConfigCtrl.PictControl.encLFAlphaC0Offset,         0 },
    { "encIDRPeriod",                gConfigCtrl.PictControl.encIDRPeriod,               0 },
    { "encIPicPeriod",               gConfigCtrl.PictControl.encIPicPeriod,              0 },
    { "encHeaderInsertionSpacing",   gConfigCtrl.PictControl.encHeaderInsertionSpacing,  0 },
    { "encCropLeftOffset",           gConfigCtrl.PictControl.encCropLeftOffset,          0 },
    { "encCropRightOffset",          gConfigCtrl.PictControl.encCropRightOffset,         0 },
    { "encCropTopOffset",            gConfigCtrl.PictControl.encCropTopOffset,           0 },
    { "encCropBottomOffset",         gConfigCtrl.PictControl.encCropBottomOffset,        0 },
    { "encNumMBsPerSlice",           gConfigCtrl.PictControl.encNumMBsPerSlice,          99 },
    { "encNumSlicesPerFrame",        gConfigCtrl.PictControl.encNumSlicesPerFrame,       3 },
    { "encForceIntraRefresh",        gConfigCtrl.PictControl.encForceIntraRefresh,       0 },
    { "encForceIMBPeriod",           gConfigCtrl.PictControl.encForceIMBPeriod,          0 },
    { "encInsertVUIParam",           gConfigCtrl.PictControl.encInsertVUIParam,          0 },
    { "encInsertSEIMsg",             gConfigCtrl.PictControl.encInsertSEIMsg,            0 },

    // ConfigRateCtl
    { "encRateControlMethod",        gConfigCtrl.RateControl.encRateControlMethod,       1 },
    { "encRateControlTargetBitRate", gConfigCtrl.RateControl.encRateControlTargetBitRate, 768000 },
    { "encRateControlPeakBitRate",   gConfigCtrl.RateControl.encRateControlPeakBitRate,  0 },
    { "encRateControlFrameRateNumerator",     gConfigCtrl.RateControl.encRateControlFrameRateNumerator, 30 },
    { "encGOPSize",                  gConfigCtrl.RateControl.encGOPSize,                 0 },
    { "encRCOptions",                gConfigCtrl.RateControl.encRCOptions,               0 },
    { "encQP_I",                     gConfigCtrl.RateControl.encQP_I,                    28 },
    { "encQP_P",                     gConfigCtrl.RateControl.encQP_P,                    28 },
    { "encQP_B",                     gConfigCtrl.RateControl.encQP_B,                    0 },
    { "encVBVBufferSize",            gConfigCtrl.RateControl.encVBVBufferSize,           175000 },
    { "encRateControlFrameRateDenominator",     gConfigCtrl.RateControl.encRateControlFrameRateDenominator, 1 },

    // ConfigMotionEstimation
    { "IMEDecimationSearch",         gConfigCtrl.MEControl.imeDecimationSearch,          0 },
    { "motionEstHalfPixel",          gConfigCtrl.MEControl.motionEstHalfPixel,           1 },
    { "motionEstQuarterPixel",       gConfigCtrl.MEControl.motionEstQuarterPixel,        1 },
    { "disableFavorPMVPoint",        gConfigCtrl.MEControl.disableFavorPMVPoint,         0 },
    { "forceZeroPointCenter",        gConfigCtrl.MEControl.forceZeroPointCenter,         0 },
    { "LSMVert",                     gConfigCtrl.MEControl.lsmVert,                      0 },
    { "encSearchRangeX",             gConfigCtrl.MEControl.encSearchRangeX,              16 },
    { "encSearchRangeY",             gConfigCtrl.MEControl.encSearchRangeY,              16 },
    { "encSearch1RangeX",            gConfigCtrl.MEControl.encSearch1RangeX,             16 },
    { "encSearch1RangeY",            gConfigCtrl.MEControl.encSearch1RangeY,             16 },
    { "disable16x16Frame1",          gConfigCtrl.MEControl.disable16x16Frame1,           0 },
    { "disableSATD",                 gConfigCtrl.MEControl.disableSATD,                  0 },
    { "enableAMD",                   gConfigCtrl.MEControl.enableAMD,                    0 },
    { "encDisableSubMode",           gConfigCtrl.MEControl.encDisableSubMode,            0 },
    { "encIMESkipX",                 gConfigCtrl.MEControl.encIMESkipX,                  0 },
    { "encIMESkipY",                 gConfigCtrl.MEControl.encIMESkipY,                  0 },
    { "encEnImeOverwDisSubm",        gConfigCtrl.MEControl.encEnImeOverwDisSubm,         0 },
    { "encImeOverwDisSubmNo",        gConfigCtrl.MEControl.encImeOverwDisSubmNo,         0 },
    { "encIME2SearchRangeX",         gConfigCtrl.MEControl.encIME2SearchRangeX,          1 },
    { "encIME2SearchRangeY",         gConfigCtrl.MEControl.encIME2SearchRangeY,          1 },

    // ConfigRDO
    { "encDisableTbePredIFrame",     gConfigCtrl.RDOControl.encDisableTbePredIFrame,     0 },
    { "encDisableTbePredPFrame",     gConfigCtrl.RDOControl.encDisableTbePredPFrame,     0 },
    { "useFmeInterpolY",             gConfigCtrl.RDOControl.useFmeInterpolY,             0 },
    { "useFmeInterpolUV",            gConfigCtrl.RDOControl.useFmeInterpolUV,            0 },
    { "enc16x16CostAdj",             gConfigCtrl.RDOControl.enc16x16CostAdj,             0 },
    { "encSkipCostAdj",              gConfigCtrl.RDOControl.encSkipCostAdj,              0 },
    { "encForce16x16skip",           gConfigCtrl.RDOControl.encForce16x16skip,           0 },
    { "preset",                      gConfigCtrl.preset,                                 1 },
    { NULL,                          0,                                                  0 },
};


static const unsigned H264_Profiles[] = { 66, 77, 100, 122, 110, 144, 88, 83, 86, 118, 128, 256, 257, 258, 259 };

// 1 1.1 1.2 1.3 2 2.1 2.2 3 3.1 3.2 4 4.1 4.2 5 5.1 5.2
static const unsigned H264_Levels[]   = { 10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 40, 41, 42, 50, 51, 52 };


ConfigEncode::ConfigEncode()
{
    m_pConfig = NULL;
}

ConfigEncode::~ConfigEncode()
{
    if (m_pConfig)
    {
        delete m_pConfig;
    }
}

RFStatus ConfigEncode::createConfig()
{
    if (m_pConfig)
    {
        delete m_pConfig;
    }

    m_pConfig = new (std::nothrow)OVConfigCtrl;
    if (!m_pConfig)
    {
        return RF_STATUS_MEMORY_FAIL;
    }

    memset(m_pConfig, 0, sizeof(*m_pConfig));

    return RF_STATUS_OK;
}


RFStatus ConfigEncode::createConfig(unsigned int uiWidth, unsigned int uiHeight, RFEncodePreset p)
{
    SAFE_CALL_RF(createConfig());

    unsigned int uiBitRate    = 6000000;
    unsigned int uiFPS        = 30;

    RFStatus status = RF_STATUS_OK;

    switch (p)
    {
    case RF_ENCODE_FAST:
        uiBitRate = 4000000;
        createConfigFast(uiWidth, uiHeight, uiBitRate, uiFPS);
        break;
    case RF_ENCODE_BALANCED:
        createConfigBalanced(uiWidth, uiHeight, uiBitRate, uiFPS);
        break;
    case RF_ENCODE_QUALITY:
        uiBitRate = 8000000;
        createConfigQuality(uiWidth, uiHeight, uiBitRate, uiFPS);
        break;
    default:
        RF_Error(RF_STATUS_INVALID_CONFIG, "Input preset is out of range");
        status = RF_STATUS_INVALID_CONFIG;
        break;
    }

    return status;
}

RFStatus ConfigEncode::createConfig(struct RFProperty* property)
{
    if (!property)
    {
        return RF_STATUS_INVALID_ENCODER_PROPERTIES;
    }

    SAFE_CALL_RF(createConfig());

    unsigned int uiWidth        = property->ENCODER_SETTINGS.VIDEO_WIDTH;
    unsigned int uiHeight       = property->ENCODER_SETTINGS.VIDEO_HEIGHT;
    unsigned int uiBitRate      = property->ENCODER_SETTINGS.BITRATE;
    unsigned int uiFPS          = property->ENCODER_SETTINGS.FPS;

    createConfigBalanced(uiWidth, uiHeight, uiBitRate, uiFPS);

    m_pConfig->ProfileLevel.profile             = checkValidProfile(property->ENCODER_SETTINGS.PROFILE);
    m_pConfig->ProfileLevel.level               = checkValidLevel(property->ENCODER_SETTINGS.LEVEL);
    m_pConfig->RateControl.encQP_I              = property->ENCODER_SETTINGS.QP_I;
    m_pConfig->RateControl.encQP_P              = property->ENCODER_SETTINGS.QP_P;
    m_pConfig->RateControl.encQP_B              = property->ENCODER_SETTINGS.QP_B;
    m_pConfig->RateControl.encGOPSize           = property->ENCODER_SETTINGS.GOP_SIZE;
    m_pConfig->RateControl.encRateControlMethod = property->ENCODER_SETTINGS.RC_METHOD;
    m_pConfig->RateControl.encRCOptions         = property->ENCODER_SETTINGS.RC_OPTIONS;
    m_pConfig->RateControl.encVBVBufferSize     = property->ENCODER_SETTINGS.VBV_BUFFER_SIZE;
    m_pConfig->MEControl.encSearchRangeX        = property->ENCODER_SETTINGS.ME_RANGE;
    m_pConfig->MEControl.encSearchRangeY        = property->ENCODER_SETTINGS.ME_RANGE;
    m_pConfig->PictControl.encIDRPeriod         = property->ENCODER_SETTINGS.IDR_PERIOD;
    m_pConfig->PictControl.encNumSlicesPerFrame = property->ENCODER_SETTINGS.NUM_SLICES_PER_FRAME;
    m_pConfig->PictControl.encForceIntraRefresh = property->ENCODER_SETTINGS.FORCE_INTRA_REFRESH;
    m_pConfig->psnr                             = property->ENCODER_SETTINGS.PSNR;

    return RF_STATUS_OK;
}

// Setting up configuration parameters
void ConfigEncode::EncodeSetParam(OVConfigMap *pConfigTable)
{
    // fill-in the general configuration structures
    m_pConfig->Height                                   = pConfigTable[0].param;
    m_pConfig->Width                                    = pConfigTable[1].param;
    m_pConfig->EncodeMode                               = (OVE_ENCODE_MODE)pConfigTable[2].param;

    // fill-in the profile and level
    m_pConfig->ProfileLevel.level                       = pConfigTable[3].param;
    m_pConfig->ProfileLevel.profile                     = pConfigTable[4].param;

    m_pConfig->PictFormat                               = (OVE_PICTURE_FORMAT)pConfigTable[5].param;
    m_pConfig->Priority                                 = (OVE_ENCODE_TASK_PRIORITY)pConfigTable[6].param;

    // fill-in the picture control structures
    m_pConfig->PictControl.size                         = sizeof(OVE_CONFIG_PICTURE_CONTROL);
    m_pConfig->PictControl.useConstrainedIntraPred      = pConfigTable[7].param;
    m_pConfig->PictControl.cabacEnable                  = pConfigTable[8].param;
    m_pConfig->PictControl.cabacIDC                     = pConfigTable[9].param;
    m_pConfig->PictControl.loopFilterDisable            = pConfigTable[10].param;
    m_pConfig->PictControl.encLFBetaOffset              = pConfigTable[11].param;
    m_pConfig->PictControl.encLFAlphaC0Offset           = pConfigTable[12].param;
    m_pConfig->PictControl.encIDRPeriod                 = pConfigTable[13].param;
    m_pConfig->PictControl.encIPicPeriod                = pConfigTable[14].param;
    m_pConfig->PictControl.encHeaderInsertionSpacing    = pConfigTable[15].param;
    m_pConfig->PictControl.encCropLeftOffset            = pConfigTable[16].param;
    m_pConfig->PictControl.encCropRightOffset           = pConfigTable[17].param;
    m_pConfig->PictControl.encCropTopOffset             = pConfigTable[18].param;
    m_pConfig->PictControl.encCropBottomOffset          = pConfigTable[19].param;
    m_pConfig->PictControl.encNumMBsPerSlice            = pConfigTable[20].param;
    m_pConfig->PictControl.encNumSlicesPerFrame         = pConfigTable[21].param;
    m_pConfig->PictControl.encForceIntraRefresh         = pConfigTable[22].param;
    m_pConfig->PictControl.encForceIMBPeriod            = pConfigTable[23].param;
    m_pConfig->PictControl.encInsertVUIParam            = pConfigTable[24].param;
    m_pConfig->PictControl.encInsertSEIMsg              = pConfigTable[25].param;

    // fill-in the rate control structures
    m_pConfig->RateControl.size                         = sizeof(OVE_CONFIG_RATE_CONTROL);
    m_pConfig->RateControl.encRateControlMethod         = pConfigTable[26].param;
    m_pConfig->RateControl.encRateControlTargetBitRate  = pConfigTable[27].param;
    m_pConfig->RateControl.encRateControlPeakBitRate    = pConfigTable[28].param;
    m_pConfig->RateControl.encRateControlFrameRateNumerator = pConfigTable[29].param;
    m_pConfig->RateControl.encGOPSize                   = pConfigTable[30].param;
    m_pConfig->RateControl.encRCOptions                 = pConfigTable[31].param;
    m_pConfig->RateControl.encQP_I                      = pConfigTable[32].param;
    m_pConfig->RateControl.encQP_P                      = pConfigTable[33].param;
    m_pConfig->RateControl.encQP_B                      = pConfigTable[34].param;
    m_pConfig->RateControl.encVBVBufferSize             = pConfigTable[35].param;
    m_pConfig->RateControl.encRateControlFrameRateDenominator = pConfigTable[36].param;

    // fill-in the motion estimation control structures
    m_pConfig->MEControl.size                           = sizeof(OVE_CONFIG_MOTION_ESTIMATION);
    m_pConfig->MEControl.imeDecimationSearch            = pConfigTable[37].param;
    m_pConfig->MEControl.motionEstHalfPixel             = pConfigTable[38].param;
    m_pConfig->MEControl.motionEstQuarterPixel          = pConfigTable[39].param;
    m_pConfig->MEControl.disableFavorPMVPoint           = pConfigTable[40].param;
    m_pConfig->MEControl.forceZeroPointCenter           = pConfigTable[41].param;
    m_pConfig->MEControl.lsmVert                        = pConfigTable[42].param;
    m_pConfig->MEControl.encSearchRangeX                = pConfigTable[43].param;
    m_pConfig->MEControl.encSearchRangeY                = pConfigTable[44].param;
    m_pConfig->MEControl.encSearch1RangeX               = pConfigTable[45].param;
    m_pConfig->MEControl.encSearch1RangeY               = pConfigTable[46].param;
    m_pConfig->MEControl.disable16x16Frame1             = pConfigTable[47].param;
    m_pConfig->MEControl.disableSATD                    = pConfigTable[48].param;
    m_pConfig->MEControl.enableAMD                      = pConfigTable[49].param;
    m_pConfig->MEControl.encDisableSubMode              = pConfigTable[50].param;
    m_pConfig->MEControl.encIMESkipX                    = pConfigTable[51].param;
    m_pConfig->MEControl.encIMESkipY                    = pConfigTable[52].param;
    m_pConfig->MEControl.encEnImeOverwDisSubm           = pConfigTable[53].param;
    m_pConfig->MEControl.encImeOverwDisSubmNo           = pConfigTable[54].param;
    m_pConfig->MEControl.encIME2SearchRangeX            = pConfigTable[55].param;
    m_pConfig->MEControl.encIME2SearchRangeY            = pConfigTable[56].param;

    // fill-in the RDO control structures
    m_pConfig->RDOControl.size                          = sizeof(OVE_CONFIG_RDO);
    m_pConfig->RDOControl.encDisableTbePredIFrame       = pConfigTable[57].param;
    m_pConfig->RDOControl.encDisableTbePredPFrame       = pConfigTable[58].param;
    m_pConfig->RDOControl.useFmeInterpolY               = pConfigTable[59].param;
    m_pConfig->RDOControl.useFmeInterpolUV              = pConfigTable[60].param;
    m_pConfig->RDOControl.enc16x16CostAdj               = pConfigTable[61].param;
    m_pConfig->RDOControl.encSkipCostAdj                = pConfigTable[62].param;
    m_pConfig->RDOControl.encForce16x16skip             = static_cast<unsigned char>(pConfigTable[63].param);
    m_pConfig->preset                                   = pConfigTable[64].param;
}



// Reading in user-specified configuration file
/*
RFStatus ConfigEncode::readConfigFile(char *fileName)
{
    char name[128];
    int index;
    int value;

    std::ifstream file;

    file.open(fileName);

    if (!file.is_open())
    {
        // Search the config file again in the path of the executable file of the current process.
        std::string path = utilGetExecutablePath();
        path.append(fileName);
        file.open(path.c_str());
        if (!file.is_open())
        {
            char buf[256];
            sprintf_s(buf, "Cannot open file %s", fileName);
            RF_Error(RF_STATUS_FILE_FAIL, buf);
            return RF_STATUS_FILE_FAIL;
        }
    }

    std::string line;
    size_t lineNo = 0;

    while (std::getline(file, line))
    {
        index = 0;
        while (gConfigTable[index].key != NULL)
        {
            if (line.find(gConfigTable[index].key) != std::string::npos)
            {
                sscanf(line.c_str(), "%s %d", name, &value);
                gConfigTable[index].param = value;
                break;
            }
            index++;
        }

        lineNo++;
    }

    // User-specified configurations override the default values
    EncodeSetParam(gConfigTable);

    file.close();

    return RF_STATUS_OK;
}
*/

RFStatus ConfigEncode::createConfigFast(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiBitRate, unsigned int uiFPS)
{
    if (!m_pConfig)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    m_pConfig->Width                 = uiWidth;
    m_pConfig->Height                = uiHeight;
    m_pConfig->EncodeMode            = OVE_AVC_FULL;
    m_pConfig->ProfileLevel.profile  = 77;
    m_pConfig->ProfileLevel.level    = 41;
    m_pConfig->PictFormat            = OVE_PICTURE_FORMAT_NV12;
    m_pConfig->Priority              = OVE_ENCODE_TASK_PRIORITY_LEVEL1;

    m_pConfig->PictControl.size                      = sizeof(OVE_CONFIG_PICTURE_CONTROL);
    m_pConfig->PictControl.useConstrainedIntraPred   = 0;
    m_pConfig->PictControl.cabacEnable               = 1;
    m_pConfig->PictControl.loopFilterDisable         = 0; //disable de-blocking filter
    m_pConfig->PictControl.cabacIDC                  = 0;
    m_pConfig->PictControl.encLFBetaOffset           = 0;
    m_pConfig->PictControl.encLFAlphaC0Offset        = 0;
    m_pConfig->PictControl.encIDRPeriod              = 0;
    m_pConfig->PictControl.encIPicPeriod             = 0;
    m_pConfig->PictControl.encHeaderInsertionSpacing = 0; //insert header in the beginning
    m_pConfig->PictControl.encCropLeftOffset         = 0;
    m_pConfig->PictControl.encCropRightOffset        = 0;
    m_pConfig->PictControl.encCropTopOffset          = 0;
    m_pConfig->PictControl.encCropBottomOffset       = 0;
    m_pConfig->PictControl.encNumMBsPerSlice         = 0;
    m_pConfig->PictControl.encNumSlicesPerFrame      = 1;
    m_pConfig->PictControl.encForceIntraRefresh      = 0;
    m_pConfig->PictControl.encForceIMBPeriod         = 0;
    m_pConfig->PictControl.encInsertVUIParam         = 0;
    m_pConfig->PictControl.encInsertSEIMsg           = 0;

    //Rate control config
    m_pConfig->RateControl.size = sizeof(OVE_CONFIG_RATE_CONTROL);
    m_pConfig->RateControl.encRateControlMethod                  = 3;
    m_pConfig->RateControl.encRateControlTargetBitRate           = uiBitRate;
    m_pConfig->RateControl.encRateControlPeakBitRate             = 0;
    m_pConfig->RateControl.encRateControlFrameRateNumerator      = uiFPS;
    m_pConfig->RateControl.encRateControlFrameRateDenominator    = 1;
    m_pConfig->RateControl.encGOPSize                            = 32;

    m_pConfig->RateControl.encVBVBufferSize      = uiBitRate / 2;
    m_pConfig->RateControl.encQP_I               = 22;
    m_pConfig->RateControl.encQP_P               = 22;
    m_pConfig->RateControl.encQP_B               = 0;
    m_pConfig->RateControl.encRCOptions          = 0;

    //Motion estimation config
    m_pConfig->MEControl.size                    = sizeof(OVE_CONFIG_MOTION_ESTIMATION);
    m_pConfig->MEControl.imeDecimationSearch     = 1; //Decimation search on
    m_pConfig->MEControl.motionEstHalfPixel      = 1; //half pel ME
    m_pConfig->MEControl.motionEstQuarterPixel   = 1; //quarter pel ME
    m_pConfig->MEControl.disableFavorPMVPoint    = 0; //favor PMVPoint
    m_pConfig->MEControl.forceZeroPointCenter    = 0;
    m_pConfig->MEControl.lsmVert                 = 0;  //LSM search window
    m_pConfig->MEControl.encSearchRangeX         = 16; //Lite2
    m_pConfig->MEControl.encSearchRangeY         = 16; //Lite2
    m_pConfig->MEControl.encSearch1RangeX        = 0; //Lite2
    m_pConfig->MEControl.encSearch1RangeY        = 0; //Lite2
    m_pConfig->MEControl.encIME2SearchRangeX     = 4;
    m_pConfig->MEControl.encIME2SearchRangeY     = 4;
    m_pConfig->MEControl.disable16x16Frame1      = 0;
    m_pConfig->MEControl.disableSATD             = 0; //SAD only
    m_pConfig->MEControl.enableAMD               = 0; //FME advanced mode decision
    m_pConfig->MEControl.encDisableSubMode       = 254; //Lite2
    m_pConfig->MEControl.encIMESkipX             = 0;
    m_pConfig->MEControl.encIMESkipY             = 0;
    m_pConfig->MEControl.encEnImeOverwDisSubm    = 0;
    m_pConfig->MEControl.encImeOverwDisSubmNo    = 0;
    m_pConfig->MEControl.encIME2SearchRangeX     = 4; //Lite2
    m_pConfig->MEControl.encIME2SearchRangeY     = 4; //Lite2

    //RDO control config
    m_pConfig->RDOControl.size = sizeof(OVE_CONFIG_RDO);
    m_pConfig->RDOControl.encDisableTbePredIFrame             = 0;
    m_pConfig->RDOControl.encDisableTbePredPFrame             = 0;
    m_pConfig->RDOControl.useFmeInterpolY                     = 0;
    m_pConfig->RDOControl.useFmeInterpolUV                    = 0;
    m_pConfig->RDOControl.enc16x16CostAdj                     = 0;
    m_pConfig->RDOControl.encSkipCostAdj                      = 0;
    m_pConfig->RDOControl.encForce16x16skip                   = 1;
    m_pConfig->preset                                         = RF_ENCODE_FAST;

    return RF_STATUS_OK;
}

RFStatus ConfigEncode::createConfigBalanced(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiBitRate, unsigned int uiFPS)
{
    if (!m_pConfig)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    m_pConfig->Width                 = uiWidth;
    m_pConfig->Height                = uiHeight;
    m_pConfig->EncodeMode            = OVE_AVC_FULL;
    m_pConfig->ProfileLevel.profile  = 77;
    m_pConfig->ProfileLevel.level    = 41;
    m_pConfig->PictFormat            = OVE_PICTURE_FORMAT_NV12;
    m_pConfig->Priority              = OVE_ENCODE_TASK_PRIORITY_LEVEL1;

    m_pConfig->PictControl.size                      = sizeof(OVE_CONFIG_PICTURE_CONTROL);
    m_pConfig->PictControl.useConstrainedIntraPred   = 0;
    m_pConfig->PictControl.cabacEnable               = 1;
    m_pConfig->PictControl.loopFilterDisable         = 0; //disable de-blocking filter
    m_pConfig->PictControl.cabacIDC                  = 0;
    m_pConfig->PictControl.encLFBetaOffset           = 0;
    m_pConfig->PictControl.encLFAlphaC0Offset        = 0;
    m_pConfig->PictControl.encIDRPeriod              = 0;
    m_pConfig->PictControl.encIPicPeriod             = 0;
    m_pConfig->PictControl.encHeaderInsertionSpacing = 0; //insert header in the beginning
    m_pConfig->PictControl.encCropLeftOffset         = 0;
    m_pConfig->PictControl.encCropRightOffset        = 0;
    m_pConfig->PictControl.encCropTopOffset          = 0;
    m_pConfig->PictControl.encCropBottomOffset       = 0;
    m_pConfig->PictControl.encNumMBsPerSlice         = 0;
    m_pConfig->PictControl.encNumSlicesPerFrame      = 1;
    m_pConfig->PictControl.encForceIntraRefresh      = 0;
    m_pConfig->PictControl.encForceIMBPeriod         = 0;
    m_pConfig->PictControl.encInsertVUIParam         = 0;
    m_pConfig->PictControl.encInsertSEIMsg           = 0;

    //Rate control config
    m_pConfig->RateControl.size = sizeof(OVE_CONFIG_RATE_CONTROL);
    m_pConfig->RateControl.encRateControlMethod                  = 3;
    m_pConfig->RateControl.encRateControlTargetBitRate           = uiBitRate;
    m_pConfig->RateControl.encRateControlPeakBitRate             = 0;
    m_pConfig->RateControl.encRateControlFrameRateNumerator      = uiFPS;
    m_pConfig->RateControl.encRateControlFrameRateDenominator    = 1;
    m_pConfig->RateControl.encGOPSize                            = 32;

    m_pConfig->RateControl.encVBVBufferSize      = uiBitRate / 2;
    m_pConfig->RateControl.encQP_I               = 22;
    m_pConfig->RateControl.encQP_P               = 22;
    m_pConfig->RateControl.encQP_B               = 0;
    m_pConfig->RateControl.encRCOptions          = 0;

    //Motion estimation config
    m_pConfig->MEControl.size                    = sizeof(OVE_CONFIG_MOTION_ESTIMATION);
    m_pConfig->MEControl.imeDecimationSearch     = 1; //Decimation search on
    m_pConfig->MEControl.motionEstHalfPixel      = 1; //half pel ME
    m_pConfig->MEControl.motionEstQuarterPixel   = 1; //quarter pel ME
    m_pConfig->MEControl.disableFavorPMVPoint    = 0; //favor PMVPoint
    m_pConfig->MEControl.forceZeroPointCenter    = 0;
    m_pConfig->MEControl.lsmVert                 = 0;  //LSM search window
    m_pConfig->MEControl.encSearchRangeX         = 24; //Lite2
    m_pConfig->MEControl.encSearchRangeY         = 24; //Lite2
    m_pConfig->MEControl.encSearch1RangeX        = 0; //Lite2
    m_pConfig->MEControl.encSearch1RangeY        = 0; //Lite2
    m_pConfig->MEControl.encIME2SearchRangeX     = 4;
    m_pConfig->MEControl.encIME2SearchRangeY     = 4;
    m_pConfig->MEControl.disable16x16Frame1      = 0;
    m_pConfig->MEControl.disableSATD             = 0; //SAD only
    m_pConfig->MEControl.enableAMD               = 0; //FME advanced mode decision
    m_pConfig->MEControl.encDisableSubMode       = 120; //Lite2
    m_pConfig->MEControl.encIMESkipX             = 0;
    m_pConfig->MEControl.encIMESkipY             = 0;
    m_pConfig->MEControl.encEnImeOverwDisSubm    = 1;
    m_pConfig->MEControl.encImeOverwDisSubmNo    = 1;
    m_pConfig->MEControl.encIME2SearchRangeX     = 4; //Lite2
    m_pConfig->MEControl.encIME2SearchRangeY     = 4; //Lite2

    //RDO control config
    m_pConfig->RDOControl.size = sizeof(OVE_CONFIG_RDO);
    m_pConfig->RDOControl.encDisableTbePredIFrame             = 0;
    m_pConfig->RDOControl.encDisableTbePredPFrame             = 0;
    m_pConfig->RDOControl.useFmeInterpolY                     = 0;
    m_pConfig->RDOControl.useFmeInterpolUV                    = 0;
    m_pConfig->RDOControl.enc16x16CostAdj                     = 0;
    m_pConfig->RDOControl.encSkipCostAdj                      = 0;
    m_pConfig->RDOControl.encForce16x16skip                   = 0;
    m_pConfig->preset                                         = RF_ENCODE_BALANCED;

    return RF_STATUS_OK;
}

RFStatus ConfigEncode::createConfigQuality(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiBitRate, unsigned int uiFPS)
{
    if (!m_pConfig)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    m_pConfig->Width                 = uiWidth;
    m_pConfig->Height                = uiHeight;
    m_pConfig->EncodeMode            = OVE_AVC_FULL;
    m_pConfig->ProfileLevel.profile  = 77;
    m_pConfig->ProfileLevel.level    = 41;
    m_pConfig->PictFormat            = OVE_PICTURE_FORMAT_NV12;
    m_pConfig->Priority              = OVE_ENCODE_TASK_PRIORITY_LEVEL1;

    m_pConfig->PictControl.size                      = sizeof(OVE_CONFIG_PICTURE_CONTROL);
    m_pConfig->PictControl.useConstrainedIntraPred   = 0;
    m_pConfig->PictControl.cabacEnable               = 1;
    m_pConfig->PictControl.loopFilterDisable         = 0; //disable de-blocking filter
    m_pConfig->PictControl.cabacIDC                  = 0;
    m_pConfig->PictControl.encLFBetaOffset           = 0;
    m_pConfig->PictControl.encLFAlphaC0Offset        = 0;
    m_pConfig->PictControl.encIDRPeriod              = 0;
    m_pConfig->PictControl.encIPicPeriod             = 0;
    m_pConfig->PictControl.encHeaderInsertionSpacing = 0; //insert header in the beginning
    m_pConfig->PictControl.encCropLeftOffset         = 0;
    m_pConfig->PictControl.encCropRightOffset        = 0;
    m_pConfig->PictControl.encCropTopOffset          = 0;
    m_pConfig->PictControl.encCropBottomOffset       = 0;
    m_pConfig->PictControl.encNumMBsPerSlice         = 0;
    m_pConfig->PictControl.encNumSlicesPerFrame      = 1;
    m_pConfig->PictControl.encForceIntraRefresh      = 0;
    m_pConfig->PictControl.encForceIMBPeriod         = 0;
    m_pConfig->PictControl.encInsertVUIParam         = 0;
    m_pConfig->PictControl.encInsertSEIMsg           = 0;

    //Rate control config
    m_pConfig->RateControl.size = sizeof(OVE_CONFIG_RATE_CONTROL);
    m_pConfig->RateControl.encRateControlMethod                  = 3;
    m_pConfig->RateControl.encRateControlTargetBitRate           = uiBitRate;
    m_pConfig->RateControl.encRateControlPeakBitRate             = 0;
    m_pConfig->RateControl.encRateControlFrameRateNumerator      = uiFPS;
    m_pConfig->RateControl.encRateControlFrameRateDenominator    = 1;
    m_pConfig->RateControl.encGOPSize                            = 32;

    m_pConfig->RateControl.encVBVBufferSize      = uiBitRate / 2;
    m_pConfig->RateControl.encQP_I               = 22;
    m_pConfig->RateControl.encQP_P               = 22;
    m_pConfig->RateControl.encQP_B               = 0;
    m_pConfig->RateControl.encRCOptions          = 0;

    //Motion estimation config
    m_pConfig->MEControl.size                    = sizeof(OVE_CONFIG_MOTION_ESTIMATION);
    m_pConfig->MEControl.imeDecimationSearch     = 1; //Decimation search on
    m_pConfig->MEControl.motionEstHalfPixel      = 1; //half pel ME
    m_pConfig->MEControl.motionEstQuarterPixel   = 1; //quarter pel ME
    m_pConfig->MEControl.disableFavorPMVPoint    = 0; //favor PMVPoint
    m_pConfig->MEControl.forceZeroPointCenter    = 1;
    m_pConfig->MEControl.lsmVert                 = 0;  //LSM search window
    m_pConfig->MEControl.encSearchRangeX         = 32; //Lite2
    m_pConfig->MEControl.encSearchRangeY         = 32; //Lite2
    m_pConfig->MEControl.encSearch1RangeX        = 0; //Lite2
    m_pConfig->MEControl.encSearch1RangeY        = 0; //Lite2
    m_pConfig->MEControl.encIME2SearchRangeX     = 4;
    m_pConfig->MEControl.encIME2SearchRangeY     = 4;
    m_pConfig->MEControl.disable16x16Frame1      = 0;
    m_pConfig->MEControl.disableSATD             = 0; //SAD only
    m_pConfig->MEControl.enableAMD               = 1; //FME advanced mode decision
    m_pConfig->MEControl.encDisableSubMode       = 0; //Lite2
    m_pConfig->MEControl.encIMESkipX             = 0;
    m_pConfig->MEControl.encIMESkipY             = 0;
    m_pConfig->MEControl.encEnImeOverwDisSubm    = 0;
    m_pConfig->MEControl.encImeOverwDisSubmNo    = 0;
    m_pConfig->MEControl.encIME2SearchRangeX     = 4; //Lite2
    m_pConfig->MEControl.encIME2SearchRangeY     = 4; //Lite2

    //RDO control config
    m_pConfig->RDOControl.size = sizeof(OVE_CONFIG_RDO);
    m_pConfig->RDOControl.encDisableTbePredIFrame             = 0;
    m_pConfig->RDOControl.encDisableTbePredPFrame             = 0;
    m_pConfig->RDOControl.useFmeInterpolY                     = 0;
    m_pConfig->RDOControl.useFmeInterpolUV                    = 0;
    m_pConfig->RDOControl.enc16x16CostAdj                     = 0;
    m_pConfig->RDOControl.encSkipCostAdj                      = 0;
    m_pConfig->RDOControl.encForce16x16skip                   = 0;
    m_pConfig->preset                                         = RF_ENCODE_QUALITY;

    return RF_STATUS_OK;
}

RFStatus ConfigEncode::setDynamicParameter(const int param, RFProperties value)
{
    if (!m_pConfig)
    {
        return RF_STATUS_INVALID_CONFIG;
    }

    switch (param)
    {
    case RF_ENCODER_BITRATE:
        if (!utilIsBitrateValid(value))
        {
            RF_Error(RF_STATUS_INVALID_CONFIG, "Input bitrate is out of range");
            return RF_STATUS_INVALID_CONFIG;
        }
        m_pConfig->RateControl.encRateControlTargetBitRate        = static_cast<unsigned int>(value);
        m_pConfig->RateControl.encRateControlPeakBitRate          = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_FPS:
        if (!utilIsFpsValid(value))
        {
            RF_Error(RF_STATUS_INVALID_CONFIG, "Input fps is out of range");
            return RF_STATUS_INVALID_CONFIG;
        }
        m_pConfig->RateControl.encRateControlFrameRateNumerator   = static_cast<unsigned int>(value);
        m_pConfig->RateControl.encRateControlFrameRateDenominator = 1;
        break;

    case RF_ENCODER_RC_METHOD:
        m_pConfig->RateControl.encRateControlMethod               = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_RC_OPTIONS:
        m_pConfig->RateControl.encRCOptions                       = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_RC_QP_I:
        m_pConfig->RateControl.encQP_I                            = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_RC_QP_P:
        m_pConfig->RateControl.encQP_P                            = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_RC_QP_B:
        m_pConfig->RateControl.encQP_B                            = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_RC_GOP_SIZE:
        m_pConfig->RateControl.encGOPSize                         = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_RC_VBV_BUFFER_SIZE:
        m_pConfig->RateControl.encVBVBufferSize                   = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_ME_RANGE:
        m_pConfig->MEControl.encSearchRangeX                      = static_cast<unsigned int>(value);
        m_pConfig->MEControl.encSearchRangeY                      = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_IDR_PERIOD:
        m_pConfig->PictControl.encIDRPeriod                       = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_NUM_SLICES_PER_FRAME:
        m_pConfig->PictControl.encNumSlicesPerFrame               = static_cast<unsigned int>(value);
        break;

    case RF_ENCODER_FORCE_INTRA_REFRESH:
        m_pConfig->PictControl.encForceIntraRefresh               = static_cast<unsigned int>(value);
        break;
    }

    return RF_STATUS_OK;
}

unsigned int ConfigEncode::getSingleParameter(const int param)
{
    unsigned int value = 0;

    if (!m_pConfig)
    {
        RF_Error(RF_STATUS_INVALID_CONFIG, "Invalid encoder config");
        return value;
    }

    if (param == RF_ENCODER_PROFILE)
    {
        value = m_pConfig->ProfileLevel.profile;
    }
    else if (param == RF_ENCODER_LEVEL)
    {
        value = m_pConfig->ProfileLevel.level;
    }
    else if (param == RF_ENCODER_BITRATE)
    {
        value = m_pConfig->RateControl.encRateControlTargetBitRate;
    }
    else if (param == RF_ENCODER_FPS)
    {
        value = m_pConfig->RateControl.encRateControlFrameRateNumerator;
    }

    return value;
}

unsigned int ConfigEncode::checkValidProfile(unsigned int profile)
{
    bool found = false;

    for (unsigned int i = 0; i < sizeof(H264_Profiles) / sizeof(H264_Profiles[0]); i++)
    {
        if (H264_Profiles[i] == profile)
        {
            found = true;
        }
    }

    if (!found)
    {
        char buf[128];
        sprintf_s(buf, 128, "[Warning] profile %d is not supported in H264. Switch to Main Profile (77).", profile);
        fprintf(stderr, "%s\n", buf);
        fflush(stderr);
        profile = 77;
    }

    return profile;
}

unsigned int ConfigEncode::checkValidLevel(unsigned int level)
{
    bool found = false;

    for (unsigned int i = 0; i < sizeof(H264_Levels) / sizeof(H264_Levels[0]); i++)
    {
        if (H264_Levels[i] == level)
        {
            found = true;
        }
    }

    if (!found)
    {
        char buf[128];
        sprintf_s(buf, 128, "[Warning] level %d is not supported in H264. Switch to 41.", level);
        fprintf(stderr, "%s\n", buf);
        fflush(stderr);
        level = 41;
    }

    return level;
}
