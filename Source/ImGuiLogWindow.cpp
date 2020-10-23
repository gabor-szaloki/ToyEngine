#include "ImGuiLogWindow.h"

#include <vector>
#include <string>
#include <Common.h>

using namespace plog;

plog::ImGuiLogWindow plog::imguiLogWindow;

ImGuiLogWindow::~ImGuiLogWindow()
{
	for (LogLine& l : lines)
		delete l.text;
	lines.clear();
}

static ImColor get_severity_color(plog::Severity severity)
{
	switch (severity)
	{
	case fatal:
	case error:
		return ImColor(255, 0, 0);
	case warning:
		return ImColor(255, 255, 0);
	case info:
	case debug:
		return ImColor(255, 255, 255);
	case verbose:
	default:
		return ImColor(128, 128, 128);
	}
}

void ImGuiLogWindow::write(const Record& record)
{
	static char buf[1024];
	wcs_to_utf8(record.getMessage(), buf, sizeof(buf));
	LogLine l;
	l.text = _strdup(buf);
	l.color = get_severity_color(record.getSeverity());
	lines.push_back(l);
}

void ImGuiLogWindow::perform()
{
	ImGui::BeginMenuBar();
	static bool autoScroll = true;
	ImGui::Checkbox("Auto-scroll", &autoScroll);
	ImGui::EndMenuBar();

	for (LogLine& l : lines)
		ImGui::TextColored(l.color, l.text);

	if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);
}

REGISTER_IMGUI_WINDOW_EX("Log", nullptr, 100, ImGuiWindowFlags_MenuBar, [] { imguiLogWindow.perform(); });