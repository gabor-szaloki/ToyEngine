#include "ImGuiLogWindow.h"

#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <string>
#include <3rdParty/plog/Util.h>
#include <Common.h>

using namespace plog;

plog::ImGuiLogWindow plog::imguiLogWindow;

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
		return ImColor(255, 255, 255);
	case debug:
	case verbose:
	default:
		return ImColor(128, 128, 128);
	}
}

void ImGuiLogWindow::write(const Record& record)
{
	LogLine l;
	l.text = plog::util::toNarrow(ToyTxtFormatter::format(record).c_str(), CP_UTF8);
	l.severity = record.getSeverity();
	lines.push_back(l);
}

void ImGuiLogWindow::perform()
{
	ImGui::BeginMenuBar();
	if (ImGui::Button("Clear"))
		lines.clear();
	if (ImGui::Button("Show log file"))
	{
		std::string params;
		params += "/select,";
		std::string logFilePath(get_log_file_path());
		std::replace(logFilePath.begin(), logFilePath.end(), '/', '\\');
		params += logFilePath;
		ShellExecute((HWND)get_hwnd(), nullptr, "explorer.exe", params.c_str(), nullptr, SW_SHOWDEFAULT);
	}
	static bool autoScroll = true;
	ImGui::Checkbox("Auto-scroll", &autoScroll);
	static constexpr int NUM_SEVERITIES = verbose + 1;
	static std::array<bool, NUM_SEVERITIES> severityFilters;
	static bool severityFiltersInited = false;
	auto initSeverityFiltersTo = [&](bool to) { for (int i = 0; i <= get()->getMaxSeverity(); i++) severityFilters[i] = to; };
	if (!severityFiltersInited)
	{
		initSeverityFiltersTo(true);
		severityFiltersInited = true;
	}
	if (ImGui::BeginMenu("Filter severity"))
	{
		for (int i = 0; i < NUM_SEVERITIES; i++)
		{
			bool severityAvailable = false;
			IF_PLOG((Severity)i)
				severityAvailable = true;
			ImGui::MenuItem(severityToString((Severity)i), nullptr, &severityFilters[i], severityAvailable);
			if (!severityAvailable && ImGui::IsItemHovered())
				ImGui::SetTooltip("Max severity is initialized to %s.", severityToString(get()->getMaxSeverity()));
		}
		if (ImGui::Button("All"))
			initSeverityFiltersTo(true);
		ImGui::SameLine();
		if (ImGui::Button("None"))
			initSeverityFiltersTo(false);
		ImGui::EndMenu();
	}
	ImGui::EndMenuBar();

	for (LogLine& l : lines)
	{
		if (!severityFilters[l.severity])
			continue;
		ImGui::TextColored(get_severity_color(l.severity), l.text.c_str());
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(l.text.c_str());
	}

	if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);
}

REGISTER_IMGUI_WINDOW_EX("Log", nullptr, 100, ImGuiWindowFlags_MenuBar, [] { imguiLogWindow.perform(); });

util::nstring plog::ToyTxtFormatter::format(const Record& record)
{
	// Modified version of TxtFormatterImpl::format from TxtFormatter.h

	tm t;
	util::localtime_s(&t, &record.getTime().time);

	util::nostringstream ss;
	ss << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_sec << PLOG_NSTR(".") << std::setfill(PLOG_NSTR('0')) << std::setw(3) << record.getTime().millitm << PLOG_NSTR(" ");
	ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << severityToString(record.getSeverity()) << PLOG_NSTR(" ");
	ss << record.getMessage() << PLOG_NSTR("\n");

	return ss.str();
}