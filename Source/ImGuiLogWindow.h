#pragma once

#include <3rdParty/plog/Appenders/IAppender.h>
#include <3rdParty/imgui/imgui.h>
#include <vector>

namespace plog
{
	class ImGuiLogWindow : public IAppender
	{
	public:
		~ImGuiLogWindow() override;
		void write(const Record& record) override;
		void perform();
	private:
		struct LogLine
		{
			const char* text;
			ImColor color;
		};

		std::vector<LogLine> lines;
	};

	extern ImGuiLogWindow imguiLogWindow;
}