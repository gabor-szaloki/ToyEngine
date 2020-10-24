#pragma once

#include <3rdParty/plog/Appenders/IAppender.h>
#include <3rdParty/imgui/imgui.h>
#include <string>
#include <vector>

namespace plog
{
	class ImGuiLogWindow : public IAppender
	{
	public:
		void write(const Record& record) override;
		void perform();
	private:
		struct LogLine
		{
			std::string text;
			ImColor color;
		};
		std::vector<LogLine> lines;
	};

	class ToyTxtFormatter
	{
	public:
		static util::nstring header() { return util::nstring(); };
		static util::nstring format(const Record& record);
	};

	extern ImGuiLogWindow imguiLogWindow;
}