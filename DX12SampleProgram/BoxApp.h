#pragma once
#include "stdafx.h"
#include "D3DAppBase.h"
#include "UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct ObjectConstants
{
    XMMATRIX WorldViewProj = XMMatrixIdentity();
};

class BoxApp :public D3DAppBase
{
public:
    BoxApp(HINSTANCE hInstance);
    BoxApp(const BoxApp& rhs) = delete;
    BoxApp& operator=(const BoxApp& rhs) = delete;
    ~BoxApp();

    virtual bool Initialize() override;

private:
    virtual void OnResize() override;
    virtual void Update(const GameTimer&gt) override;
    virtual void Draw(const GameTimer& gt)override;

    // Handle mouse movement.
    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void BuildConstantDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

private:
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;
    ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;

    std::unique_ptr<UploadBuffer<ObjectConstants>>  m_objectCB = nullptr;

    std::unique_ptr<MeshGeometry> m_boxGeo = nullptr;

    ComPtr<ID3DBlob> m_vertexShaderByteCode = nullptr;
    ComPtr<ID3DBlob> m_pixelShaderByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

    ComPtr<ID3D12PipelineState> m_PSO = nullptr;

    XMMATRIX m_world = XMMatrixIdentity();
    XMMATRIX m_view = XMMatrixIdentity();
    XMMATRIX m_proj = XMMatrixIdentity();

    float m_theta = 1.5f * XM_PI;
    float m_phi = XM_PIDIV4;
    float m_radius = 5.0f;

    POINT m_lastMousePos;
};