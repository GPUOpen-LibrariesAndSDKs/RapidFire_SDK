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

#ifndef RAPIDFIRE_CONFIGENCODE_H_
#define RAPIDFIRE_CONFIGENCODE_H_

#include "RFTypes.h"
#include "RapidFireServer.h"

// configuration structure
struct OVConfigMap
{
    const char     *key;
    unsigned int    param;
    unsigned int    value;
};

class ConfigEncode
{
public:

    ConfigEncode();
    ~ConfigEncode();

    RFStatus         createConfig();

    // Create config by passing a preset
    RFStatus         createConfig(unsigned int uiWidth, unsigned int uiHeight, RFEncodePreset p);

    // Create config by passing properties
    RFStatus         createConfig(struct RFProperty* property);

    // Set a specific dynamic parameter
    RFStatus         setDynamicParameter(const int param, RFProperties value);

    unsigned int     getSingleParameter(const int param);

    OVConfigCtrl*    getConfigCtrl() { return m_pConfig; };

private:

    RFStatus createConfigFast(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiBitRate, unsigned int uiFPS);
    RFStatus createConfigBalanced(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiBitRate, unsigned int uiFPS);
    RFStatus createConfigQuality(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiBitRate, unsigned int uiFPS);

    void             EncodeSetParam(OVConfigMap *pConfigTable);
   
    unsigned int     checkValidProfile(unsigned int profile);
    unsigned int     checkValidLevel(unsigned int level);

private:

    OVConfigCtrl*    m_pConfig;
};

#endif // ifndef RAPIDFIRE_CONFIGENCODE_H_
