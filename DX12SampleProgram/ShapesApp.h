#pragma once
#include "stdafx.h"
#include "D3DAppBase.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"

#ifndef IS_ENABLE_SHAPE_APP
#define IS_ENABLE_SHAPE_APP 1
#endif // !IS_ENABLE_SHAPE_APP

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;



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

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>  m_geometries;
    std::unordered_map<std::string, ComPtr<ID3DBlob>>    m_shaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>    m_PSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> m_allItems;

    // Render items divided by PSO.
    std::vector<RenderItem*> m_opaqueRenderItems;

    PassConstants m_mainPassCB;

    UINT m_passCbvOffset = 0;

    bool m_isWireFrame = false;

    XMFLOAT3 m_eyePos = { 0.0f,0.0f,0.0f };
    XMMATRIX m_view = DirectX::XMMatrixIdentity();
    XMMATRIX m_proj = DirectX::XMMatrixIdentity();

    float m_theta = 1.5f * XM_PI;
    float m_phi = 0.2f * XM_PI;
    float m_radius = 5.0f;

    POINT m_lastMousePos;

    XMMATRIX m_viewProj;
    XMVECTOR result;
    XMFLOAT4 result2;
};
#endif