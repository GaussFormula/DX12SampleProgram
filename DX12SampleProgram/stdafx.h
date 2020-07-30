#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wrl.h>

#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
#include "d3dx12.h"

#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <stdexcept>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <array>
#include <vector>

const unsigned int gNumFrameResources = 3;