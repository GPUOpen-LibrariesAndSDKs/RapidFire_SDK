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

//
//  Generates RGB image (SGI Image File Format) file for the specified data values.
//

#include <stdlib.h>
#include <string.h>

#include <cstdio>
#include <fstream>

using namespace std;


void putbyte(ofstream& outf, unsigned char val)
{
    unsigned char buf[1];

    buf[0] = val;

    outf.write(reinterpret_cast<char*>(buf), sizeof(buf));
}


void putshort(ofstream& outf, unsigned short val)
{
    unsigned char buf[2];

    buf[0] = static_cast<unsigned char>(val >> 8);
    buf[1] = static_cast<unsigned char>(val >> 0);

    outf.write(reinterpret_cast<char*>(buf), sizeof(buf));
}


void putlong(ofstream& outf, unsigned long val)
{
    unsigned char buf[4];

    buf[0] = static_cast<unsigned char>(val >> 24);
    buf[1] = static_cast<unsigned char>(val >> 16);
    buf[2] = static_cast<unsigned char>(val >> 8);
    buf[3] = static_cast<unsigned char>(val >> 0);

    outf.write(reinterpret_cast<char*>(buf), sizeof(buf));
}


static void getbyte(ifstream& inf, unsigned char* val)
{
    if (inf.is_open() && val)
    {
        inf.read(reinterpret_cast<char*>(val), 1);
    }
}


static void getshort(ifstream& inf, unsigned short* val)
{
    if (inf.is_open() && val)
    {
        unsigned char buf[2];

        inf.read(reinterpret_cast<char*>(buf), 2);

        (*val) = static_cast<unsigned short>(buf[0]);
        (*val) = (*val) << 8;
        (*val) |= static_cast<unsigned short>(buf[1]);
    }
}


static void getlong(ifstream& inf, unsigned long* val)
{
    if (inf.is_open() && val)
    {
        unsigned char buf[4];

        inf.read(reinterpret_cast<char*>(buf), 4);

        (*val) = static_cast<unsigned short>(buf[0]);
        (*val) = (*val) << 24;
        (*val) |= static_cast<unsigned short>(buf[1]);
        (*val) = (*val) << 16;
        (*val) = static_cast<unsigned short>(buf[2]);
        (*val) = (*val) << 8;
        (*val) |= static_cast<unsigned short>(buf[3]);
    }
}


static void writeHeader(ofstream& outf, int xsize, int ysize, short channels)
{
    putshort(outf, 474);       //  MAGIC  

    putbyte(outf, 0);          //  STORAGE is VERBATIM 

    putbyte(outf, 1);          //  BPC is 1            

    putshort(outf, 2);         //  DIMENSION is 2      

    putshort(outf, xsize);     //  XSIZE      

    putshort(outf, ysize);     //  YSIZE      

    putshort(outf, channels);  //  Channels   

    putlong(outf, 0);          //  PIXMIN is 0   

    putlong(outf, 255);        //  PIXMAX is 255  

    for (int i = 0; i < 4; ++i)   //   DUMMY 4 bytes
    {
        putbyte(outf, 0);
    }

    outf.write("No Name", 80);   // IMAGENAME  

    putlong(outf, 0);            //  COLORMAP is 0   

    for (int i = 0; i < 404; ++i)   //  DUMMY 404 bytes     
    {
        putbyte(outf, 0);
    }
}


// function to generate rgb file from the buffer data
extern void writediffmap(unsigned char* outbuf, int xsize, int ysize, const char* fname)
{
    ofstream of(fname, ios::out | ios::binary);

    if (!of.is_open())
    {
        fprintf(stderr, "sgiimage: can't open output file\n");
        exit(-1);
    }

    writeHeader(of, xsize, ysize, 3);

    for (int z = 0; z < 3; ++z)
    {
        for (int y = 0; y < ysize; ++y)
        {
            for (int x = 0; x < xsize; ++x)
            {
                of.put(static_cast<char>(outbuf[(y * xsize) + x]) * 255);
            }
        }
    }

    of.close();
}


// function to generate rgb file from the buffer data
extern void writeimage(unsigned char* outbuf, int xsize, int ysize, const int* pChannelOrder, const char* fname)
{
    ofstream of(fname, ios::out | ios::binary);

    if (!of.is_open())
    {
        fprintf(stderr, "sgiimage: can't open output file\n");
        exit(-1);
    }

    writeHeader(of, xsize, ysize, 4);

    for (int z = 0; z < 4; ++z)
    {
        for (int y = 0; y < ysize; ++y)
        {
            for (int x = 0; x < xsize; ++x)
            {
                of.put(static_cast<char>(outbuf[(((4 * y * xsize) + 4 * x)) + pChannelOrder[z]]));
            }
        }
    }

    of.close();
}


// function to generate rgb file from the buffer data
extern void writeRGBAimage(unsigned char* outbuf, int xsize, int ysize, const char* fname)
{
    // writeimage writes an rgba image. 
    // The channel order maps the source channels to the RGBA.
    const int pChanOrder[4] = { 0, 1, 2, 3 };

    writeimage(outbuf, xsize, ysize, pChanOrder, fname);
}


// function to generate rgb file from the buffer data
extern void writeARGBimage(unsigned char* outbuf, int xsize, int ysize, const char* fname)
{
    const int pChanOrder[4] = { 1, 2, 3, 0 };

    writeimage(outbuf, xsize, ysize, pChanOrder, fname);
}


// function to generate rgb file from the buffer data
extern void writeBGRAimage(unsigned char* outbuf, int xsize, int ysize, const char* fname)
{
    const int pChanOrder[4] = { 2, 1, 0, 3 };

    writeimage(outbuf, xsize, ysize, pChanOrder, fname);
}


extern char* readRGBimage(int &xsize, int &ysize, int &channels, const char* fname)
{
    ifstream imageFile;

    imageFile.open(fname, ios::in | ios::binary);

    if (!imageFile.is_open())
    {
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Read Header
    //
    //    2 bytes | short  | MAGIC     | IRIS image file magic number
    //    1 byte  | char   | STORAGE   | Storage format
    //    1 byte  | char   | BPC       | Number of bytes per pixel channel
    //    2 bytes | ushort | DIMENSION | Number of dimensions
    //    2 bytes | ushort | XSIZE     | X size in pixels
    //    2 bytes | ushort | YSIZE     | Y size in pixels
    //    2 bytes | ushort | ZSIZE     | Number of channels
    //    4 bytes | long   | PIXMIN    | Minimum pixel value
    //    4 bytes | long   | PIXMAX    | Maximum pixel value
    //    4 bytes | char   | DUMMY     | Ignored
    //   80 bytes | char   | IMAGENAME | Image name
    //    4 bytes | long   | COLORMAP  | Colormap ID
    //  404 bytes | char   | DUMMY     | Ignored
    //
    ///////////////////////////////////////////////////////////////////////////////
    unsigned char  ucBuffer;
    unsigned short usBuffer;
    unsigned long  ulBuffer;

    getshort(imageFile, &usBuffer);
    if (usBuffer != 474)
    {
        return nullptr;
    }

    getbyte(imageFile, &ucBuffer);
    if (ucBuffer != 0)
    {
        return nullptr;
    }

    getbyte(imageFile, &ucBuffer);
    if (ucBuffer != 1)
    {
        return nullptr;
    }

    getshort(imageFile, &usBuffer);
    if (ucBuffer != 1)
    {
        return nullptr;
    }

    getshort(imageFile, &usBuffer);
    xsize = usBuffer;

    getshort(imageFile, &usBuffer);
    ysize = usBuffer;

    getshort(imageFile, &usBuffer);
    channels = usBuffer;

    getlong(imageFile, &ulBuffer);
    if (ulBuffer != 0)
    {
        return nullptr;
    }

    getlong(imageFile, &ulBuffer);
    if (ulBuffer != 255)
    {
        return nullptr;
    }

    // Ignore the next 492 bytes
    for (unsigned int i = 0; i < 492; ++i)
    {
        getbyte(imageFile, &ucBuffer);
    }

    char* pPixels = new char[xsize*ysize*channels];

    for (int z = 0; z < channels; ++z)
    {
        for (int y = 0; y < ysize; ++y)
        {
            for (int x = 0; x < xsize; ++x)
            {
                char* pPtr = &(pPixels[y*xsize*channels + x*channels + z]);

                getbyte(imageFile, reinterpret_cast<unsigned char*>(pPtr));
            }
        }
    }

    return pPixels;
}