#pragma once

#include <core/Buffer.h>
#include <components/Component.h>

#include "RFContext.h"

class RFContextAMF : public RFContextCL
{
public:

    RFContextAMF();
    ~RFContextAMF();

    // Creates OpenCL context.
    virtual RFStatus    createContext() override;

    // Creates OpenCL context based on an existing OpenGL context.
    virtual RFStatus    createContext(DeviceCtx hDC, GraphicsCtx hGLRC) override;
    // Creates OpenCL context based on an existing D3D9 Device.
    virtual RFStatus    createContext(ID3D11Device*         pD3DDevice) override;
    // Creates OpenCL context based on an existing D3D9 Ex Device.
    virtual RFStatus    createContext(IDirect3DDevice9*     pD3DDevice) override;
    // Creates OpenCL context based on an existing D3D11 Device.
    virtual RFStatus    createContext(IDirect3DDevice9Ex* pD3DDeviceEx) override;

    // Registers DX9 texture. DX9 is only supported with AMF.
    virtual RFStatus    setInputTexture(IDirect3DSurface9* pD3D9Texture, const unsigned int uiWidth, unsigned int uiHeight, unsigned int& idx) override;

    virtual RFStatus    createBuffers(RFFormat format, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiAlignedWidth, unsigned int uiAlignedHeight, bool bUseAsyncCopy) override;

    virtual RFStatus    processBuffer(bool bInvert, unsigned int uiSorceIdx, unsigned int uiDestIdx) override;

    amf::AMFContextPtr  getAMFContext() const { return m_amfContext; };

    amf::AMFSurfacePtr  getAMFSurface(unsigned int uiIdx) const;

    amf::AMF_SURFACE_FORMAT getAMFSurfaceFormat() const { return m_amfFormat; };

private:

    // Performs common actions like compiling kernels, creating dma queue to finish context creation.
    RFStatus             finalizeContext();

    // Returns a pointer to the native image plane of an AMF surface.
    void*                getImageBuffer(unsigned int uiBuffer, unsigned int uiPlaneId);

    unsigned int                    m_uiPlaneCount;

    amf::AMFContextPtr              m_amfContext;
    amf::AMF_SURFACE_FORMAT         m_amfFormat;
    amf::AMFSurfacePtr*             m_pSurfaceList;
    amf::AMF_MEMORY_TYPE            m_amfMemory;

    IDirect3DSurface9*              m_pD3D9Surfaces[MAX_NUM_RENDER_TARGETS];
};