#pragma once
#include "stdafx.h"
#include "UploadBuffer.h"

struct ObjectConstants
{
    DirectX::XMMATRIX World = DirectX::XMMatrixIdentity();
};

struct PassConstants
{
    DirectX::XMMATRIX View = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX InvView = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX Proj = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX InvProj = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX ViewProj = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX InvViewProj = DirectX::XMMatrixIdentity();
    DirectX::XMFLOAT3 EyePosW = { 0.0f,0.0f,0.0f };
};