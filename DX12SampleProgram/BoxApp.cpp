#include "stdafx.h"
#include "BoxApp.h"

BoxApp::BoxApp(HINSTANCE hInstance):
    D3DAppBase(hInstance)
{

}

BoxApp::~BoxApp()
{

}

void BoxApp::BuildConstantDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
    m_objectCB = std::make_unique<UploadBuffer<ObjectConstants>>(m_device.Get(), 1, true);
    UINT objectConstantBufferByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_objectCB->Resource()->GetGPUVirtualAddress();

    UINT boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objectConstantBufferByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = CalculateConstantBufferByteSize(sizeof(ObjectConstants));
    m_device->CreateConstantBufferView(&cbvDesc,
        m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // Create a single descriptor table of CBVs.
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    // Create a single descriptor table of CBVs.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // Create a root signature with a single slot which points to a descriptor range consisting of a single 
    // constant buffer.
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig, &errorBlob));

    ThrowIfFailed(m_device->CreateRootSignature(
        0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), 
        IID_PPV_ARGS(&m_rootSignature)));
}

void BoxApp::BuildShadersAndInputLayout()
{
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &m_vertexShaderByteCode, nullptr));
    ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &m_pixelShaderByteCode, nullptr));

    m_inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void BoxApp::BuildBoxGeometry()
{
    std::array<Vertex, 8> vertices =
    {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
        Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
    };

    std::array<std::uint16_t, 36> indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };
    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    m_boxGeo = std::make_unique<MeshGeometry>();
    m_boxGeo->Name = "BoxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_boxGeo->VertexBufferCPU));
    CopyMemory(m_boxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_boxGeo->IndexBufferCPU));
    CopyMemory(m_boxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    m_boxGeo->VertexBufferGPU = CreateDefaultBuffer(m_device.Get(),
        m_commandList.Get(), vertices.data(), vbByteSize,
        m_boxGeo->VertexBufferUploader);

    m_boxGeo->IndexBufferGPU = CreateDefaultBuffer(m_device.Get(), m_commandList.Get(),
        indices.data(), ibByteSize, m_boxGeo->IndexBufferUploader);

    m_boxGeo->VertexByteStride = sizeof(Vertex);
    m_boxGeo->VertexBufferByteSize = vbByteSize;
    m_boxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    m_boxGeo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.BaseVertexLocation = 0;
    submesh.StartIndexCount = 0;

    m_boxGeo->DrawArags["box"] = submesh;
}

void BoxApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { m_inputLayout.data(),(UINT)m_inputLayout.size() };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_vertexShaderByteCode.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_pixelShaderByteCode.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_backBufferFormat;
    psoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = m_depthStencilFormat;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&m_PSO)
    ));
}

bool BoxApp::Initialize()
{
    if (!D3DAppBase::Initialize())
    {
        return false;
    }

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
}