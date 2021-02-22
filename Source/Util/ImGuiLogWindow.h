#pragma once

#include <3rdParty/plog/Appenders/IAppender.h>
#include <3rdParty/imgui/imgui.h>
#include <string>
#include <vector>
#include <array>
#include <mutex>

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
			Severity severity;
		};
		std::vector<LogLine> lines;
		std::mutex linesMutex;
		unsigned int errorCount = 0;
		unsigned int warningCount = 0;
	};

	class ToyTxtFormatter
	{
	public:
		static util::nstring header() { return util::nstring(); };
		static util::nstring format(const Record& record);
	};

	extern ImGuiLogWindow imguiLogWindow;
}