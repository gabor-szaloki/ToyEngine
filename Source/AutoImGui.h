#pragma once

namespace autoimgui
{
    extern bool is_active;

    void init();
    void shutdown();
    bool is_window_opened(const char *window_name);
    void set_window_opened(const char *window_name, bool opened);
    void perform();

    typedef void (*ImGuiFuncPtr)();

    struct ImGuiFunctionQueue
    {
        ImGuiFunctionQueue* next = nullptr; // single-linked list
        ImGuiFuncPtr function = nullptr;
        const char* group = nullptr;
        const char* name = nullptr;
        const char* hotkey = nullptr;
        int priority = 0; // lower the number, earlier it will be in the list
        unsigned int windowFlags = 0;
        bool opened = false;
        static ImGuiFunctionQueue* windowHead;
        static ImGuiFunctionQueue* functionHead;
        ImGuiFunctionQueue(
            const char* group_,
            const char* name_,
            const char* hotkey_,
            int priority_,
            unsigned int window_flags,
            ImGuiFuncPtr func,
            bool is_window);
    };
}

#define TOY_IGQ_CC0(a, b) a##b
#define TOY_IGQ_CC1(a, b) TOY_IGQ_CC0(a, b)
#define REGISTER_IMGUI_WINDOW(name, func)\
  static autoimgui::ImGuiFunctionQueue TOY_IGQ_CC1(AutoImGuiWindow, __LINE__)(nullptr, name, nullptr, 100, 0, func, true)
#define REGISTER_IMGUI_WINDOW_EX(name, hotkey, priority, flags, func)\
  static autoimgui::ImGuiFunctionQueue TOY_IGQ_CC1(AutoImGuiWindow, __LINE__)(nullptr, name, hotkey, priority, flags, func, true)
#define REGISTER_IMGUI_FUNCTION(group, name, func)\
  static autoimgui::ImGuiFunctionQueue TOY_IGQ_CC1(AutoImGuiFunction, __LINE__)(group, name, nullptr, 100, 0, func, false)
#define REGISTER_IMGUI_FUNCTION_EX(group, name, hotkey, priority, func)\
  static autoimgui::ImGuiFunctionQueue TOY_IGQ_CC1(AutoImGuiFunction, __LINE__)(group, name, hotkey, priority, 0, func, false)