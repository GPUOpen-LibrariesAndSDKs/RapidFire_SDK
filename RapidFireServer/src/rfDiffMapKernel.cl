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

__kernel void DiffMap_LocalreductionImage(__global unsigned int* Image1, __global unsigned int* Image2, __global unsigned char* DiffMap,
                                          unsigned int DomainSizeX, unsigned int DomainSizeY, const unsigned int uiLocalPxX, const unsigned int uiLocalPxY)
{
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
    unsigned int discard = 0;

    for (; localIndex < localBlockSize; localIndex += 4 * groupSize)
    {
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
        if (amd_sad4(pixels1, pixels2, discard) != 0)
        {
            DiffMap[groupIndex] = 1;
        }
    }
};