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