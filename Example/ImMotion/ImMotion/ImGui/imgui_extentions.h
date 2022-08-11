#pragma once
#include "imgui.h"
#include "imgui_internal.h"

namespace ImExt
{
	IMGUI_API bool Button(const char* label, const ImVec2& size = ImVec2(NULL, NULL), const float dt = 1.0f, ImGuiButtonFlags flags = NULL);
	IMGUI_API bool ProgressButton(const char* label, bool* v, float* v_progress, const ImVec2& size = ImVec2(NULL, NULL), const float v_speed = 0.01f, const float dt = 1.0f, ImGuiButtonFlags flags = NULL);
	IMGUI_API bool ToggleButton(const char* label, bool* v, const ImVec2& size = { 0.f, 0.f }, const float dt = 1.0f, ImGuiButtonFlags flags = NULL);
	IMGUI_API bool ProgressToggleButton(const char* label, bool* v, float* v_progress, const ImVec2& size = ImVec2(NULL, NULL), const float v_speed = 0.01f, const float dt = 1.0f, ImGuiButtonFlags flags = NULL);
	IMGUI_API bool ToggleSwitch(const char* label, bool* v, const float dt = 1.0f);
	IMGUI_API bool RadioButton(const char* label, bool active, const float dt = 1.0f);
	IMGUI_API bool RadioButton(const char* label, int* v, int v_button, const float dt = 1.0f);

	IMGUI_API bool Checkbox(const char* label, bool* v, const float dt = 1.0f);

	namespace ImDraw
	{
        IMGUI_API void RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImU32 color, const ImVec2& align = ImVec2(0, 0), const ImRect* clip_rect = NULL);
        IMGUI_API void RenderTextClippedEx(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImU32 color, const ImVec2& align = ImVec2(0, 0), const ImRect* clip_rect = NULL);
	}
}