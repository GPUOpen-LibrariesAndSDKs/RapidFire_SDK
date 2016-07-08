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

#ifndef __AMFPlatform_h__
#define __AMFPlatform_h__
#pragma once

//----------------------------------------------------------------------------------------------
// export declaration
//----------------------------------------------------------------------------------------------
#ifdef _WIN32
    #if defined(AMF_CORE_STATIC)
        #define AMF_CORE_LINK
    #else
        #if defined(AMF_CORE_EXPORTS)
            #define AMF_CORE_LINK __declspec(dllexport)
        #else
            #define AMF_CORE_LINK __declspec(dllimport)
        #endif
    #endif
#else // #ifdef _WIN32
    #define AMF_CORE_LINK
#endif // #ifdef _WIN32

#define AMF_MACRO_STRING2(x) #x
#define AMF_MACRO_STRING(x) AMF_MACRO_STRING2(x)

#define AMF_TODO(_todo) (__FILE__ "(" AMF_MACRO_STRING(__LINE__) "): TODO: "_todo)

#include <stdio.h>
#include <cstdint>

#if defined(_WIN32)


#ifndef NOMINMAX
#define NOMINMAX
#endif
    #define AMF_STD_CALL            __stdcall
    #define AMF_CDECL_CALL          __cdecl
    #define AMF_FAST_CALL           __fastcall
    #define AMF_INLINE              inline
    #define AMF_FORCEINLINE         __forceinline
    #define AMF_NO_VTABLE           __declspec(novtable)

    #define AMFPRId64   "I64d"
    #define LPRId64    L"I64d"

    #define AMFPRIx64   "I64x"
    #define LPRIx64    L"I64x"

#else // !WIN32 - Linux and Mac

    #define AMF_STD_CALL
    #define AMF_CDECL_CALL
    #define AMF_FAST_CALL
    #define AMF_INLINE              __inline__
    #define AMF_FORCEINLINE         __inline__
    #define AMF_NO_VTABLE           

    #if !defined(AMFPRId64)
        #if defined(__x86_64)
            #define AMFPRId64    "ld"
            #define LPRId64     L"lld"
            #define AMFPRIx64    "lx"
            #define LPRIx64     L"llx"
        #else
            #define AMFPRId64    "lld"
            #define LPRId64     L"lld"
            #define AMFPRIx64    "llx"
            #define LPRIx64     L"llx"
        #endif
    #endif

#endif // WIN32


#if defined(_MSC_VER)
#define AMF_WEAK __declspec( selectany ) 
#else //GCC
#define AMF_WEAK attribute((weak))
#endif

//-------------------------------------------------------------------------------------------------
// basic data types
//-------------------------------------------------------------------------------------------------
typedef     int64_t             amf_int64;
typedef     int32_t             amf_int32;
typedef     int16_t             amf_int16;
typedef     int8_t              amf_int8;

typedef     uint64_t            amf_uint64;
typedef     uint32_t            amf_uint32;
typedef     uint16_t            amf_uint16;
typedef     uint8_t             amf_uint8;
typedef     size_t              amf_size;

typedef     void*               amf_handle;
typedef     double              amf_double;
typedef     float               amf_float;

typedef     void                amf_void;
typedef     bool                amf_bool;
typedef     long                amf_long; 
typedef     int                 amf_int; 
typedef     unsigned long       amf_ulong; 
typedef     unsigned int        amf_uint; 

typedef     amf_int64           amf_pts;     // in 100 nanosecs

#define AMF_SECOND          10000000L    // 1 second in 100 nanoseconds

#define AMF_MIN(a, b) ((a) < (b) ? (a) : (b))
#define AMF_MAX(a, b) ((a) > (b) ? (a) : (b))

#if defined(_WIN32)
    #define PATH_SEPARATOR_WSTR         L"\\"
    #define PATH_SEPARATOR_WCHAR        L'\\'
#elif defined(__linux) // Linux
    #define PATH_SEPARATOR_WSTR          L"/"
    #define PATH_SEPARATOR_WCHAR         L'/'
#endif

struct AMFRect
{
    amf_int32 left;
    amf_int32 top;
    amf_int32 right;
    amf_int32 bottom;

    bool operator==(const AMFRect& other) const
    {
         return left == other.left && top == other.top && right == other.right && bottom == other.bottom; 
    }
    amf_int32 Width() const { return right - left; }
    amf_int32 Height() const { return bottom - top; }
};

inline AMFRect AMFConstructRect(amf_int32 left, amf_int32 top, amf_int32 right, amf_int32 bottom)
{
    AMFRect object = {left, top, right, bottom};
    return object;
}

struct AMFSize
{
    amf_int32 width;
    amf_int32 height;
    bool operator==(const AMFSize& other) const
    {
         return width == other.width && height == other.height; 
    }
};

inline AMFSize AMFConstructSize(amf_int32 width, amf_int32 height)
{
    AMFSize object = {width, height};
    return object;
}

struct AMFPoint
{
    amf_int32 x;
    amf_int32 y;
    bool operator==(const AMFPoint& other) const
    {
         return x == other.x && y == other.y; 
    }
};

inline AMFPoint AMFConstructPoint(amf_int32 x, amf_int32 y)
{
    AMFPoint object = {x, y};
    return object;
}

struct AMFRate
{
    amf_uint32 num;
    amf_uint32 den;
    bool operator==(const AMFRate& other) const
    {
         return num == other.num && den == other.den; 
    }
};

inline AMFRate AMFConstructRate(amf_int32 num, amf_int32 den)
{
    AMFRate object = {num, den};
    return object;
}

struct AMFRatio
{
    amf_uint32 num;
    amf_uint32 den;
    bool operator==(const AMFRatio& other) const
    {
         return num == other.num && den == other.den; 
    }
};

inline AMFRatio AMFConstructRatio(amf_int32 num, amf_int32 den)
{
    AMFRatio object = {num, den};
    return object;
}

#pragma pack(push, 1)
#pragma warning( push )
#if defined(WIN32)
#pragma warning(disable : 4200)
#pragma warning(disable : 4201)
#endif
struct AMFColor
{
    union
    {
        struct
        {
            amf_uint8 r;
            amf_uint8 g;
            amf_uint8 b;
            amf_uint8 a;
        };
        amf_uint32 rgba;
    };
    bool operator==(const AMFColor& other) const
    {
         return r == other.r && g == other.g && b == other.b && a == other.a; 
    }
};
#pragma warning( pop )
#pragma pack(pop)


inline AMFColor AMFConstructColor(amf_uint8 r, amf_uint8 g, amf_uint8 b, amf_uint8 a)
{
    AMFColor object = {r, g, b, a};
    return object;
}

#if defined(_WIN32)
    #include <combaseapi.h>

    #if defined(__cplusplus)
    extern "C"
    {
    #endif
        // allocator
        inline void* AMF_CDECL_CALL amf_variant_alloc(amf_size count)
        {
            return CoTaskMemAlloc(count);
        }
        inline void AMF_CDECL_CALL amf_variant_free(void* ptr)
        {
            CoTaskMemFree(ptr);
        }
    #if defined(__cplusplus)
    }
    #endif

#else // defined(_WIN32)
    #if defined(__cplusplus)
    extern "C"
    {
    #endif
        // allocator
        inline void* AMF_CDECL_CALL amf_variant_alloc(amf_size count)
        {
            return malloc(count);
        }
        inline void AMF_CDECL_CALL amf_variant_free(void* ptr)
        {
            free(ptr);
        }
    #if defined(__cplusplus)
    }
    #endif
#endif // defined(_WIN32)

namespace amf
{
    struct AMFGuid
    {
        AMFGuid(amf_uint32 _data1, amf_uint16 _data2, amf_uint16 _data3,
                amf_uint8 _data41, amf_uint8 _data42, amf_uint8 _data43, amf_uint8 _data44,
                amf_uint8 _data45, amf_uint8 _data46, amf_uint8 _data47, amf_uint8 _data48)
            : data1 (_data1),
            data2 (_data2),
            data3 (_data3),
            data41(_data41),
            data42(_data42),
            data43(_data43),
            data44(_data44),
            data45(_data45),
            data46(_data46),
            data47(_data47),
            data48(_data48)
        {}
        amf_uint32 data1;
        amf_uint16 data2;
        amf_uint16 data3;
        amf_uint8 data41;
        amf_uint8 data42;
        amf_uint8 data43;
        amf_uint8 data44;
        amf_uint8 data45;
        amf_uint8 data46;
        amf_uint8 data47;
        amf_uint8 data48;
    };

    AMF_INLINE bool AMFCompareGUIDs(const AMFGuid& guid1, const AMFGuid& guid2)
    {
        return
            guid1.data1 == guid2.data1 &&
            guid1.data2 == guid2.data2 &&
            guid1.data3 == guid2.data3 &&
            guid1.data41 == guid2.data41 &&
            guid1.data42 == guid2.data42 &&
            guid1.data43 == guid2.data43 &&
            guid1.data44 == guid2.data44 &&
            guid1.data45 == guid2.data45 &&
            guid1.data46 == guid2.data46 &&
            guid1.data47 == guid2.data47 &&
            guid1.data48 == guid2.data48;
    }
}

#endif //#ifndef __AMFPlatform_h__
