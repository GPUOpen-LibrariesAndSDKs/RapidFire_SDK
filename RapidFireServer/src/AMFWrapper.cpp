#include "AMFWrapper.h"

HMODULE             AMFWrapper::s_hDLLHandle = NULL;
amf::AMFFactory*    AMFWrapper::s_pFactory = nullptr;
std::mutex          AMFWrapper::s_lock;

AMF_RESULT AMFWrapper::CreateContext(amf::AMFContext** ppContext)
{
    std::lock_guard<std::mutex> lock(s_lock);
    if (!s_pFactory)
    {
        AMF_RESULT res = Init();
        if (res != AMF_OK)
        {
            return res;
        }
    }

    return s_pFactory->CreateContext(ppContext);
}

AMF_RESULT AMFWrapper::CreateComponent(amf::AMFContext* pContext, const wchar_t* id, amf::AMFComponent** ppComponent)
{
    std::lock_guard<std::mutex> lock(s_lock);
    if (!s_pFactory)
    {
        AMF_RESULT res = Init();
        if (res != AMF_OK)
        {
            return res;
        }
    }

    return s_pFactory->CreateComponent(pContext, id, ppComponent);
}

AMF_RESULT AMFWrapper::Init()
{
    if (!s_hDLLHandle)
    {
        s_hDLLHandle = LoadLibraryW(AMF_DLL_NAME);
        if (!s_hDLLHandle)
        {
            return AMF_FAIL;
        }
    }

    AMFInit_Fn initFun = (AMFInit_Fn)::GetProcAddress(s_hDLLHandle, AMF_INIT_FUNCTION_NAME);
    if (initFun == NULL)
    {
        return AMF_FAIL;
    }

    AMF_RESULT res = initFun(AMF_FULL_VERSION, &s_pFactory);
    if (res != AMF_OK)
    {
        return res;
    }

    return AMF_OK;
}
