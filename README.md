# RapidFire SDK

The RapidFire SDK provides an interface for cloud gaming and virtual desktop applications to reduce the encoding latency by utilizing AMD Radeon&trade; Pro and FirePro&trade; GPUs. OpenGL&reg; 4.2, DirectX&reg; 9 and DirectX&reg; 11 textures as well as desktop capturing can be used as input to generate H.264, H265 or uncompressed stream output. While the encoding is handled entirely by RapidFire, the developer keeps full control over the configuration of the stream with the ability to adjust key encoding parameters such as resolution, frame rate and bit rate control throughout the streaming process.

### Prerequisites
* AMD Radeon&trade; Pro or FirePro&trade; GCN-based GPU
* AMD Radeon&trade; Pro GPU for H.265 encoding
* AMD Radeon&trade; Pro driver version 17.30.1041 or newer
* Windows&reg; 7 or Windows&reg; 10
* Building the SDK and samples requires Visual Studio&reg; 2013, Visual Studio&reg; 2015 or Visual Studio&reg; 2017
* Building the SDK requires the AMD APP SDK

### Getting Started
* A Visual Studio&reg; solution for the samples can be found in the `Samples` directory.
* Additional documentation can be found in the `doc` directory.

### License
RapidFire is licensed under the MIT license. See LICENSE file for full license information.

### Copyright information

##### DirectX, Visual Studio and Windows
DirectX&reg;, Visual Studio&reg;, and Windows&reg; are registered trademarks of Microsoft Corporation in the United States and/or other countries.

##### OpenGL
OpenGL&reg; and the oval logo are trademarks or registered trademarks of Silicon Graphics, Inc. in the United States and/or other countries worldwide.

##### OpenCL
OpenCL&trade; and the OpenCL logo are trademarks of Apple Inc. used by permission by Khronos.

##### glew
The OpenGL Extension Wrangler Library Copyright (C) 2008-2015, Nigel Stewart Copyright (C) 2002-2008, Milan Ikits Copyright (C) 2002-2008, Marcelo E. Magallon Copyright (C) 2002, Lev Povalahev All rights reserved.