#pragma once
#include "stdafx.h"
#include "D3DAppBase.h"
#include "UploadBuffer.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const unsigned int gNumFrameResources = 3;

// Light weight structure stores parameters to draw a shape.
// This will vary from app-to-app.

class RenderItem
{
public:
    RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, whick defines the position, orientation,
    // and scale of the object in the world .
    XMMATRIX World = DirectX::XMMatrixIdentity();
};