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
