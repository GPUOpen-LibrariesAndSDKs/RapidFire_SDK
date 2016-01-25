#include "RFLock.h"


RFLock::RFLock()
{
    InitializeCriticalSection(&m_cs);
}


RFLock::~RFLock()
{
    // Release just in case lock is still used.
    LeaveCriticalSection(&m_cs);

    DeleteCriticalSection(&m_cs);
}


bool RFLock::lock()
{
    try
    {
        EnterCriticalSection(&m_cs);
    }
    catch (...)
    {
        // Catch possible exception EXCEPTION_POSSIBLE_DEADLOCK.
        return false;
    }

    return true;
}


void RFLock::unlock()
{
    LeaveCriticalSection(&m_cs);
}


RFReadWriteAccess::RFReadWriteAccess(RFLock* pLock)
    : m_pLock( pLock)
{
    if (m_pLock)
    {
        m_pLock->lock();
    }
}


RFReadWriteAccess::~RFReadWriteAccess()
{
    if (m_pLock)
    {
        m_pLock->unlock();
    }
}