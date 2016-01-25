#pragma once

#include <string>
#include <vector>

#include <Windows.h>

class  ADLWrapper;
class  GPU_LIST_ENTRY;

typedef enum _DOPPEventType 
{
    DOPP_DESKOTOP_EVENT = 0, 
    DOPP_MOUSE_EVENT,
    DOPP_MAX_EVENT,
} DOPPEventType;

// DOPPDrvInterface can be used by applications to query the DOPP state and to register
// DOPP specific events like DOPP_DESKOTOP_EVENT or DOPP_MOUSE_EVENT.
// The application can create user events and should delete them by calling deleteDOPPEvent.
// It is allowed to register multiple events. Signaling is done based on GPU events.
// If a GPU event of a certain type occurs all user events of the same type will get signaled.
class DOPPDrvInterface
{
public:

    DOPPDrvInterface(const std::string& strDisplayName, unsigned int uiBusId);

    // The destructor will un-register all kmd events.
    // If DOPP was enabled by calling enableDOPP, the destructor will disable DOPP.
    ~DOPPDrvInterface();

    // Gets current DOPP state.
    bool getDoppState();

    // Tries to enable DOPP. 
    // This will only work on drivers that have DOPP support enabled.
    bool enableDopp();

    // Disbales DOPP, should only be called if DOPP was enabled by calling enableDopp.
    bool disableDopp();

    // Creates and registers a user event that gets signaled by the driver.
    HANDLE createDOPPEvent(DOPPEventType eventType);

    // Removes a user event.
    void deleteDOPPEvent(HANDLE hEvent);

private:

    struct GPU_USER_EVENT
    {
        HANDLE          hEvent;
        DOPPEventType   EventType;;
    };

    // Builds a static list containing information on all GPUs in the system.
    bool buildGpuList();

    // The device context used for this intance. 
    // The DC is required to register events.
    HDC m_hDC;

    // List of user events that were registered with createDOPPEvent.
    std::vector<GPU_USER_EVENT> m_UserEventList;

    // Pointer to the entry of the GPU on which this intance is created.
    GPU_LIST_ENTRY* m_pMyGpu;

    const ADLWrapper& m_adl;
};