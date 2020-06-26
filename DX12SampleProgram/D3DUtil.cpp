#include "D3DUtil.h"
#include "stdafx.h"
#include <comdef.h>
using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber):
    ErrorCode(hr),
    FunctionName(functionName),
    FileName(filename),
    LineNumber(lineNumber)
{}

std::wstring DxException::ToString()const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + FileName + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}