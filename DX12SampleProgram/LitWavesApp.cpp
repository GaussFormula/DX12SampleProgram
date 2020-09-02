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

void LitWavesApp::BuildPSOs()
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

bool LitWavesApp::Initialize()
{
    if (!D3DAppBase::Initialize())
    {
        return false;
    }

    // Reset command list to prep for initialization commands.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    m_waves = std::make_unique<Waves>(256, 256, 1.0f, 0.03f, 4.0f, 0.2f);

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildLandGeometry();
    BuildWavesGeometryBuffers();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Wait until initialization is completed.
    FlushCommandQueue();

    return true;
}

void LitWavesApp::OnResize()
{
    D3DAppBase::OnResize();

    // When window resized, so update the aspect ratio and recompute the projection matrix.
    m_proj = XMMatrixPerspectiveFovLH(0.25 * XM_PI, GetAspectRatio(), 1.0f, 1000.0f);
}

void LitWavesApp::OnKeyboardInput(const GameTimer& gt)
{
    const float dt = gt.DeltaTime();

    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
    {
        m_sunTheta -= 1.0f * dt;
    }

    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
    {
        m_sunTheta += 1.0f * dt;
    }

    if (GetAsyncKeyState(VK_UP) & 0x8000)
    {
        m_sunPhi -= 1.0f * dt;
    }

    if (GetAsyncKeyState(VK_DOWN) & 0x8000)
    {
        m_sunPhi += 1.0f * dt;
    }

    if (GetAsyncKeyState('1') & 0x8000)
    {
        m_isWireFrame = true;
    }
    else
    {
        m_isWireFrame = false;
    }
}

void LitWavesApp::UpdateCamera(const GameTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.

    // theta is angle of vector with Y axis.
    // phi is angle of vector with x axis.
    m_cameraPos.x = m_radius * sinf(m_phi) * cosf(m_theta);
    m_cameraPos.y = m_radius * cosf(m_phi);
    m_cameraPos.z = m_radius * sinf(m_phi) * sinf(m_theta);

    // Build the view matrix.
    XMVECTOR pos = XMVectorSet(m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    
    m_view = XMMatrixLookAtLH(pos, target, up);
}

void LitWavesApp::UpdateObjectConstantBuffers(const GameTimer& gt)
{
    UploadBuffer<ObjectConstants>* currentObjectCB = m_currentFrameResource->m_objCB.get();
    for (auto& e : m_allRenderItems)
    {
        // Only update the cbuffer data if the constants have changed.
        // This needs to be tracked per frame resource.
        if (e->NumFrameDirty > 0)
        {
            ObjectConstants objConstants;
            objConstants.World = XMMatrixTranspose(e->World);

            e->TexTransform = XMMatrixIdentity();

            currentObjectCB->CopyData(e->ObjectConstantBufferIndex, objConstants);

            // Next FrameResource need to be updated too.
            e->NumFrameDirty--;
        }
    }
}

void LitWavesApp::UpdateMaterialConstantBuffers(const GameTimer& gt)
{
    auto currentMaterialCB = m_currentFrameResource->m_materialCB.get();
    for (auto& e : m_materials)
    {
        // Only update the cbuffer data if the constants have changed.
        // If the cbuffer data changes, it needs to be updated for each FrameResource.
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;

            currentMaterialCB->CopyData(mat->MaterialConstantBufferIndex, matConstants);

            // Next FrameResource need to be updated too.
            mat->NumFramesDirty--;
        }
    }
}

void LitWavesApp::UpdateMainPassConstantBuffer(const GameTimer& gt)
{
    XMMATRIX viewProj = XMMatrixMultiply(m_view, m_proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(m_view), m_view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(m_proj), m_proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    m_mainPassCB.View = XMMatrixTranspose(m_view);
    m_mainPassCB.InvView = XMMatrixTranspose(invView);
    m_mainPassCB.Proj = XMMatrixTranspose(m_proj);
    m_mainPassCB.InvProj = XMMatrixTranspose(invProj);
    m_mainPassCB.ViewProj = XMMatrixTranspose(viewProj);
    m_mainPassCB.InvViewProj = XMMatrixTranspose(invViewProj);

    m_mainPassCB.EyePosW = m_cameraPos;
    m_mainPassCB.RenderTargetSize = XMFLOAT2((float)m_width, (float)m_height);
    m_mainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_width, 1.0f / m_height);
    m_mainPassCB.NearZ = 1.0f;
    m_mainPassCB.FarZ = 1000.0f;
    m_mainPassCB.TotalTime = gt.TotalTime();
    m_mainPassCB.DeltaTime = gt.DeltaTime();
    m_mainPassCB.AmbientLight = { 0.25f,0.25f,0.35f,1.0f };

    XMVECTOR lightDir = -myMathLibrary::SphericalToCartesian(1.0f, m_sunTheta, m_sunPhi);
    XMStoreFloat3(&m_mainPassCB.Lights[0].Direction, lightDir);
    m_mainPassCB.Lights[0].Strength = { 1.0f,1.0f,0.9f };

    auto currentPassCB = m_currentFrameResource->m_passCB.get();
    currentPassCB->CopyData(0, m_mainPassCB);
}

void LitWavesApp::UpdateWaves(const GameTimer& gt)
{
    // Every quarter second, generate a random wave.
    static float t_base = 0.0f;
    if ((m_gameTimer.TotalTime() - t_base) >= 0.25f)
    {
        t_base += 0.25f;

        int i = myMathLibrary::Rand(4, m_waves->GetRowCount() - 5);
        int j = myMathLibrary::Rand(4, m_waves->GetColumnCount() - 5);

        float r = myMathLibrary::RandF(0.2f, 0.5f);

        m_waves->Disturb(i, j, r);
    }

    // Update the wave simulation.
    m_waves->Update(gt.DeltaTime());

    // Update the wave vertex buffer with the new solution.
    UploadBuffer<Vertex>* currentWaveCB = m_currentFrameResource->m_wavesVB.get();
    for (int i = 0; i < m_waves->GetVertexCount(); ++i)
    {
        Vertex v;
        v.Pos = m_waves->Position(i);
        v.Normal = m_waves->Normal(i);

        currentWaveCB->CopyData(i, v);
    }

    // Set the dynamic VB of the wave renderitem to the current frame VB.
    m_wavesItem->Geo->VertexBufferGPU = currentWaveCB->Resource();
}

void LitWavesApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % gNumFrameResources;
    m_currentFrameResource = m_frameResources[m_currentFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource.
    // If not, wait until the GPU has completed commands up to this fence point.
    if (m_currentFrameResource->m_fence != 0 && m_fence->GetCompletedValue() < m_currentFrameResource->m_fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFrameResource->m_fence, eventHandle));
        WaitForSingleObject(eventHandle,INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectConstantBuffers(gt);
    UpdateMaterialConstantBuffers(gt);
    UpdateMainPassConstantBuffer(gt);
    UpdateWaves(gt);
}

void LitWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems)
{
    UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = CalculateConstantBufferByteSize(sizeof(MaterialConstants));

    ID3D12Resource* objectCB = m_currentFrameResource->m_objCB->Resource();
    ID3D12Resource* matCB = m_currentFrameResource->m_materialCB->Resource();

    // For each render item...
    for (size_t i = 0; i < renderItems.size(); ++i)
    {
        RenderItem* ri = renderItems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjectConstantBufferIndex * (UINT64)objCBByteSize;
        D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MaterialConstantBufferIndex * (UINT64)matCBByteSize;

        cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

void LitWavesApp::Draw(const GameTimer& gt)
{
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(m_currentFrameResource->m_commandAllocator->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    if (m_isWireFrame)
    {
        ThrowIfFailed(m_commandList->Reset(m_currentFrameResource->m_commandAllocator.Get(),
            m_PSOs["opaque_wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(m_commandList->Reset(m_currentFrameResource->m_commandAllocator.Get(),
            m_PSOs["opaque"].Get()));
    }

    m_commandList->RSSetViewports(1, &m_screenViewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate a state transition on the resouce usage.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    // Clear the back buffer and depth buffer.
    m_commandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightBlue, 0, nullptr);
    m_commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    m_commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12Resource* passCB = m_currentFrameResource->m_passCB->Resource();
    m_commandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(m_commandList.Get(), m_renderItemLayer[(int)RenderLayer::Opaque]);

    // Indicate a state transition on the resource usage.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    ));

    // Done recording commands.
    ThrowIfFailed(m_commandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Swap the back and front buffers.
    ThrowIfFailed(m_swapchain->Present(0, 0));
    m_currentBackBufferIndex = (m_currentBackBufferIndex + 1) % m_swapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    m_currentFrameResource->m_fence = ++m_currentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    m_commandQueue->Signal(m_fence.Get(), m_currentFence);
}
#endif // IS_ENABLE_LITLAND_APP