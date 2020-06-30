#include "stdafx.h"
#include "BoxApp.h"

BoxApp::BoxApp(HINSTANCE hInstance):
    D3DAppBase(hInstance)
{

}

BoxApp::~BoxApp()
{

}

void BoxApp::BuildConstantDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
    m_objectCB = std::make_unique<UploadBuffer<ObjectConstants>>(m_device.Get(), 1, true);
    UINT objectConstantBufferByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_objectCB->Resource()->GetGPUVirtualAddress();

    UINT boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objectConstantBufferByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = CalculateConstantBufferByteSize(sizeof(ObjectConstants));
    m_device->CreateConstantBufferView(&cbvDesc,
        m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

bool BoxApp::Initialize()
{
    if (!D3DAppBase::Initialize())
    {
        return false;
    }

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
}