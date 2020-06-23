#include "D3DAppBase.h"

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

D3DAppBase::D3DAppBase(HINSTANCE hInstance):m_hMainWnd(hInstance)
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