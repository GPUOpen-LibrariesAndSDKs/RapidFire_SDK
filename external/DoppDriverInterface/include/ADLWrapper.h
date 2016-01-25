#pragma once

#include "adl_sdk.h"

typedef int (*ADL2_MAIN_CONTROLX2_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int, ADL_CONTEXT_HANDLE*,  ADLThreadingModel); 
typedef int (*ADL2_MAIN_CONTROL_DESTROY)(ADL_CONTEXT_HANDLE);
typedef int (*ADL2_MAIN_CONTROL_REFRESH) (ADL_CONTEXT_HANDLE context);
typedef int (*ADL2_ADAPTER_NUMBEROFADAPTERS_GET) (ADL_CONTEXT_HANDLE, int*);
typedef int (*ADL2_ADAPTER_ACTIVE_GET) (ADL_CONTEXT_HANDLE, int, int*);
typedef int (*ADL2_ADAPTER_ADAPTERINFO_GET) (ADL_CONTEXT_HANDLE, LPAdapterInfo, int);
typedef int (*ADL2_DISPLAY_DISPLAYINFO_GET) (ADL_CONTEXT_HANDLE, int, int *, ADLDisplayInfo **, int);
typedef int (*ADL2_ADAPTER_PRIMARY_GET) (ADL_CONTEXT_HANDLE,int*);

class ADLWrapper
{
public:

    static ADLWrapper const& getInstance();

    inline ADL_CONTEXT_HANDLE const getHandle() const { return m_hADL; }
    operator bool() const { return m_bADLReady; }

    ADL2_MAIN_CONTROLX2_CREATE          ADL2_Main_ControlX2_Create;
    ADL2_MAIN_CONTROL_DESTROY           ADL2_Main_Control_Destroy;
    ADL2_MAIN_CONTROL_REFRESH           ADL2_Main_Control_Refresh;
    ADL2_ADAPTER_NUMBEROFADAPTERS_GET   ADL2_Adapter_NumberOfAdapters_Get;
    ADL2_ADAPTER_ACTIVE_GET             ADL2_Adapter_Active_Get;
    ADL2_ADAPTER_ADAPTERINFO_GET        ADL2_Adapter_AdapterInfo_Get;
    ADL2_DISPLAY_DISPLAYINFO_GET        ADL2_Display_DisplayInfo_Get;
    ADL2_ADAPTER_PRIMARY_GET            ADL2_Adapter_Primary_Get;

private:

    ADLWrapper();
    ~ADLWrapper();

    ADLWrapper(ADLWrapper const&);
    ADLWrapper& operator=(ADLWrapper const&);

    bool initADL();

    ADL_CONTEXT_HANDLE   m_hADL;
    bool                 m_bADLReady;
    HMODULE              m_hDLL;
};
