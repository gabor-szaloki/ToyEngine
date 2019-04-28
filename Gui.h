#pragma once

#include <windows.h>
#include "Common.h"

class Gui
{
public:
	
	Gui();
	~Gui();

	void Init(HWND hWnd, ID3D11Device *device, ID3D11DeviceContext *context);
	void Release();
	void Update();
	void Render();

	bool IsActive() { return guiState.enabled; }
	void SetActive(bool active) { guiState.enabled = active; }

private:
	
	HWND hWnd;

	struct GuiState
	{
		bool enabled;
		bool showDemoWindow;
		bool showEngineSettingsWindow;
		bool showLightSettingsWindow;
		bool showStatsWindow;
		bool showShadowmapDebugWindow;
	};
	GuiState guiState;
};

