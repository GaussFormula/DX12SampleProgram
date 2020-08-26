#include "stdafx.h"
#include "LitWavesApp.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#if IS_ENABLE_LITLAND_APP

LitWavesApp::LitWavesApp(HINSTANCE hInstance)
    :D3DAppBase(hInstance)
{

}

LitWavesApp::~LitWavesApp()
{
    if (m_device != nullptr)
    {
        FlushCommandQueue();
    }
}

void LitWavesApp::BuildRootSignature()
{
    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameters[3];

    // Create root CBV.
    for (int i = 0; i < _countof(slotRootParameters); ++i)
    {
        slotRootParameters[i].InitAsConstantBufferView(i);
    }

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameters), slotRootParameters, 0,
        nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // Create a root signature with single slot which points to a descriptor
    // range consisting of a constant buffer.
    ComPtr<ID3DBlob>    serializedRootSig = nullptr;
    ComPtr<ID3DBlob>    errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);
    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(m_device->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    ));
}

void LitWavesApp::BuildShadersAndInputLayout()
{
#if defined(DEBUG)|(_DEBUG)
    UINT compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlag = 0;
#endif
    ThrowIfFailed(D3DCompileFromFile(L"Default.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSMain", "vs_5_0", compileFlag, 0, &m_shaders["standardVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Default.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSMain", "ps_5_0", compileFlag, 0, &m_shaders["opaquePS"], nullptr));

    m_inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void LitWavesApp::BuildLandGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

    // Extract the vertex elements we are interested and apply the height function to 
    // each vertex. In addition, color the vertices based on their height so we have 
    // sandy looking beaches, grassy low hills, and snow mountain peaks.

    std::vector<Vertex> vertices;
    const size_t verticesSize = (size_t)grid.Vertices.size();
    for (size_t i = 0; i < verticesSize; ++i)
    {
        XMFLOAT3& p = grid.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
        vertices[i].Normal = GetHillsNormal(p.x, p.z);
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
        vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = CreateDefaultBuffer(m_device.Get(), m_commandList.Get(), indices.data(),
        ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.BaseVertexLocation = 0;
    submesh.StartIndexCount = 0;
    submesh.IndexCount = (UINT)indices.size();

    geo->DrawArags["grid"] = submesh;

    m_geometries["landGeo"] = std::move(geo);
}

void LitWavesApp::BuildWavesGeometryBuffers()
{
    // 3 indices per face.
    std::vector<std::uint16_t> indices((size_t)3 * m_waves->GetTriangleCount());
    assert(m_waves->GetVertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = m_waves->GetRowCount();
    int n = m_waves->GetColumnCount();
    size_t k = 0;
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
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
    geo->Name = "waterGeo";

    // Set dynamically.
    geo->VertexBufferGPU = nullptr;
    geo->VertexBufferCPU = nullptr;

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->IndexBufferGPU = CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
        indices.data(), ibByteSize, geo->IndexBufferUploader);

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

void LitWavesApp::BuildMaterials()
{
    std::unique_ptr < Material > grass = std::make_unique<Material>();

    grass->Name = "grass";
    grass->MaterialConstantBufferIndex = 0;
    grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
    grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    grass->Roughness = 0.125f;

    // This is not a good water material defination, but we do not have all the render
    // tools we need (tranparency, enviroment reflection), so we fake it for now.
    std::unique_ptr<Material> water = std::make_unique<Material>();
    water->Name = "water";
    water->MaterialConstantBufferIndex = 1;
    water->DiffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
    water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    water->Roughness = 0.0f;

    m_materials["grass"] = std::move(grass);
    m_materials["water"] = std::move(water);
}

void LitWavesApp::BuildRenderItems()
{
    std::unique_ptr<RenderItem> wavesRenderItem = std::make_unique<RenderItem>();
    wavesRenderItem->World = XMMatrixIdentity();
    wavesRenderItem->ObjectConstantBufferIndex = 0;
    wavesRenderItem->Mat = m_materials["water"].get();
    wavesRenderItem->Geo = m_geometries["waterGeo"].get();
    wavesRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    wavesRenderItem->IndexCount = wavesRenderItem->Geo->DrawArags["grid"].IndexCount;
    wavesRenderItem->StartIndexLocation = wavesRenderItem->Geo->DrawArags["grid"].StartIndexCount;
    wavesRenderItem->BaseVertexLocation = wavesRenderItem->Geo->DrawArags["grid"].BaseVertexLocation;

    m_wavesItem = wavesRenderItem.get();

    m_renderItemLayer[(int)RenderLayer::Opaque].push_back(wavesRenderItem.get());

    std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>();
    gridRenderItem->World = XMMatrixIdentity();
    gridRenderItem->ObjectConstantBufferIndex = 1;
    gridRenderItem->Mat = m_materials["grass"].get();
    gridRenderItem->Geo = m_geometries["landGeo"].get();
    gridRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRenderItem->IndexCount = gridRenderItem->Geo->DrawArags["grid"].IndexCount;
    gridRenderItem->BaseVertexLocation = gridRenderItem->Geo->DrawArags["grid"].BaseVertexLocation;
    gridRenderItem->StartIndexLocation = gridRenderItem->Geo->DrawArags["grid"].StartIndexCount;

    m_renderItemLayer[(int)RenderLayer::Opaque].push_back(gridRenderItem.get());

    m_allRenderItems.push_back(std::move(wavesRenderItem));
    m_allRenderItems.push_back(std::move(gridRenderItem));
}

void LitWavesApp::BuildFrameResources()
{
    for (UINT i = 0; i < gNumFrameResources; ++i)
    {
        m_frameResources.push_back(std::make_unique<FrameResource>(m_device.Get(),
            1, (UINT)m_allRenderItems.size(), (UINT)m_waves->GetVertexCount(), (UINT)m_materials.size()));
    }
}

#endif // IS_ENABLE_LITLAND_APP