#pragma once
#include "stdafx.h"
#include "D3DAppBase.h"
#include "GeometryGenerator.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "Waves.h"

#ifndef IS_ENABLE_LAND_APP
#define IS_ENABLE_LAND_APP 0
#endif // !IS_ENABLE_LAND_APP


class LandAndWavesApp :public D3DAppBase
{
public:
    LandAndWavesApp(HINSTANCE hInstance);
    LandAndWavesApp(const LandAndWavesApp& rhs) = delete;
    LandAndWavesApp& operator=(const LandAndWavesApp& rhs) = delete;
    ~LandAndWavesApp();

    virtual bool Initialize() override;

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

    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildLandGeometry();
    void BuildWaveGeometryBuffers();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& RenderItems);

    float GetHillsHeight(float x, float z)const;
    DirectX::XMFLOAT3 GetHillsNormal(float x, float z)const;

private:
    std::vector<std::unique_ptr<FrameResource>> m_frameResources;
    FrameResource* m_currentFrameResource = nullptr;
    UINT m_currentFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>  m_geometries;
    std::unordered_map<std::string, ComPtr<ID3DBlob>>   m_shaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>    m_PSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC>   m_inputLayout;

    RenderItem* m_waveRenderItem = nullptr;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>>    m_allRenderItems;

    // Render items divided by PSO.
    std::vector<RenderItem*>    m_renderItemLayer[(int)RenderLayer::Count];

    std::unique_ptr<Waves>  m_waves;

    PassConstants m_mainPassConstantBuffer;

    bool m_isWireFrame = false;

    DirectX::XMFLOAT3 m_cameraPos = { 0.0f,0.0f,0.0f };
    DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_proj = DirectX::XMMatrixIdentity();

    float m_cameraTheta = 1.5f * DirectX::XM_PI;
    float m_cameraPhi = DirectX::XM_PIDIV2 - 0.1f;
    float m_cameraRadius = 50.f;

    float m_sunTheta = 1.25f * DirectX::XM_PI;
    float m_sunPhi = DirectX::XM_PIDIV4;

    POINT m_lastMousePos = { (LONG)(0),(LONG)0 };
};