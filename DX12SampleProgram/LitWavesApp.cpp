#include "stdafx.h"
#include "LitWavesApp.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

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