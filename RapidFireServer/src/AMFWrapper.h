#pragma once

#include <mutex>

#include "core/Factory.h"

class AMFWrapper
{
public:

    static AMF_RESULT CreateContext(amf::AMFContext** ppContext);
    static AMF_RESULT CreateComponent(amf::AMFContext* pContext, const wchar_t* id, amf::AMFComponent** ppComponent);

private:

    AMFWrapper() {}

    static AMF_RESULT Init();

    static HMODULE             s_hDLLHandle;
    static amf::AMFFactory*    s_pFactory;
    static std::mutex          s_lock;
};