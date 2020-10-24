#include "ImGuiLogWindow.h"

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
	case debug:
		return ImColor(255, 255, 255);
	case verbose:
	default:
		return ImColor(128, 128, 128);
	}
}

void ImGuiLogWindow::write(const Record& record)
{
	LogLine l;
	constexpr unsigned int CP_UTF8 = 65001; // This is normally defined in some windows header, but didn't want to include it just for this.
	l.text = plog::util::toNarrow(ToyTxtFormatter::format(record).c_str(), CP_UTF8);
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
		ImGui::TextColored(l.color, l.text.c_str());

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
	//ss << t.tm_year + 1900 << "-" << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mon + 1 << PLOG_NSTR("-") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mday << PLOG_NSTR(" ");
	ss << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min << PLOG_NSTR(":") << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_sec << PLOG_NSTR(".") << std::setfill(PLOG_NSTR('0')) << std::setw(3) << record.getTime().millitm << PLOG_NSTR(" ");
	ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << severityToString(record.getSeverity()) << PLOG_NSTR(" ");
	//ss << PLOG_NSTR("[") << record.getTid() << PLOG_NSTR("] ");
	//ss << PLOG_NSTR("[") << record.getFunc() << PLOG_NSTR("@") << record.getLine() << PLOG_NSTR("] ");
	ss << record.getMessage() << PLOG_NSTR("\n");

	return ss.str();
}