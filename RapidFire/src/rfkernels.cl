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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Kernels to run CSC
// 
// const int4 vDim conatins the dimension of the image
//      vDim.x = width  of input image
//      vDim.y = height of input image
//      vDim.z = Aligned width  of output buffer
//      vDim.w = Aligned height of output buffer
//      Since the oitput of the CSC is sent to an encoder there might be restrictions on the 
//      image size. Those are represented in the aligned width and aligned height.
//
// const int isOpenGLObj
//      If true, the CSC will mirror the result.
//
// const int nTargetOrdering
//      Specifies the channel ordering. The following values are accepted.
//      0: RGBA
//      1: ARGB
//      2: BGRA
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////


__constant sampler_t imageSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

// The dimension is based on the chroma planes. Since we use a YUV 420 format the chroma plane
// is only 1/4 of the size of the Y Plane.
// E.g. a 1280x720 input image has a 640x360 dimension 
//
// Global work size is width/2, height/2. Each workitem computes 4 pixels
//

__kernel void rgbaTonv12_image2d(read_only image2d_t pIn, __global uchar * pOut, const int4 vDim, const int mirror)
{
    uint uiGlobalIdX = get_global_id(0);    // 0 - width /2
    uint uiGlobalIdY = get_global_id(1);    // 0 - height/2

    if (uiGlobalIdX >= vDim.x / 2 || uiGlobalIdY >= vDim.y / 2)
    {
        return;
    }

    // Calculate the offset into Y Plane and into the RGBA buffer. The Y Plane and the RGBA
    // buffer have the dimension width x height. The Kernel dimension is only (width/2) x (height / 2)
    // which corresponds to the chroma plane dimension. 
    // Each WorkItem needs to write 4 Luminance samples and one chroma sample.

    // Offset to the start of the chroma u-plane
    uint uPlaneOffset = vDim.z * vDim.y;

    // Offset into the Y Plane. The Y Plane contains 1 Byte data but since each work item computes
    // 2x2 pixels, the offset for the first 2 pixels is multiplied by 2
    uint uiYPlaneOffset = (uiGlobalIdX + uiGlobalIdY * vDim.z) << 1;

    // The FBO contains the pixel in inverted order. The pixel of the lower left corner is stored
    // at id 0, so we need to read the buffer starting at the top.

    uchar4 RGBA1, RGBA2, RGBA3, RGBA4;

    // Read (x,y)
    int2 pos = (int2)(uiGlobalIdX * 2, (mirror == 1) ? (vDim.y - 2 * uiGlobalIdY - 1) : 2 * uiGlobalIdY);
    RGBA1 = convert_uchar4_sat_rte(255 * read_imagef(pIn, imageSampler, pos));
    // Read (x+1, y)
    pos.x += 1;
    RGBA2 = convert_uchar4_sat_rte(255 * read_imagef(pIn, imageSampler, pos));

    // Switch to next row
    // Read (x, y+1)
    pos = (int2)(uiGlobalIdX * 2, (mirror == 1) ? (vDim.y - uiGlobalIdY * 2 - 2) : (uiGlobalIdY * 2 + 1));
    RGBA3 = convert_uchar4_sat_rte(255 * read_imagef(pIn, imageSampler, pos));
    // Read (x+1, y+1)
    pos.x += 1;
    RGBA4 = convert_uchar4_sat_rte(255 * read_imagef(pIn, imageSampler, pos));

    ushort4 RGBA = convert_ushort4(RGBA1) + convert_ushort4(RGBA2) + convert_ushort4(RGBA3) + convert_ushort4(RGBA4);

    // Take average color
    RGBA = RGBA >> 2;

    uchar Y1 = ((66 * RGBA1.x + 129 * RGBA1.y + 25 * RGBA1.z + 128) >> 8) + 16;
    uchar Y2 = ((66 * RGBA2.x + 129 * RGBA2.y + 25 * RGBA2.z + 128) >> 8) + 16;
    uchar Y3 = ((66 * RGBA3.x + 129 * RGBA3.y + 25 * RGBA3.z + 128) >> 8) + 16;
    uchar Y4 = ((66 * RGBA4.x + 129 * RGBA4.y + 25 * RGBA4.z + 128) >> 8) + 16;

    // Write Y Plane
    pOut[uiYPlaneOffset]     = Y1;
    pOut[uiYPlaneOffset + 1] = Y2;

    uiYPlaneOffset += vDim.z;

    pOut[uiYPlaneOffset]     = Y3;
    pOut[uiYPlaneOffset + 1] = Y4;

    // Write U Plane
    pOut[uPlaneOffset + 2 * uiGlobalIdX + uiGlobalIdY * vDim.z]     = ((-38 * RGBA.x - 74 * RGBA.y + 112 * RGBA.z + 128) >> 8) + 128;
    // Write V Plane
    pOut[uPlaneOffset + 2 * uiGlobalIdX + 1 + uiGlobalIdY * vDim.z] = ((112 * RGBA.x - 94 * RGBA.y - 18  * RGBA.z + 128) >> 8) + 128;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
// converts RGBA input buffer into NV12 output image planes. The input image is shiftev by vShift.
//
// rgbaIn:  Input image (CL_RGBA, CL_UNORM_INT8) containing pixel RGBA data. The color range is 0.0f..1.0f.
// yOut:    Luminance color plane (image-width   * image-height)      CL_R
// uvOut:   Chroma output planes  (image-width/2 * image-height/2)    CL_RG
//
// Number of work items: image-width/2 * image-height/2
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

__kernel void rgbaToNV12_Planes(__read_only image2d_t rgbaIn, __write_only image2d_t yOut, const int4 vDim, const int mirror, __write_only image2d_t uvOut)
{
    uint uiGlobalId_X = get_global_id(0);
    uint uiGlobalId_Y = get_global_id(1);

    if (uiGlobalId_X >= vDim.x / 2 || uiGlobalId_Y >= vDim.y / 2)
    {
        return;
    }

    // Offset into RGBA source buffer
    int2 SrcCoord = (int2)(uiGlobalId_X * 2, (mirror == 1) ? (vDim.y - 2 * uiGlobalId_Y - 1) : 2 * uiGlobalId_Y);
    int2 DstCoord = (int2)((uiGlobalId_X * 2), (uiGlobalId_Y * 2));

    float4 Y, U, V, R, G, B;

    float4 pixel = read_imagef(rgbaIn, imageSampler, SrcCoord);

    R.x = pixel.x;
    G.x = pixel.y;
    B.x = pixel.z;

    pixel = read_imagef(rgbaIn, imageSampler, (int2)(SrcCoord.x + 1, SrcCoord.y));

    R.y = pixel.x;
    G.y = pixel.y;
    B.y = pixel.z;


    SrcCoord.y = (mirror == 1) ? (vDim.y - uiGlobalId_Y * 2 - 2) : (uiGlobalId_Y * 2 + 1);

    pixel = read_imagef(rgbaIn, imageSampler, (int2)(SrcCoord.x, SrcCoord.y));

    R.z = pixel.x;
    G.z = pixel.y;
    B.z = pixel.z;

    pixel = read_imagef(rgbaIn, imageSampler, (int2)(SrcCoord.x + 1, SrcCoord.y));

    R.w = pixel.x;
    G.w = pixel.y;
    B.w = pixel.z;

    Y = (66.0f * R + 129.0f * G + 25.0f * B) + 16.0f;

    // Calculate the chroma values for each of the 4 pixels.
    U = (-38.0f * R - 74.0f * G + 112.0f * B) + (float4)(128.0f);
    V = (112.0f * R - 94.0f * G - 18.0f * B) + (float4)(128.0f);

    // Write Y plane. Y range is 0..255.
    write_imageui(yOut, DstCoord, (uint4)((uchar)(Y.x), 0, 0, 0));
    write_imageui(yOut, (int2)(DstCoord.x + 1, DstCoord.y), (uint4)((uchar)(Y.y), 0, 0, 0));
    write_imageui(yOut, (int2)(DstCoord.x, DstCoord.y + 1), (uint4)((uchar)(Y.z), 0, 0, 0));
    write_imageui(yOut, (int2)(DstCoord.x + 1, DstCoord.y + 1), (uint4)((uchar)(Y.w), 0, 0, 0));

    // Set image coordinates to be used to write the Chroma plane.
    DstCoord.x = uiGlobalId_X;
    DstCoord.y = uiGlobalId_Y;

    // Store the average of U and V interleaved in the uv iamge U,V range is 0..255.
    write_imageui(uvOut, DstCoord, convert_uint4_sat((uchar4)((U.x + U.y + U.z + U.w) / 4.0f, (V.x + V.y + V.z + V.w) / 4.0f, 0.0f, 0.0f)));
}


__kernel void rgbaToI420_image2d(read_only image2d_t pIn, __global uchar* pOutI420, const int4 vDim, const int mirror)
{
    uint uiGlobalIdX = get_global_id(0);    // 0 - width /2
    uint uiGlobalIdY = get_global_id(1);    // 0 - height/2

    if (uiGlobalIdX >= vDim.x / 2 || uiGlobalIdY >= vDim.y / 2)
    {
        return;
    }

    // Calculate the offset into Y Plane and into the RGBA buffer. The Y Plane and the RGBA
    // buffer have the dimension width x height. The Kernel dimension is only (width/2) x (height / 2)
    // which corresponds to the chroma plane dimension. 
    // Each WorkItem needs to write 4 luminance samples and one chroma sample. 

    // Offset into the Y Plane. The Y Plane contains 1 Byte data but since each work item computes
    // 2x2 pixels, the offset for the first 2 pixels is multiplied by 2
    uint uiYPlaneOffset = (uiGlobalIdX + uiGlobalIdY * vDim.z) << 1;

    // OpenGL FBO contains the pixel in inverted order. The pixel of the lower left corner is stored
    // at id 0, so we need to read the buffer starting at the top.

    uchar4 RGBA1, RGBA2, RGBA3, RGBA4;

    // Read (x,y)
    int2 pos = (int2)(uiGlobalIdX * 2, (mirror == 1) ? (vDim.y - uiGlobalIdY * 2 - 1) : uiGlobalIdY * 2);
    RGBA1 = convert_uchar4_sat_rte(255 * read_imagef(pIn, imageSampler, pos));
    // Read (x+1, y)
    pos.x += 1;
    RGBA2 = convert_uchar4_sat_rte(255 * read_imagef(pIn, imageSampler, pos));

    // Switch to next row
    // Read (x, y+1)
    pos = (int2)(uiGlobalIdX * 2, (mirror == 1) ? (vDim.y - uiGlobalIdY * 2 - 2) : (uiGlobalIdY * 2 + 1));
    RGBA3 = convert_uchar4_sat_rte(255 * read_imagef(pIn, imageSampler, pos));
    // Read (x+1, y+1)
    pos.x += 1;
    RGBA4 = convert_uchar4_sat_rte(255 * read_imagef(pIn, imageSampler, pos));

    ushort4 RGBA = convert_ushort4(RGBA1) + convert_ushort4(RGBA2) + convert_ushort4(RGBA3) + convert_ushort4(RGBA4);

    // Take average color
    RGBA = RGBA >> 2;

    uchar Y1 = ((66 * RGBA1.x + 129 * RGBA1.y + 25 * RGBA1.z + 128) >> 8) + 16;
    uchar Y2 = ((66 * RGBA2.x + 129 * RGBA2.y + 25 * RGBA2.z + 128) >> 8) + 16;
    uchar Y3 = ((66 * RGBA3.x + 129 * RGBA3.y + 25 * RGBA3.z + 128) >> 8) + 16;
    uchar Y4 = ((66 * RGBA4.x + 129 * RGBA4.y + 25 * RGBA4.z + 128) >> 8) + 16;

    // Write Y Plane
    pOutI420[uiYPlaneOffset] = Y1;
    pOutI420[uiYPlaneOffset + 1] = Y2;

    uiYPlaneOffset += vDim.z;

    pOutI420[uiYPlaneOffset] = Y3;
    pOutI420[uiYPlaneOffset + 1] = Y4;

    // Write U Plane
    __global uchar* pOutU = pOutI420 + vDim.z * vDim.w;
    __global uchar* pOutV = pOutU + vDim.z * vDim.w / 2;
    pOutU[uiGlobalIdX + uiGlobalIdY * vDim.z] = ((-38 * RGBA.x - 74 * RGBA.y + 112 * RGBA.z + 128) >> 8) + 128;
    // Write V Plane
    pOutV[uiGlobalIdX + uiGlobalIdY * vDim.z] = ((112 * RGBA.x - 94 * RGBA.y - 18 * RGBA.z + 128) >> 8) + 128;
}


__kernel void copy_rgba_image2d(__read_only image2d_t rgbaIn, __global uchar *rgbaOut, const int4 vDim, const int mirror, const int nTargetOrdering)
{
    uint uiGlobalId_X = get_global_id(0);
    uint uiGlobalId_Y = get_global_id(1);

    if (uiGlobalId_X >= vDim.x || uiGlobalId_Y >= vDim.y)
    {
        return;
    }

    int2 ImgCoord;

    if (mirror)
    {
        ImgCoord = (int2)(uiGlobalId_X, vDim.y - (uiGlobalId_Y + 1));
    }
    else
    {
        ImgCoord = (int2)(uiGlobalId_X, uiGlobalId_Y);
    }

    uint uiBufferOffset = (uiGlobalId_X + (uiGlobalId_Y * vDim.z)) * 4;

    uchar4 pixel = convert_uchar4_sat(255.0f * read_imagef(rgbaIn, imageSampler, ImgCoord));

    if (nTargetOrdering == 1)
    {
        // Write ARGB
        rgbaOut[uiBufferOffset]     = pixel.w;
        rgbaOut[uiBufferOffset + 1] = pixel.x;
        rgbaOut[uiBufferOffset + 2] = pixel.y;
        rgbaOut[uiBufferOffset + 3] = pixel.z;
    }
    else if (nTargetOrdering == 2)
    {
        // Write BGRA
        rgbaOut[uiBufferOffset]     = pixel.z;
        rgbaOut[uiBufferOffset + 1] = pixel.y;
        rgbaOut[uiBufferOffset + 2] = pixel.x;
        rgbaOut[uiBufferOffset + 3] = pixel.w;
    }
    else
    {
        // Write RGBA
        rgbaOut[uiBufferOffset]     = pixel.x;
        rgbaOut[uiBufferOffset + 1] = pixel.y;
        rgbaOut[uiBufferOffset + 2] = pixel.z;
        rgbaOut[uiBufferOffset + 3] = pixel.w;
    }
}