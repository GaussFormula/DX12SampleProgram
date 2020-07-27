#include "stdafx.h"
#include "BoxApp.h"
#include "ShapesApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG)|(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
#if IS_ENABLE_SHAPE_APP
		ShapesApp theApp(hInstance);
#else
		BoxApp theApp(hInstance);
#endif
		if (!theApp.Initialize())
		{
			return 0;
		}
		return theApp.Run();
	}
	catch (DxException&e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR failed", MB_OK);
		return 0;
	}
}