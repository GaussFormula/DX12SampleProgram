#include "stdafx.h"
#include "ShapesApp.h"

ShapesApp::ShapesApp(HINSTANCE hInstance)
    :D3DAppBase(hInstance)
{}

ShapesApp::~ShapesApp()
{
    if (m_device != nullptr)
    {
        FlushCommandQueue();
    }
}

void ShapesApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];

    // Create root CBVs.
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // Create a root signature with a single slot which points to a descriptor
    // range consisting of a single constant buffer.
    ComPtr<ID3DBlob>    serializedRootSig = nullptr;
    ComPtr<ID3DBlob>    errorBlob = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig, &errorBlob));

    ThrowIfFailed(m_device->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    ));
}

void ShapesApp::BuildShadersAndInputLayout()
{
#if defined(_DEBUG)||(DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &m_shaders["standardVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &m_shaders["opaquePS"], nullptr));

    m_inputLayout =
    {
        {"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };
}

void ShapesApp::BuildShapesGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
    GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(20, 20, 0.5f, 3.0f);

    // We are concatenating all the geometry into one big vertex/index buffer.
    // So define the regions in the buffer each submesh covers.

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
    UINT sphereVertexOffset = 0;
    UINT pyramidVertexOffset = (UINT)sphere.Vertices.size();
    
    // Cache the starting index for each object in the concatenated index buffer.
    UINT sphereIndexOffset = 0;
    UINT pyramidIndexOffset = (UINT)sphere.Indices32.size();

    // Define the SubmeshGeometry that cover different 
    // regions of the vertex/index buffers.
    SubmeshGeometry sphereSubmesh;
    sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubmesh.StartIndexCount = sphereIndexOffset;
    sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

    SubmeshGeometry pyramidSubmesh;
    pyramidSubmesh.IndexCount = (UINT)pyramid.Indices32.size();
    pyramidSubmesh.StartIndexCount = pyramidIndexOffset;
    pyramidSubmesh.BaseVertexLocation = pyramidVertexOffset;

    // Extract the vertex elements we are interested in and pack the 
    // vertices of all the meshes into one vertex buffer.
    auto totalVertexCount = sphere.Indices32.size() + pyramid.Indices32.size();

    std::vector<Vertex> vertices(totalVertexCount);
    UINT k = 0;
    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
    }
    for (size_t i=0;i<pyramid.Vertices.size();++i,++k)
    {
        vertices[k].Pos = pyramid.Vertices[i].Position;
        vertices[k].Color = XMFLOAT4(DirectX::Colors::ForestGreen);
    }

    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
    indices.insert(indices.end(), std::begin(pyramid.GetIndices16()), std::end(pyramid.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "shapeGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = CreateDefaultBuffer(
        m_device.Get(), m_commandList.Get(), vertices.data(),
        vbByteSize, geo->VertexBufferUploader
    );

    geo->IndexBufferGPU = CreateDefaultBuffer(
        m_device.Get(), m_commandList.Get(), indices.data(),
        ibByteSize, geo->IndexBufferUploader
    );

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArags["sphere"] = sphereSubmesh;
    geo->DrawArags["pyramid"] = pyramidSubmesh;

    m_geometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
    // PSO for opaque objects.

    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaquePsoDesc.InputLayout = { m_inputLayout.data(),(UINT)m_inputLayout.size() };
    opaquePsoDesc.pRootSignature = m_rootSignature.Get();
    opaquePsoDesc.VS = CD3DX12_SHADER_BYTECODE(m_shaders["standardVS"].Get());
    opaquePsoDesc.PS = CD3DX12_SHADER_BYTECODE(m_shaders["opaquePS"].Get());
    opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
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

void ShapesApp::BuildRenderItems()
{
    UINT objectCBufferIndex = 0;
    auto sphereRenderItem = std::make_unique<RenderItem>();
    sphereRenderItem->World = XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f);
    sphereRenderItem->ObjectConstantBufferIndex = objectCBufferIndex++;
    sphereRenderItem->Geo = m_geometries["shapeGeo"].get();
    sphereRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    sphereRenderItem->IndexCount = sphereRenderItem->Geo->DrawArags["sphere"].IndexCount;
    sphereRenderItem->BaseVertexLocation = sphereRenderItem->Geo->DrawArags["sphere"].BaseVertexLocation;
    sphereRenderItem->StartIndexLocation = sphereRenderItem->Geo->DrawArags["sphere"].StartIndexCount;

    m_allItems.push_back(std::move(sphereRenderItem));

    auto pyramidRenderItem = std::make_unique<RenderItem>();
    pyramidRenderItem->World = DirectX::XMMatrixIdentity();
    pyramidRenderItem->ObjectConstantBufferIndex = objectCBufferIndex++;
    pyramidRenderItem->Geo = m_geometries["shapeGeo"].get();
    pyramidRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    pyramidRenderItem->IndexCount = pyramidRenderItem->Geo->DrawArags["pyramid"].IndexCount;
    pyramidRenderItem->StartIndexLocation = pyramidRenderItem->Geo->DrawArags["pyramid"].StartIndexCount;
    pyramidRenderItem->BaseVertexLocation = pyramidRenderItem->Geo->DrawArags["pyramid"].BaseVertexLocation;
    m_allItems.push_back(std::move(pyramidRenderItem));

    for (auto& e : m_allItems)
    {
        m_opaqueRenderItems.push_back(e.get());
    }
}

void ShapesApp::BuildFrameResources()
{
    for (int i = 0; i < gNumFrameResources; ++I)
    {
        m_frameResources.push_back(std::make_unique<FrameResource>(m_device.Get(), 1, (UINT)m_allItems.size()));
    }
}

void ShapesApp::BuildConstantDescriptorHeaps()
{
    UINT objCount = (UINT)m_opaqueRenderItems.size();

    // Need a CBV descriptor for each object for each frame resource.
    // +1 for the per pass CBV for each frame resource.
    UINT numDescriptors = (objCount + 1) * gNumFrameResources;

    // Svae an offset to the start of the pass CBVs. These are the last 3 descriptors.
    m_passCbvOffset = objCount * gNumFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = numDescriptors;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}

bool ShapesApp::Initialize()
{
    if (!D3DAppBase::Initialize())
    {
        return false;
    }

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
}