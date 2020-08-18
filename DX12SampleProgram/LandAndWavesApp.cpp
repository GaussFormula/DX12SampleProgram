#include "stdafx.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

#include "LandAndWavesApp.h"
#if IS_ENABLE_LAND_APP
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
    ZeroMemory(slotRootParameter, sizeof(CD3DX12_ROOT_PARAMETER) * _countof(slotRootParameter));

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
    ThrowIfFailed(D3DCompileFromFile(L"Shapes.hlsl", nullptr, nullptr, 
        "VSMain", "vs_5_0", compileFlags, 0, &m_shaders["standardVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shapes.hlsl", nullptr, nullptr,
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
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(200.0f, 200.0f, 100, 100);

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
    std::vector<std::uint16_t> indices((UINT64)3 * m_waves->GetTriangleCount());
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
    gridRenderItem->IndexCount = gridRenderItem->Geo->DrawArags["grid"].IndexCount;
    gridRenderItem->StartIndexLocation = gridRenderItem->Geo->DrawArags["grid"].StartIndexCount;
    gridRenderItem->BaseVertexLocation = gridRenderItem->Geo->DrawArags["grid"].BaseVertexLocation;

    m_renderItemLayer[(int)RenderLayer::Opaque].push_back(gridRenderItem.get());

    m_allRenderItems.push_back(std::move(wavesRenderItem));
    m_allRenderItems.push_back(std::move(gridRenderItem));
}

void LandAndWavesApp::BuildFrameResources()
{
    for (int i = 0; i < gNumFrameResources; ++i)
    {
        m_frameResources.push_back(std::make_unique<FrameResource>(m_device.Get(), 1,
            (UINT)m_allRenderItems.size(), m_waves->GetVertexCount()));
    }
}

bool LandAndWavesApp::Initialize()
{
    if (!D3DAppBase::Initialize())
    {
        return false;
    }

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    m_waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildLandGeometry();
    BuildWaveGeometryBuffers();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialize commands.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Wait until initialization is completed.
    FlushCommandQueue();

    return true;
}

void LandAndWavesApp::OnResize()
{
    D3DAppBase::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    m_proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, GetAspectRatio(), 1.0f, 1000.0f);
}

void LandAndWavesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    m_lastMousePos.x = x;
    m_lastMousePos.y = y;

    SetCapture(m_hMainWnd);
}

void LandAndWavesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void LandAndWavesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.03f * static_cast<float>(x - m_lastMousePos.x));
        float dy = XMConvertToRadians(0.03f * static_cast<float>(y - m_lastMousePos.y));

        // Update angles based on input to orbit camera around the items.
        m_cameraTheta += dx;
        m_cameraPhi += dy;
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.2f * static_cast<float>(x - m_lastMousePos.x);
        float dy = 0.2f * static_cast<float>(y - m_lastMousePos.y);

        // Update the camera radius based on input.
        m_cameraRadius += dx - dy;

    }

    m_lastMousePos = { x,y };
}

void LandAndWavesApp::UpdateCamera(const GameTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    m_cameraPos.x = m_cameraRadius * sinf(m_cameraPhi) * cosf(m_cameraTheta);
    m_cameraPos.z = m_cameraRadius * sinf(m_cameraPhi) * sinf(m_cameraTheta);
    m_cameraPos.y = m_cameraRadius * cosf(m_cameraPhi);

    // Build view matrix.
    XMVECTOR pos = XMVectorSet(m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_view = XMMatrixLookAtLH(pos, target, up);
}

void LandAndWavesApp::OnKeyboardInput(const GameTimer& gt)
{
    if (GetAsyncKeyState('1') & 0x8000)
    {
        m_isWireFrame = true;
    }
    else
    {
        m_isWireFrame = false;
    }
}

void LandAndWavesApp::UpdateObjectConstantBuffers(const GameTimer& gt)
{
    auto currentObjectCB = m_currentFrameResource->m_objCB.get();
    for (auto& e : m_allRenderItems)
    {
        // Only update the cbuffer data if the constants have changed.
        // This needs to be tracked per frame resource.
        if (e->NumFrameDirty > 0)
        {
            ObjectConstants obj;
            obj.World = XMMatrixTranspose(e->World);

            currentObjectCB->CopyData(e->ObjectConstantBufferIndex, obj);

            // Next frame resource need to be updated too.
            e->NumFrameDirty--;
        }
    }
}

void LandAndWavesApp::UpdateMainPassConstantBuffer(const GameTimer& gt)
{
    XMMATRIX viewProj = XMMatrixMultiply(m_view, m_proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(m_view), m_view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(m_proj), m_proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    m_mainPassConstantBuffer.View = XMMatrixTranspose(m_view);
    m_mainPassConstantBuffer.InvView = XMMatrixTranspose(invView);
    m_mainPassConstantBuffer.Proj = XMMatrixTranspose(m_proj);
    m_mainPassConstantBuffer.InvProj = XMMatrixTranspose(invProj);
    m_mainPassConstantBuffer.ViewProj = XMMatrixTranspose(viewProj);
    m_mainPassConstantBuffer.InvViewProj = XMMatrixTranspose(invViewProj);
    
    m_mainPassConstantBuffer.EyePosW = m_cameraPos;
    m_mainPassConstantBuffer.RenderTargetSize = XMFLOAT2((float)m_width, (float)m_height);
    m_mainPassConstantBuffer.InvRenderTargetSize = XMFLOAT2(1.0f / m_width, 1.0f / m_height);
    m_mainPassConstantBuffer.NearZ = 1.0f;
    m_mainPassConstantBuffer.FarZ = 1000.0f;
    m_mainPassConstantBuffer.DeltaTime = gt.DeltaTime();
    m_mainPassConstantBuffer.TotalTime = gt.TotalTime();

    UploadBuffer<PassConstants>* currentPassConstantBuffer = m_currentFrameResource->m_passCB.get();
    currentPassConstantBuffer->CopyData(0, m_mainPassConstantBuffer);
}

void LandAndWavesApp::UpdateWaves(const GameTimer& gt)
{
    // Every quarter second, generate a random wave.
    static float t_base = 0.0f;
    if ((m_gameTimer.TotalTime() - t_base >= 0.25f))
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
    UploadBuffer<Vertex>* currentWavesVB = m_currentFrameResource->m_wavesVB.get();

    for (int i = 0; i < m_waves->GetVertexCount(); ++i)
    {
        Vertex v;

        v.Pos = m_waves->Position(i);
        v.Color = XMFLOAT4(DirectX::Colors::Blue);

        currentWavesVB->CopyData(i, v);
    }

    // Set the dynamic VB of the wave render item to the current frame VB.
    m_waveRenderItem->Geo->VertexBufferGPU = currentWavesVB->Resource();
}

void LandAndWavesApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % gNumFrameResources;
    m_currentFrameResource = m_frameResources[m_currentFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (m_currentFrameResource->m_fence != 0 && m_fence->GetCompletedValue() < m_currentFrameResource->m_fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFrameResource->m_fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectConstantBuffers(gt);
    UpdateMainPassConstantBuffer(gt);
    UpdateWaves(gt);
}

void LandAndWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& RenderItems)
{
    UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));

    ID3D12Resource* objectCB = m_currentFrameResource->m_objCB->Resource();

    // For each render item.
    for (size_t i = 0; i < RenderItems.size(); ++i)
    {
        RenderItem* ri = RenderItems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
        objCBAddress += (UINT64)ri->ObjectConstantBufferIndex * objCBByteSize;

        cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

void LandAndWavesApp::Draw(const GameTimer& gt)
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

    // Indicate a state transition on the resource usage.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    // Clear the back buffer and depth buffer.
    m_commandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::GhostWhite, 0, nullptr);
    m_commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    m_commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    // Bind per-pass constant buffer. We only need to do this once per-pass.
    auto passCB = m_currentFrameResource->m_passCB->Resource();
    m_commandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    DrawRenderItems(m_commandList.Get(), m_renderItemLayer[(int)RenderLayer::Opaque]);

    // Indicate a state transition on the resource usage.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
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
    // Because we are on the GPU timeline, the new fence point won't be set
    // the GPU finishes processing all the commands prior to this signal().
    m_commandQueue->Signal(m_fence.Get(), m_currentFence);
}
#endif