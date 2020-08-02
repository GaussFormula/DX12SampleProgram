#pragma once
#include "stdafx.h"
#include "D3DAppBase.h"
#include "GeometryGenerator.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "Waves.h"


class LandAndWavesApp :public D3DAppBase
{
public:
    LandAndWavesApp(HINSTANCE hInstance);
    LandAndWavesApp(const LandAndWavesApp& rhs) = delete;
    LandAndWavesApp& operator=(const LandAndWavesApp& rhs) = delete;
    ~LandAndWavesApp();

    virtual bool Initialize() override;

private:
    virtual void OnResize()override;
};