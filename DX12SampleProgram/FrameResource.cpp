#include "stdafx.h"
#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount,UINT waveVertexCount,UINT materialCount)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&m_commandAllocator)
    ));
    m_passCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    m_objCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
    m_wavesVB = std::make_unique<UploadBuffer<Vertex>>(device, waveVertexCount, false);
    m_materialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);;
}

FrameResource::~FrameResource()
{

}