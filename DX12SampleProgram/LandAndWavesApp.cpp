#include "stdafx.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

#ifndef IS_ENABLE_LAND_APP
#define IS_ENABLE_LAND_APP 0
#endif // !IS_ENABLE_LAND_APP

#if IS_ENABLE_LAND_APP
#include "LandAndWavesApp.h"

LandAndWavesApp::LandAndWavesApp(HINSTANCE hInstance)
    :D3DAppBase(hInstance)
{}

LandAndWavesApp::~LandAndWavesApp()
{
    if (m_device != nullptr)
    {
        FlushCommandQueue();
    }
}

void LandAndWavesApp::BuildRootSignature()
{
    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];
    ZeroMemory(slotRootParameter, sizeof(CD3DX12_ROOT_PARAMETER) * 2);

    // Create root CBV.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);

    // A root signature can be an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // Create a root signature with a single slot which points to a descriptor range consisting of a single constant 
    // buffer.
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(m_device->CreateRootSignature(
        0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    ));
}

void LandAndWavesApp::BuildShadersAndInputLayout()
{
#if defined(_DEBUG)|(DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    ThrowIfFailed(D3DCompileFromFile(L"LandAndWaves.hlsl", nullptr, nullptr, 
        "VSMain", "vs_5_0", compileFlags, 0, &m_shaders["standardVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"LandAndWaves.hlsl", nullptr, nullptr,
        "PSMain", "ps_5_0", compileFlags, 0, &m_shaders["opaquePS"], nullptr
    ));
    m_inputLayout =
    {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };
}

float LandAndWavesApp::GetHillsHeight(float x, float z) const
{
    return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 LandAndWavesApp::GetHillsNormal(float x, float z)const
{
    // n=(-df/dx,1,-df/dz)
    XMFLOAT3 n(
        -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
        1.0f,
        -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z)
    );

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);
    return n;
}

void LandAndWavesApp::BuildLandGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

    // Extract the vertex elements we are interested and apply the height function to 
    // each vertex. In addition, color the vertices based on their height so we have sandy looking 
    // beaches, grassy low hills, and snow mountain peaks.
    const UINT numGridVertices = grid.Vertices.size();
    std::vector<Vertex> vertices(numGridVertices);
    for (size_t i = 0; i < numGridVertices; ++i)
    {
        XMFLOAT3& p = grid.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Pos.y = GetHillsHeight(p.x, p.z);

        // Color the vertex based on its height.
        if (vertices[i].Pos.y < -10.0f)
        {
            // Sandy beach color.
            vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
        }
        else if (vertices[i].Pos.y<5.0f)
        {
            // Light yellow-green.
            vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
        }
        else if (vertices[i].Pos.y < 12.0f)
        {
            // Dark yellow green.
            vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
        }
        else if (vertices[i].Pos.y < 20.0f)
        {
            // Dark brown.
            vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
        }
        else
        {
            // White snow.
            vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    std::vector<std::uint16_t> indices = grid.GetIndices16();
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
    geo->Name = "landGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = CreateDefaultBuffer(m_device.Get(), m_commandList.Get(), 
        vertices.data(), 
        vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
        indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry subMesh;
    subMesh.BaseVertexLocation = 0;
    subMesh.StartIndexCount = 0;
    subMesh.IndexCount = (UINT)indices.size();

    geo->DrawArags["grid"] = subMesh;

    m_geometries["landGeo"] = std::move(geo);
}

void LandAndWavesApp::BuildWaveGeometryBuffers()
{
    std::vector<std::uint16_t> indices(3 * m_waves->GetTriangleCount());
    assert(m_waves->GetVertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = m_waves->GetRowCount();
    int n = m_waves->GetColumnCount();
    UINT64 k = 0;
    for (int i = 0; i < m - 1; ++i)
    {
        for (int j = 0; j < n - 1; ++j)
        {
            indices[k] = i * n + j;
            indices[k + 1] = i * n + j + 1;
            indices[k + 2] = (i + 1) * n + j;

            indices[k + 3] = (i + 1) * n + j;
            indices[k + 4] = i * n + j + 1;
            indices[k + 5] = (i + 1) * n + j + 1;
            k += 6;// Next quad.
        }
    }

    UINT vbByteSize = m_waves->GetVertexCount() * sizeof(Vertex);
    UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

    std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
    geo->Name = "waterGeo";

    // Set dynamically.
    geo->VertexBufferCPU = nullptr;
    geo->VertexBufferGPU = nullptr;

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->IndexBufferGPU = CreateDefaultBuffer(m_device.Get(), m_commandList.Get(), indices.data(),
        ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexCount = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArags["grid"] = submesh;

    m_geometries["waterGeo"] = std::move(geo);
}

void LandAndWavesApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

    // PSO for opaque objects.

    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaquePsoDesc.InputLayout = { m_inputLayout.data(),(UINT)m_inputLayout.size() };
    opaquePsoDesc.pRootSignature = m_rootSignature.Get();
    opaquePsoDesc.VS = CD3DX12_SHADER_BYTECODE(m_shaders["standardVS"].Get());
    opaquePsoDesc.PS = CD3DX12_SHADER_BYTECODE(m_shaders["opaquePS"].Get());
    opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaquePsoDesc.SampleMask = UINT_MAX;
    opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaquePsoDesc.NumRenderTargets = 1;
    opaquePsoDesc.RTVFormats[0] = m_backBufferFormat;
    opaquePsoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    opaquePsoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    opaquePsoDesc.DSVFormat = m_depthStencilFormat;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque"])));

    // PSO for opaque wireframe objects.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque_wireframe"])));
}

void LandAndWavesApp::BuildRenderItems()
{
    std::unique_ptr<RenderItem> wavesRenderItem = std::make_unique<RenderItem>();
    wavesRenderItem->World = DirectX::XMMatrixIdentity();
    wavesRenderItem->ObjectConstantBufferIndex = 0;
    wavesRenderItem->Geo = m_geometries["waterGeo"].get();
    wavesRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    wavesRenderItem->IndexCount = wavesRenderItem->Geo->DrawArags["grid"].IndexCount;
    wavesRenderItem->StartIndexLocation = wavesRenderItem->Geo->DrawArags["grid"].StartIndexCount;
    wavesRenderItem->BaseVertexLocation = wavesRenderItem->Geo->DrawArags["grid"].BaseVertexLocation;

    m_waveRenderItem = wavesRenderItem.get();
    m_renderItemLayer[(int)RenderLayer::Opaque].push_back(wavesRenderItem.get());

    std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>();
    gridRenderItem->World = DirectX::XMMatrixIdentity();
    gridRenderItem->ObjectConstantBufferIndex = 1;
    gridRenderItem->Geo = m_geometries["landGeo"].get();
}
#endif