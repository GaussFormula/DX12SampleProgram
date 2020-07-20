#pragma once
#include "stdafx.h"
#include "D3DAppBase.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"

#ifndef IS_ENABLE_SHAPE_APP
#define IS_ENABLE_SHAPE_APP 0
#endif // !IS_ENABLE_SHAPE_APP

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const unsigned int gNumFrameResources = 3;

// Light weight structure stores parameters to draw a shape.
// This will vary from app-to-app.

class RenderItem
{
public:
    RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, whick defines the position, orientation,
    // and scale of the object in the world .
    XMMATRIX World = DirectX::XMMatrixIdentity();

    // Dirty flag indicating the object data has changed and we need to update
    // the constant buffer. Because we have an object cbuffer for each FrameResource
    // we have to apply the update for each FrameResource. Thus, when we modify object data
    // object data we should set NumFrameDirty = gNumFrameResources so that each
    // frame resource gets the updates.
    int NumFrameDirty = gNumFrameResources;

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

#if IS_ENABLE_SHAPE_APP
class ShapesApp :public D3DAppBase
{
public:
    ShapesApp(HINSTANCE hInstance);
    ShapesApp(const ShapesApp& rhs) = delete;
    ShapesApp& operator=(const ShapesApp& rhs) = delete;
    ~ShapesApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    // Handle mouse movement.
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void UpdateCamera(const GameTimer& gt);
    void UpdateObjectCBs(const GameTimer& gt);
    void UpdateMainPassCB(const GameTimer& gt);

    void BuildConstantDescriptorHeaps();
    void BuildConstantBufferAndViews();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& items);

private:
    std::vector<std::unique_ptr<FrameResource>> m_frameResources;
    FrameResource* m_currentFrameResource = nullptr;
    int m_currentFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;
};
#endif