#pragma once
#if defined(DEBUG)||defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
#include "D3DUtil.h"
#include "GameTimer.h"

class D3DAppBase
{
protected:
    D3DAppBase(HINSTANCE hInstance);
    D3DAppBase(const D3DAppBase& rhs) = delete;
    D3DAppBase& operator=(const D3DAppBase& rhs) = delete;
    virtual ~D3DAppBase();

public:
    static D3DAppBase* GetApp();

    HINSTANCE GetAppInstance()const;
    HWND GetMainWnd()const;
    float GetAspectRatio()const;

    bool Get4xMsaaState()const;
    void Set4xMsaaState(const bool value);

    int Run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void CreateRtvAndDsvDescriptorHeaps();
    virtual void OnResize();
    virtual void Update(const GameTimer& gt) = 0;
    virtual void Draw(const GameTimer& gt) = 0;

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y){}
    virtual void OnMouseUp(WPARAM btnState,int x,int y){}
    virtual void OnMouseMove(WPARAM btnState,int x, int y){}

protected:
    bool InitMainWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();

    void FlushCommandQueue();

    void CalculateFrameStats();

    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    ID3D12Resource* m_currentBackBuffer = nullptr;

protected:
    static D3DAppBase* m_app;   

    HINSTANCE m_hAppInstance = nullptr; // application instance handle.
    HWND    m_hMainWnd;                 // main window handle.
    bool m_appPaused = false;    // is the application paused?
    bool m_minimized = false;   // is the application minimized?
    bool m_maximized = false;   // is the application maximized?
    bool m_resizing = false;    // are the resize bars being dragged?
    bool m_fullscreenState = false; // fullscreen state.

    ComPtr<ID3D12Device>    m_device = nullptr;

    UINT m_width = 0;
    UINT m_height = 0;

    // Set true to use 4X MSAA. The default is false.
    bool m_4xMsaaState = false; // 4X MSAA enabled.
    UINT m_4xMsaaQuality = 0;   // quality level of 4X MSAA.

    GameTimer m_gameTimer;

    ComPtr<IDXGIFactory4>   m_factory;
    ComPtr<IDXGISwapChain>  m_swapchain;
    ComPtr<ID3D12Device>    m_device;

    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    UINT m_swapChainBufferCount = 2;
    UINT m_currentBackBuffer = 0;
    std::vector<ComPtr<ID3D12Resource>> m_swapChainBuffer;
    ComPtr<ID3D12Resource>  m_depthStencilBuffer;

    ComPtr<ID3D12DescriptorHeap>    m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap>    m_dsvHeap;

    D3D12_VIEWPORT m_screenViewport;
    D3D12_RECT  m_scissorRect;
};