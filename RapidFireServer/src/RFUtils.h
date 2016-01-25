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

#include <string>

// Gets the path of the executable.
std::string utilGetExecutablePath();

// Aligns a value by ALIGNMENT.
unsigned int utilAlignNum(unsigned int val, unsigned int ALIGNMENT);

bool utilIsFpsValid(size_t fps);

bool utilIsBitrateValid(size_t bitrate);

bool utilIsPropertyValid(size_t Property);

#ifdef _DEBUG
#include <CL/cl.h>

#include "RapidFireServer.h"
#include "RFContext.h"

void dumpCLBuffer(cl_mem clBuffer, RFContextCL* pContext, unsigned int uiWidth, unsigned int uiHeight, RFFormat rfFormat, const char* pFileName);
#endif