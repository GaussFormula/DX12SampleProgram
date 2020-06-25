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
    wc.lpszClassName = "MainWnd";
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