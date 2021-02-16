#include "AutoImGui.h"

#include <cstring>
#include <map>
#include <3rdParty/imgui/imgui.h>
#include <3rdParty/imgui/implot.h>
#include <Util/CaseSensitiveIni.h>

#include "Common.h"

using namespace autoimgui;

bool autoimgui::is_active = true;

static mINI::INIFile ini_file("toyimgui.ini");
static mINI::INIStructure ini;

void autoimgui::init()
{
	bool success = ini_file.read(ini);
	if (!success)
		ini_file.generate(ini, true);
}

void autoimgui::shutdown()
{
}

bool autoimgui::is_window_opened(const std::string& window_name)
{
	return ini["windows"][window_name] == "open";
}

void autoimgui::set_window_opened(const std::string& window_name, bool opened)
{
	ini["windows"][window_name] = opened ? "open" : "closed";
	ini_file.write(ini, true);
}

void autoimgui::save_custom_param(const std::string& key, const std::string& value)
{
	ini["custom"][key] = value;
	ini_file.write(ini, true);
}

std::string autoimgui::load_custom_param(const std::string& key, const std::string& default_value)
{
	return ini["custom"].has(key) ? ini["custom"][key] : default_value;
}

void autoimgui::perform()
{
	if (!is_active)
		return;

	const char* imGuiDemoWindowName = "Dear ImGui Demo window";
	const char* imPlotDemoWindowName = "ImPlot Demo window";

	// Construct main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		// Function queue
		if (ImGuiFunctionQueue::functionHead != nullptr)
		{
			const char* currentGroup = ImGuiFunctionQueue::functionHead->group;
			bool currentGroupOpened = ImGui::BeginMenu(currentGroup);
			for (ImGuiFunctionQueue* q = ImGuiFunctionQueue::functionHead; q; q = q->next)
			{
				if (_stricmp(currentGroup, q->group) != 0)
				{
					if (currentGroupOpened)
						ImGui::EndMenu();
					currentGroup = q->group;
					currentGroupOpened = ImGui::BeginMenu(currentGroup);
				}
				if (currentGroupOpened)
				{
					if (ImGui::MenuItem(q->name, q->hotkey))
					{
						assert(q->function);
						q->function();
					}
				}
			}
			if (currentGroupOpened)
				ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ImGui"))
		{
			ImGui::Separator();
			bool showImGuiDemoWindow = is_window_opened(imGuiDemoWindowName);
			bool oldShowImGuiDemoWindow = showImGuiDemoWindow;
			ImGui::MenuItem(imGuiDemoWindowName, nullptr, &showImGuiDemoWindow);
			if (showImGuiDemoWindow != oldShowImGuiDemoWindow)
				set_window_opened(imGuiDemoWindowName, showImGuiDemoWindow);
			bool showImPlotDemoWindow = is_window_opened(imPlotDemoWindowName);
			bool oldShowImPlotDemoWindow = showImPlotDemoWindow;
			ImGui::MenuItem(imPlotDemoWindowName, nullptr, &showImPlotDemoWindow);
			if (showImPlotDemoWindow != oldShowImPlotDemoWindow)
				set_window_opened(imPlotDemoWindowName, showImPlotDemoWindow);
			ImGui::EndMenu();
		}

		// Window queue
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGuiFunctionQueue::windowHead != nullptr)
			{
				for (ImGuiFunctionQueue* q = ImGuiFunctionQueue::windowHead; q; q = q->next)
				{
					q->opened = is_window_opened(q->name);
					bool oldOpened = q->opened;
					ImGui::MenuItem(q->name, q->hotkey, &q->opened);
					if (q->opened != oldOpened)
						set_window_opened(q->name, q->opened);
				}
			}
			ImGui::EndMenu();
		}

		// Exit button to the right
		ImGui::SameLine(ImGui::GetWindowWidth() - 40);
		if (ImGui::Button("x", ImVec2(32, 0)))
			exit_program();

		ImGui::EndMainMenuBar();
	}

	// Execute window functions
	for (ImGuiFunctionQueue* q = ImGuiFunctionQueue::windowHead; q; q = q->next)
	{
		q->opened = is_window_opened(q->name);
		if (q->opened)
		{
			assert(q->function);
			bool oldOpened = q->opened;
			ImGui::Begin(q->name, &q->opened, q->windowFlags);
			if (q->opened != oldOpened)
				set_window_opened(q->name, q->opened);
			q->function();
			ImGui::End();
		}
	}

	// Show demo windows
	{
		bool showImGuiDemoWindow = is_window_opened(imGuiDemoWindowName);
		if (showImGuiDemoWindow)
		{
			bool oldShowImGuiDemoWindow = showImGuiDemoWindow;
			ImGui::ShowDemoWindow(&showImGuiDemoWindow);
			if (showImGuiDemoWindow != oldShowImGuiDemoWindow)
				set_window_opened(imGuiDemoWindowName, showImGuiDemoWindow);
		}
		bool showImPlotDemoWindow = is_window_opened(imPlotDemoWindowName);
		if (showImPlotDemoWindow)
		{
			bool oldShowImPlotDemoWindow = showImPlotDemoWindow;
			ImPlot::ShowDemoWindow(&showImPlotDemoWindow);
			if (showImPlotDemoWindow != oldShowImPlotDemoWindow)
				set_window_opened(imPlotDemoWindowName, showImPlotDemoWindow);
		}
	}
}

ImGuiFunctionQueue* ImGuiFunctionQueue::windowHead = nullptr;
ImGuiFunctionQueue* ImGuiFunctionQueue::functionHead = nullptr;

ImGuiFunctionQueue::ImGuiFunctionQueue(
	const char* group_, const char* name_, const char* hotkey_, int priority_, unsigned int window_flags, ImGuiFuncPtr func, bool is_window)
	: group(group_), name(name_), hotkey(hotkey_), priority(priority_), windowFlags(window_flags), function(func)
{
	ImGuiFunctionQueue** head = is_window ? &windowHead : &functionHead;
	if (*head == nullptr)
	{
		*head = this;
		return;
	}

	ImGuiFunctionQueue* n = *head, * p = nullptr;
	for (; n; p = n, n = n->next)
	{
		int cmp = is_window ? 0 : _stricmp(group, n->group);
		if (cmp < 0 || (cmp == 0 && priority < n->priority))
		{
			// insert before
			next = n;
			if (p)
				p->next = this;
			else
				*head = this;
			return;
		}
	}
	p->next = this;
}