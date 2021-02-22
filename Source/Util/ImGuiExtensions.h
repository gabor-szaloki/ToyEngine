#pragma once

#include <cmath>
#include <3rdParty/imgui/imgui.h>
#include <Common.h>

namespace ImGui
{
	inline bool ColorEdit3Srgb(const char* label, float col[3], ImGuiColorEditFlags flags = 0)
	{
		for (int i = 0; i < 3; i++)
			col[i] = powf(col[i], 1.0f / 2.2f);
		bool result = ColorEdit3(label, col, flags);
		for (int i = 0; i < 3; i++)
			col[i] = powf(col[i], 2.2f);
		return result;
	}

	inline bool DragAngle3(const char* label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.0f deg", ImGuiSliderFlags flags = 0)
	{
		float vDeg[3];
		for (int i = 0; i < 3; i++)
			vDeg[i] = to_deg(v[i]);
		bool changed = DragFloat3(label, vDeg, v_speed, v_min, v_max, format, flags);
		if (changed)
			for (int i = 0; i < 3; i++)
				v[i] = to_rad(vDeg[i]);
		return changed;
	}
}