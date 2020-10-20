#include "Common.h"

IDriver* drv;

void ThrowIfFailed(HRESULT hr, const char *errorMsg)
{
	if (FAILED(hr))
	{
		if (errorMsg != nullptr)
			OutputDebugString(errorMsg);
		throw;
	}
}
