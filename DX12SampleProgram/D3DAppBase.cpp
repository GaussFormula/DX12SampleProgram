#include "D3DAppBase.h"
#include    <windowsx.h>
using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

D3DAppBase* D3DAppBase::m_app = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Forward hwnd on because we can get messages.
    return D3DAppBase::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DAppBase* D3DAppBase::GetApp()
{
    return m_app;
}

D3DAppBase::D3DAppBase(HINSTANCE hInstance):m_hAppInstance(hInstance)
{
    // Only one d3dapp can be constructed.
    assert(m_app == nullptr);
    m_app = this;

    WCHAR assetsPath[512];
    GetAssetPath(assetsPath, _countof(assetsPath));
    m_assetPath = assetsPath;
}

D3DAppBase::~D3DAppBase()
{
    if (m_device != nullptr)
    {
        FlushCommandQueue();
    }
}

HINSTANCE D3DAppBase::GetAppInstance()const
{
    return m_hAppInstance;
}

HWND D3DAppBase::GetMainWnd() const
{
    return m_hMainWnd;
}

float D3DAppBase::GetAspectRatio() const
{
    return static_cast<float>(m_width) / static_cast<float>(m_height);
}

bool D3DAppBase::Get4xMsaaState() const
{
    return m_4xMsaaState;
}

void D3DAppBase::Set4xMsaaState(const bool value)
{
    if (m_4xMsaaState != value)
    {
        m_4xMsaaState = value;

        // Recreate the swapchain and buffers with new multisample settings.
        CreateSwapChain();
        OnResize();
    }
}

int D3DAppBase::Run()
{
    MSG msg = { 0 };

    m_gameTimer.Reset();
    while (msg.message!=WM_QUIT)
    {
        // if there are window messages, then process them.
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Otherwise, do animation/game stuff.
        else
        {
            m_gameTimer.Tick();
            if (!m_appPaused)
            {
                CalculateFrameStats();
                Update(m_gameTimer);
                Draw(m_gameTimer);
            }
            else
            {
                Sleep(100);
            }
        }
    }
    return (int)msg.wParam;
}

bool D3DAppBase::InitMainWindow()
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hAppInstance;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"MainWnd";

    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
        return false;
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = { 0,0,m_width,m_height };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;
    m_hMainWnd = CreateWindow(L"MainWnd", m_mainWndCaption.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, m_hAppInstance, 0);
    if (!m_hMainWnd)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return false;
    }
    ShowWindow(m_hMainWnd, SW_SHOW);
    UpdateWindow(m_hMainWnd);
    return true;
}

LRESULT D3DAppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // WM_CREATE is sent when the window is activated or deactivated.
        // We pause the game when the window is deactivated and unpause it
        // when it becomes active.
    case WM_CREATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            m_appPaused = true;
            m_gameTimer.Stop();
        }
        else
        {
            m_appPaused = false;
            m_gameTimer.Start();
        }
        return 0;

    // WM_SIZE is sent when the user resizes the window.
    case WM_SIZE:
        // Save the new client area dimensions.
        m_width = LOWORD(lParam);
        m_height = HIWORD(lParam);
        if (m_device)
        {
            if (wParam == SIZE_MINIMIZED)
            {
                m_appPaused = true;
                m_minimized = true;
                m_maximized = false;
            }
            else if (wParam == SIZE_MAXIMIZED)
            {
                m_appPaused = true;
                m_minimized = false;
                m_maximized = true;
                OnResize();
            }
            else if (wParam == SIZE_RESTORED)
            {
                // Restoring from minimized state?
                if (m_minimized)
                {
                    m_appPaused = false;
                    m_minimized = false;
                    OnResize();
                }
                else if (m_maximized)
                {
                    m_appPaused = false;
                    m_maximized = false;
                    OnResize();
                }
                else if (m_resizing)
                {
                    // If user is dragging the resize bars, we do not know resize
                    // the buffers here bacause as the user continuously 
                    // drags the resize bars, a stream of WM_SIZE messages are sent
                    // to the window, and it would be pointless (and slow) 
                    // to resize for each WM_SIZE message received from dragging 
                    // the resize bars. So instead , we reset after the user is done
                    // resizing the window and releases the resize bars, which 
                    // sends a WM_EXITSIZEMOVE message.
                }
                else // API call such as SetWindowPos or m_swapChain->SetFullscrennState()
                {
                    OnResize();
                }
            }
        }
        return 0;

        // WM_ENTERSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
        m_appPaused = true;
        m_resizing = true;
        m_gameTimer.Stop();
        return 0;

        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
        // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
        m_appPaused = false;
        m_resizing = false;
        m_gameTimer.Start();
        OnResize();
        return 0;

        // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

        // The WM_MENUCHAR message is sent when a menu is active and the user presses
        // a key that does not correspond to any mnemonic or accelerator key.
    case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

        // Catch this message so to prevent the window from being too small.
    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
        return 0;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_LBUTTONUP:
    case  WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if ((int)wParam == VK_F2)
        {
            Set4xMsaaState(!m_4xMsaaState);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DAppBase::Initialize()
{
    if (!InitMainWindow())
    {
        return false;
    }

    if (!InitDirect3D())
    {
        return false;
    }

    // Do the initial resize code.
    OnResize();
    return true;
}

void D3DAppBase::CreateFactory()
{
    UINT dxgiFactoryFlags = 0;
#if defined(DEBUG)||defined(_DEBUG)
    // Enable the debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags,IID_PPV_ARGS(&m_factory)));
}

void D3DAppBase::CreateHardwareAdapter()
{
    m_adapter = nullptr;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &m_adapter); adapterIndex++)
    {
        DXGI_ADAPTER_DESC1 desc;
        m_adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter
            // If you want a software adapter, pass in "warp" on the command line.
            continue;
        }
        if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }
}

void D3DAppBase::CreateAdapter()
{
    if (m_useWarpDevice)
    {
        ThrowIfFailed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&m_adapter)));
    }
    else
    {
        CreateHardwareAdapter();
        assert(m_adapter != nullptr);
        ThrowIfFailed(D3D12CreateDevice(m_adapter.Get(), 
            D3D_FEATURE_LEVEL_11_0, 
            IID_PPV_ARGS(&m_device)));
    }
}

void D3DAppBase::CreateFenceObject()
{
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
}

void D3DAppBase::InitDescriptorSize()
{
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3DAppBase::CheckFeatureSupport()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = m_backBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(m_device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)
    ));
    m_4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(m_4xMsaaQuality > 0 && "Unexpected MSAA quality level");
}

void D3DAppBase::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = m_commandListType;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    ThrowIfFailed(m_device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&m_commandAllocator)));

    ThrowIfFailed(m_device->CreateCommandList(
        0, m_commandListType,
        m_commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList)
    ));

    // Start off in a closed state. This is because the first time we refer
    // to the command list we will Reset it, and it needs to be closed before
    // calling reset.
    m_commandList->Close();
}

void D3DAppBase::CreateSwapChain()
{
    // Release the previous swapchain we will be recreating.
    m_swapchain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = m_width;
    sd.BufferDesc.Height = m_height;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = m_backBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = m_swapChainBufferCount;
    sd.OutputWindow = m_hMainWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(m_factory->CreateSwapChain(
        m_commandQueue.Get(),
        &sd,
        &m_swapchain
    ));
}

void D3DAppBase::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = m_swapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_depthStencilBuffer)));
}

bool D3DAppBase::InitDirect3D()
{
    CreateFactory();
    CreateAdapter();
    CreateFenceObject();
    InitDescriptorSize();
    CheckFeatureSupport();
    CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

    return true;
}

void D3DAppBase::CreateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < m_swapChainBufferCount; i++)
    {
        ThrowIfFailed(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i])));
        m_device->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
    }
}

void D3DAppBase::CreateDepthStencilBufferAndView()
{
    // Create the depth/stencil buffer and view.
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_width;
    depthStencilDesc.Height = m_height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

    // Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
    // the depth buffer.  Therefore, because we need to create two views to the same resource:
    //   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
    //   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
    // we need to create the depth buffer resource with a typeless format.  
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = m_depthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&m_depthStencilBuffer)
    ));

    // Create descriptor to mip level 0 of entire resource using the format of the resource.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = m_depthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // Transition the resource from its initial state to be used as a depth buffer.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void D3DAppBase::FlushCommandQueue()
{
    // Advance the fence value to mark commands up to this fence point.
    m_currentFence++;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU time line, the new fence point won't be set
    // until the GPU finishes processing all the commands prior to this Signal().
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFence));

    // Wait until the GPU has completed commands up tp this fence point.
    if (m_fence->GetCompletedValue() < m_currentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFence, eventHandle));

        // Wait until the GPU hits current fence.
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void D3DAppBase::OnResize()
{
    assert(m_device);
    assert(m_swapchain);
    assert(m_commandAllocator);

    // Flush before changing any resources.
    FlushCommandQueue();

    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    // Release the previous resources we will be recreating.
    m_swapChainBuffer.resize(m_swapChainBufferCount);
    for (int i = 0; i < m_swapChainBufferCount; i++)
    {
        m_swapChainBuffer[i].Reset();
    }
    m_depthStencilBuffer.Reset();

    // Resize the swap chain.
    ThrowIfFailed(m_swapchain->ResizeBuffers(
        m_swapChainBufferCount,
        m_width, m_height,
        m_backBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    ));

    m_currentBackBufferIndex = 0;
    CreateRenderTargetViews();
    CreateDepthStencilBufferAndView();

    // Execute the resize commands.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Wait until resize is complete.
    FlushCommandQueue();

    // Update the viewport transform to cover the client area.
    m_screenViewport.TopLeftX = 0;
    m_screenViewport.TopLeftY = 0;
    m_screenViewport.Width = static_cast<float>(m_width);
    m_screenViewport.Height = static_cast<float>(m_height);
    m_screenViewport.MinDepth = 0.0f;
    m_screenViewport.MaxDepth = 1.0f;

    m_scissorRect = { 0,0,m_width,m_height };
}

bool D3DAppBase::Initialize()
{
    if (!InitMainWindow())
    {
        return false;
    }

    if (!InitDirect3D())
    {
        return false;
    }

    // Do the initial resize code.
    OnResize();

    return true;
}

std::wstring D3DAppBase::GetAssetFullPath(LPCTSTR assetName)
{
    return m_assetPath + assetName;
}

ID3D12Resource* D3DAppBase::CurrentBackBuffer()const
{
    return m_swapChainBuffer[m_currentBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DAppBase::CurrentBackBufferView()const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_currentBackBufferIndex,
        m_rtvDescriptorSize
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DAppBase::DepthStencilView()const
{
    return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DAppBase::CalculateFrameStats()
{
    // Code computes the average frames per second, and also the 
    // average time it takes to render one frame.  These stats 
    // are appended to the window caption bar.

    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    frameCnt++;

    // Compute averages over one second period.
    if ((m_gameTimer.TotalTime() - timeElapsed) >= 1.0f)
    {
        float fps = (float)frameCnt; // fps = frameCnt / 1
        float mspf = 1000.0f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);

        wstring windowText = m_mainWndCaption +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText(m_hMainWnd, windowText.c_str());

        // Reset for next average.
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}