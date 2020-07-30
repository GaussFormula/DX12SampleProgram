#include "stdafx.h"
#include "ShapesApp.h"
#if IS_ENABLE_SHAPE_APP
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
    ThrowIfFailed(D3DCompileFromFile(L"Shapes.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &m_shaders["standardVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shapes.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &m_shaders["opaquePS"], nullptr));

    m_inputLayout =
    {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };
}

void ShapesApp::BuildShapesGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
    GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(10, 10, 0.5f, 3.0f);
    //GeometryGenerator::MeshData pyramid= geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
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
    sphereRenderItem->World = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f);
    sphereRenderItem->ObjectConstantBufferIndex = objectCBufferIndex++;
    sphereRenderItem->Geo = m_geometries["shapeGeo"].get();
    sphereRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    sphereRenderItem->IndexCount = sphereRenderItem->Geo->DrawArags["sphere"].IndexCount;
    sphereRenderItem->BaseVertexLocation = sphereRenderItem->Geo->DrawArags["sphere"].BaseVertexLocation;
    sphereRenderItem->StartIndexLocation = sphereRenderItem->Geo->DrawArags["sphere"].StartIndexCount;

    m_allItems.push_back(std::move(sphereRenderItem));

    auto pyramidRenderItem = std::make_unique<RenderItem>();
    pyramidRenderItem->World = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(1.0f, -0.5f, 0.0f);
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
    for (int i = 0; i < gNumFrameResources; ++i)
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

    // Save an offset to the start of the pass CBVs. These are the last 3 descriptors.
    m_passCbvOffset = objCount * gNumFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = numDescriptors;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}

void ShapesApp::BuildConstantBufferAndViews()
{
    UINT objectCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));
    UINT objCount = (UINT)m_opaqueRenderItems.size();

    // Need a CBV descriptor for each object for each frame resources.
    for (UINT frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto objectCB = m_frameResources[frameIndex]->m_objCB->Resource();
        for (UINT i = 0; i < objCount; ++i)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

            // Offset to the ith object constant buffer in the buffer.
            cbAddress += (UINT64)i * objectCBByteSize;

            // Offset to the object cbv in the descriptor heap.
            int heapIndex = frameIndex * objCount + i;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, m_cbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbAddress;
            cbvDesc.SizeInBytes = objectCBByteSize;

            m_device->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    UINT passCBByteSize = CalculateConstantBufferByteSize(sizeof(PassConstants));

    // Last three descriptors are the pass CBVs for each frame resource.
    for (UINT frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto passCB = m_frameResources[frameIndex]->m_passCB->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

        // Offset to the pass cbv in the descriptor heap.
        int heapIndex = m_passCbvOffset + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, m_cbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = passCBByteSize;

        m_device->CreateConstantBufferView(&cbvDesc, handle);
    }
}

bool ShapesApp::Initialize()
{
    if (!D3DAppBase::Initialize())
    {
        return false;
    }

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildShapesGeometry();
    BuildRenderItems();
    BuildFrameResources();
    BuildConstantDescriptorHeaps();
    BuildConstantBufferAndViews();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Wait until initialization is completed.
    FlushCommandQueue();

    return true;
}

void ShapesApp::OnResize()
{
    D3DAppBase::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    m_proj = XMMatrixPerspectiveFovLH(0.25f * XM_PI, GetAspectRatio(), 1.0f, 1000.0f);
}

void ShapesApp::OnKeyboardInput(const GameTimer& gt)
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

void ShapesApp::UpdateCamera(const GameTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    m_eyePos.x = m_radius * sinf(m_phi) * cosf(m_theta);
    m_eyePos.y = m_radius * cosf(m_phi);
    m_eyePos.z = m_radius * sinf(m_phi) * sinf(m_theta);

    // Build view matrix.
    XMVECTOR pos = XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
    //XMVECTOR pos = XMVectorSet(1.5f, 0.0f, 0.0f, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    m_view = XMMatrixLookAtLH(pos, target, up);
}

void ShapesApp::UpdateObjectCBs(const GameTimer& gt)
{
    auto currentObjectConstantBuffer = m_currentFrameResource->m_objCB.get();
    for (auto& e : m_allItems)
    {
        // Only update the cbuffer data if the constants have changed.
        // This needs to be tracked per frame resource.
        if (e->NumFrameDirty > 0)
        {
            ObjectConstants objConstants;
            objConstants.World = XMMatrixTranspose(e->World);

            currentObjectConstantBuffer->CopyData(e->ObjectConstantBufferIndex, objConstants);

            // Next FrameResource need to be updated too.
            e->NumFrameDirty--;
        }
    }
}

void ShapesApp::UpdateMainPassCB(const GameTimer& gt)
{
    XMMATRIX viewProj = XMMatrixMultiply(m_view, m_proj);
    m_viewProj = viewProj;
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(m_view), m_view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(m_proj), m_proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    m_mainPassCB.View = XMMatrixTranspose(m_view);
    m_mainPassCB.InvView = XMMatrixTranspose(invView);
    m_mainPassCB.Proj = XMMatrixTranspose(m_proj);
    m_mainPassCB.InvProj = XMMatrixTranspose(invProj);
    m_mainPassCB.ViewProj = XMMatrixTranspose(viewProj);
    m_mainPassCB.InvViewProj = XMMatrixTranspose(invViewProj);

    m_mainPassCB.EyePosW = m_eyePos;
    m_mainPassCB.RenderTargetSize = XMFLOAT2((float)m_width, (float)m_height);
    m_mainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_width, 1.0f / m_height);
    m_mainPassCB.NearZ = 1.0f;
    m_mainPassCB.FarZ = 1000.0f;
    m_mainPassCB.DeltaTime = gt.DeltaTime();
    m_mainPassCB.TotalTime = gt.TotalTime();

    auto currentPassCB = m_currentFrameResource->m_passCB.get();
    currentPassCB->CopyData(0, m_mainPassCB);
}

void ShapesApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % gNumFrameResources;
    m_currentFrameResource = m_frameResources[m_currentFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resources?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (m_currentFrameResource->m_fence != 0 && m_fence->GetCompletedValue() < m_currentFrameResource->m_fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFrameResource->m_fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectCBs(gt);
    UpdateMainPassCB(gt);
    result = XMVector3TransformCoord(XMVectorSet(-0.5f,-0.5f,-0.5f,1.0f), m_viewProj);
    XMStoreFloat4(&result2, result);
}

void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& RenderItems)
{
    UINT objCBByteSize = CalculateConstantBufferByteSize(sizeof(ObjectConstants));

    auto objectCB = m_currentFrameResource->m_objCB->Resource();

    // For each render item...
    for (size_t i = 0; i < RenderItems.size(); ++i)
    {
        auto ri = RenderItems[i];
        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        // Offset to the CBV in the descriptor heap for this object 
        // and for this frame resource.
        UINT cbvIndex = m_currentFrameResourceIndex * (UINT)m_opaqueRenderItems.size() + ri->ObjectConstantBufferIndex;
        auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(cbvIndex, m_cbvSrvUavDescriptorSize);

        cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

void ShapesApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = m_currentFrameResource->m_commandAllocator;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command list have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    if (m_isWireFrame)
    {
        ThrowIfFailed(m_commandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque_wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(m_commandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));
    }

    m_commandList->RSSetViewports(1, &m_screenViewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate a state transition on the resource usage.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    // Clear the back buffer and depth buffer.
    m_commandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightPink, 0, nullptr);
    m_commandList->ClearDepthStencilView(DepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    m_commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, 
        &m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    UINT passCbvIndex = m_passCbvOffset + (UINT)m_currentFrameResourceIndex;
    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passCbvIndex, m_cbvSrvUavDescriptorSize);
    m_commandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);
    
    DrawRenderItems(m_commandList.Get(), m_opaqueRenderItems);

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
    // Because we are on the GPU time line, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    m_commandQueue->Signal(m_fence.Get(), m_currentFence);
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    m_lastMousePos.x = x;
    m_lastMousePos.y = y;

    SetCapture(m_hMainWnd);
}

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));

        // Update angles based on input to orbit camera around box.
        m_theta += dx;
        m_phi += dy;

    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f * static_cast<float>(x - m_lastMousePos.x);
        float dy = 0.05f * static_cast<float>(y - m_lastMousePos.y);

        // Update the camera radius based on input.
        m_radius += dx - dy;

    }

    m_lastMousePos.x = x;
    m_lastMousePos.y = y;
}

#endif