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

#pragma once

#include <queue>

#include <Windows.h>

// RFLock implements a critical section.
class RFLock
{
public:

    RFLock();
    virtual ~RFLock();

    virtual bool lock();

    virtual void unlock();

private:

    // Disable copy constructor.
    RFLock(const RFLock& lock);
    // Disable assignmnet operator.
    RFLock& operator= (const RFLock& rhs);

    CRITICAL_SECTION m_cs;
};


// Class to apply lock by simply creating an instance and unlock on destruction.
class RFReadWriteAccess
{
public:

    // Contructor acquires lock from pLock.
    RFReadWriteAccess(RFLock* pLock);
    // Destructor will release lock.
    ~RFReadWriteAccess();

private:

    RFLock*  m_pLock;

    // Disable default constructor.
    RFReadWriteAccess();
    // Disable assignment operator.
    RFReadWriteAccess& operator= (const RFReadWriteAccess& rhs);
    // Disable new operator.
    void* operator new  (size_t size);
    void* operator new[](size_t size);
};


template <class T>
class RFLockedQueue
{
public:

    inline size_t size()
    {
        RFReadWriteAccess qlock(&m_Lock);

        return m_queue.size();
    }

    inline void push(T elem)
    {
        RFReadWriteAccess qlock(&m_Lock);

        m_queue.push(elem);
    }

    inline T front()
    {
        RFReadWriteAccess qlock(&m_Lock);

        return m_queue.front();
    }

    inline T pop()
    {
        RFReadWriteAccess qlock(&m_Lock);

        T elem = m_queue.front();

        m_queue.pop();

        return elem;
    }

private:

    RFLock              m_Lock;
    std::queue<T>       m_queue;
};


class RFGLContextGuard
{
public:

    // Use this ctor to store a context that needs to be restored at the end of scope.
    RFGLContextGuard()
        : m_bContextBound(false)
        , m_hExtDC(NULL)
        , m_hExtGlrc(NULL)
        , m_hDC(NULL)
        , m_hGlrc(NULL)
    {
        // Save a context that might be current.
        m_hExtDC   = wglGetCurrentDC();
        m_hExtGlrc = wglGetCurrentContext();

        m_bContextBound = (m_hExtGlrc != NULL);
    }

    // Use this ctor to bind a context and restore previously bound at end of scope.
    RFGLContextGuard(HDC hDC, HGLRC hGlrc)
        : m_bContextBound(false)
        , m_hExtDC(NULL)
        , m_hExtGlrc(NULL)
        , m_hDC(hDC)
        , m_hGlrc(hGlrc)
    {
        // Save a context that might be current.
        m_hExtDC  = wglGetCurrentDC();
        m_hExtGlrc = wglGetCurrentContext();

        if (m_hDC && m_hGlrc)
        {
            // Make the new context current.
            m_bContextBound = wglMakeCurrent(m_hDC, m_hGlrc);
        }
    }

    ~RFGLContextGuard()
    {
        // Release own context and restore previous context. If no extern context exists
        // the below call will be wglMakeCurrent(NULL, NULL) which just releases the ctx.
        wglMakeCurrent(m_hExtDC, m_hExtGlrc);
    }

    bool isContextBound() const
    {
        return (m_bContextBound != 0);
    };

private:

    // Disable copy ctor.
    RFGLContextGuard(const RFGLContextGuard& other);

    // Disable assignment op.
    RFGLContextGuard& operator=(const RFGLContextGuard& other);

    BOOL            m_bContextBound;

    HDC             m_hExtDC;
    HGLRC           m_hExtGlrc;

    const HDC       m_hDC;
    const HGLRC     m_hGlrc;
};