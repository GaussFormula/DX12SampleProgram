// Use arrow keys to move light positions.
#pragma once
#include "stdafx.h"
#include "D3DAppBase.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "Waves.h"
#include "FrameResource.h"

#ifndef IS_ENABLE_LITLAND_APP
#define IS_ENABLE_LITLAND_APP 0
#endif // !IS_ENABLE_LITLAND_APP


class LitWavesApp :public D3DAppBase
{
public:
    LitWavesApp(HINSTANCE hInstance);
    LitWavesApp(const LitWavesApp& rhs) = delete;
    LitWavesApp& operator=(const LitWavesApp& rhs) = delete;
    ~LitWavesApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void UpdateCamera(const GameTimer& gt);
    void UpdateObjectConstantBuffers(const GameTimer& gt);
    void UpdateMainPassConstantBuffer(const GameTimer& gt);
    void UpdateWaves(const GameTimer& gt);
    void UpdateMaterialConstantBuffers(const GameTimer& gt);

    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildLandGeometry();
    void BuildWavesGeometryBuffers();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems);

    float GetHillsHeight(float x, float z)const;
    DirectX::XMFLOAT3 GetHillsNormal(float x, float z)const;

private:
    std::vector<std::unique_ptr<FrameResource>> m_frameResources;
    FrameResource* m_currentFrameResource = nullptr;
    int m_currentFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>  m_geometries;
    std::unordered_map<std::string, std::unique_ptr<Material>>   m_materials;
    std::unordered_map<std::string, std::unique_ptr<Texture>>   m_textures;
    std::unordered_map<std::string, ComPtr<ID3DBlob>>   m_shaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

    RenderItem* m_wavesItem = nullptr;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>>    m_allRenderItems;
    
    // Render items divided by PSO.
    std::vector<RenderItem*>    m_renderItemLayer[(int)RenderLayer::Count];

    std::unique_ptr<Waves> m_waves = nullptr;

    PassConstants m_mainPassCB;

    DirectX::XMFLOAT3 m_cameraPos = { 0.0f,0.0f,0.0f };
    DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_proj = DirectX::XMMatrixIdentity();

    // theta is angle of vector with Y axis.
    // phi is angle of vector with x axis.
    float m_theta = 1.5f * DirectX::XM_PI;
    float m_phi = DirectX::XM_PIDIV2 - 0.1f;
    float m_radius = 50.0f;

    float m_sunTheta = 1.25f * DirectX::XM_PI;
    float m_sunPhi = DirectX::XM_PIDIV4;

    POINT m_lastMousePos = { LONG(0),LONG(0) };
};