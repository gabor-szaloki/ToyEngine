#include "ImGuiLogWindow.h"

#include <vector>
#include <string>
#include <3rdParty/imgui/imgui.h>
#include <Common.h>

using namespace plog;

plog::ImGuiLogWindow plog::imguiLogWindow;

ImGuiLogWindow::~ImGuiLogWindow()
{
	for (char* msg : logMessages)
		delete msg;
	logMessages.clear();
}

void ImGuiLogWindow::write(const Record& record)
{
	static char buf[1024];
	wcs_to_utf8(record.getMessage(), buf, sizeof(buf));
	logMessages.push_back(_strdup(buf));
}

void ImGuiLogWindow::perform()
{
	for (char* msg : logMessages)
		ImGui::Text(msg);
}

static void perform_imgui_log_window()
{
	imguiLogWindow.perform();
}

REGISTER_IMGUI_WINDOW("Log", perform_imgui_log_window);