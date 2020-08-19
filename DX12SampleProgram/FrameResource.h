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
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT waveVertexCount=256);
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

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it. So each frame needs their own cbuffers.
    std::unique_ptr<UploadBuffer<Vertex>> m_wavesVB = nullptr;

    // Fence value to mark commands up to this fence point.
    // This lets us check if these frame resources are still in use by the GPU.
    UINT64 m_fence = 0;
};

// Light weight structure stores parameters to draw a shape.
// This will vary from app-to-app.

class RenderItem
{
public:
    RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, whick defines the position, orientation,
    // and scale of the object in the world .
    DirectX::XMMATRIX World = DirectX::XMMatrixIdentity();

    // Dirty flag indicating the object data has changed and we need to update
    // the constant buffer. Because we have an object cbuffer for each FrameResource
    // we have to apply the update for each FrameResource. Thus, when we modify object data
    // object data we should set NumFrameDirty = gNumFrameResources so that each
    // frame resource gets the updates.
    UINT NumFrameDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    UINT ObjectConstantBufferIndex = -1;

    MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};