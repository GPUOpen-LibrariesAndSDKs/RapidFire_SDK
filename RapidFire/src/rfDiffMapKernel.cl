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

////////////////////////////////////////////////////////////////////////////////////////////////
// Kernel to compare two images and to create a diff map. Comparison is done blockwise
// if one pixel of the Image1 and Image2 inside a block differs the corresponding pixel
// in DiffMap is set to 1 otherwise to 0.
// Each work item compares a block of uiLocalPxX x uiLocalPxY pixels.
// 
// Global Work Size : (DomainSizeX / uiLocalPxX) x (DomainSizeY / uiLocalPxY)
// Local Work Size  : 16 x 16
//
// Image1: Linear buffer containing pixel information of first image.
// Image2: Lienar buffer containing pixel information of second image.
// DiffMap: Output buffer containing difference between the two images.
// DomainSizeX: Image width
// DomainSizeY: Image height
// uiLocalPxX: Number of pixels each work item compares in x direction
// uiLocalPxY: Number of pixels each work item compares in y direction
////////////////////////////////////////////////////////////////////////////////////////////////

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

__kernel void DiffMap_Image(__read_only image2d_t Image1, __read_only image2d_t Image2, __global unsigned char* DiffMap,
                            unsigned int DomainSizeX, unsigned int DomainSizeY, const unsigned int uiLocalPxX, const unsigned int uiLocalPxY)
{
    __local unsigned int result;
    result = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    short groupX = get_group_id(0);
    short groupY = get_group_id(1);
    short groupIndex = groupX + get_num_groups(0) * groupY;
    short groupSize = get_local_size(0) * get_local_size(1);
    short localIndex = get_local_id(0) + get_local_size(0) * get_local_id(1);

    // Offset into the image
    unsigned int x_offset = groupX * uiLocalPxX;
    unsigned int y_offset = groupY * uiLocalPxY;

    // Limit size to xDimension
    unsigned int localBlockSize = uiLocalPxX * uiLocalPxY;

    float4 pixels1;
    float4 pixels2;

    int2 pos = (int2)(x_offset, y_offset);

    for (; localIndex < localBlockSize; localIndex += groupSize)
    {
        if (result != 0)
        {
            return;
        }

        unsigned int x = localIndex % uiLocalPxX;
        unsigned int y = localIndex / uiLocalPxX;

        pixels1 = read_imagef(Image1, sampler, pos + (int2)(x, y));
        pixels2 = read_imagef(Image2, sampler, pos + (int2)(x, y));

        if (amd_sad4(as_uint4(pixels1), as_uint4(pixels2), 0) != 0)
        {
            result = 1;
            DiffMap[groupIndex] = 1;
            return;
        }
    }
};


__kernel void DiffMap_Buffer(__global unsigned int* Image1, __global unsigned int* Image2, __global unsigned char* DiffMap,
                             unsigned int DomainSizeX, unsigned int DomainSizeY, const unsigned int uiLocalPxX, const unsigned int uiLocalPxY)
{
    __local unsigned int result;
    result = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    short groupX = get_group_id(0);
    short groupY = get_group_id(1);
    short groupIndex = groupX + get_num_groups(0) * groupY;
    short groupSize = get_local_size(0) * get_local_size(1);
    short localIndex = get_local_id(0) + get_local_size(0) * get_local_id(1);

    // Offset into the image
    unsigned int x_offset = groupX * uiLocalPxX;
    unsigned int y_offset = groupY * uiLocalPxY;

    // Offset into linear buffer
    unsigned int idx = x_offset + (DomainSizeX * y_offset);

    // Limit size to xDimension
    unsigned int localBlockSize = uiLocalPxX * uiLocalPxY;

    uint4 pixels1;
    uint4 pixels2;

    for (; localIndex < localBlockSize; localIndex += 4 * groupSize)
    {
        if(result != 0)
        {
            return;
        }

        for (unsigned int i = 0; i < 4; ++i)
        {
            unsigned int localIndex_ = localIndex + i * groupSize;
            unsigned int x = localIndex_ % uiLocalPxX;
            unsigned int y = localIndex_ / uiLocalPxX;
            if (x_offset + x < DomainSizeX && y_offset + y < DomainSizeY)
            {
                ((unsigned int*)&(pixels1))[i] = Image1[idx + x + y * DomainSizeX];
                ((unsigned int*)&(pixels2))[i] = Image2[idx + x + y * DomainSizeX];
            }
            else
            {
                ((unsigned int*)&(pixels1))[i] = 0;
                ((unsigned int*)&(pixels2))[i] = 0;
            }
        }
        if (amd_sad4(pixels1, pixels2, 0) != 0)
        {
            result = 1;
            DiffMap[groupIndex] = 1;
            return;
        }
    }
};