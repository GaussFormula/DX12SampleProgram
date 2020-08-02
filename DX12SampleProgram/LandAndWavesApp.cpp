#include "stdafx.h"
#ifndef IS_ENABLE_LAND_APP
#define IS_ENABLE_LAND_APP 0
#endif // !IS_ENABLE_LAND_APP

#if IS_ENABLE_LAND_APP
#include "LandAndWavesApp.h"

LandAndWavesApp::LandAndWavesApp(HINSTANCE hInstance)
    :D3DAppBase(hInstance)
{}
#endif