#pragma once

#include <3rdParty/plog/Appenders/IAppender.h>
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
		std::vector<char*> logMessages;
	};

	extern ImGuiLogWindow imguiLogWindow;
}