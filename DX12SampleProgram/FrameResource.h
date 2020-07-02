#pragma once
#include "stdafx.h"
#include "UploadBuffer.h"

struct ObjectConstants
{
    DirectX::XMMATRIX World = DirectX::XMMatrixIdentity();
};

struct PassConstants
{
    DirectX::XMMATRIX View = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX InvView = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX Proj = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX InvProj = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX ViewProj = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX InvViewProj = DirectX::XMMatrixIdentity();
    DirectX::XMFLOAT3 EyePosW = { 0.0f,0.0f,0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f,0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f,0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

// Store the resources needed for the CPU to build the command lists for a frame.
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator = nullptr;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it. So each frame needs their own cbuffers.
    std::unique_ptr<UploadBuffer<PassConstants>> m_passCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>>  m_objCB = nullptr;

    // Fence value to mark commands up to this fence point.
    // This lets us check if these frame resources are still in use by the GPU.
    UINT64 m_fence = 0;
};