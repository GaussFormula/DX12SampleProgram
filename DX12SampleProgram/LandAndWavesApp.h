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
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void UpdateCamera(const GameTimer& gt);
    void UpdateObjectConstantBuffers(const GameTimer& gt);
};