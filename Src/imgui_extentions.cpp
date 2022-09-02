#include "imgui_extentions.h"

#include <imgui.h>
#include <imgui_internal.h>

using namespace ImGui;

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

static const char* PatchFormatStringFloatToInt(const char* fmt)
{
	if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
		return "%d";
	const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
	const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
	if (fmt_end > fmt_start && fmt_end[-1] == 'f')
	{
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
		if (fmt_start == fmt && fmt_end[0] == 0)
			return "%d";
		const char* tmp_format;
		ImFormatStringToTempBuffer(&tmp_format, NULL, "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
		return tmp_format;
#else
		IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please replace with e.g. "%d"
#endif
	}
	return fmt;
}

#pragma region ImDraw
void ImExt::ImDraw::RenderTextClippedEx(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_display_end, const ImVec2* text_size_if_known, const ImU32 color, const ImVec2& align, const ImRect* clip_rect)
{
	// Perform CPU side clipping for single clipped element to avoid using scissor state
	ImVec2 pos = pos_min;
	const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_display_end, false, 0.0f);

	const ImVec2* clip_min = clip_rect ? &clip_rect->Min : &pos_min;
	const ImVec2* clip_max = clip_rect ? &clip_rect->Max : &pos_max;
	bool need_clipping = (pos.x + text_size.x >= clip_max->x) || (pos.y + text_size.y >= clip_max->y);
	if (clip_rect) // If we had no explicit clipping rectangle then pos==clip_min
		need_clipping |= (pos.x < clip_min->x) || (pos.y < clip_min->y);

	// Align whole block. We should defer that to the better rendering function when we'll have support for individual line alignment.
	if (align.x > 0.0f) pos.x = ImMax(pos.x, pos.x + (pos_max.x - pos.x - text_size.x) * align.x);
	if (align.y > 0.0f) pos.y = ImMax(pos.y, pos.y + (pos_max.y - pos.y - text_size.y) * align.y);

	// Render
	if (need_clipping)
	{
		ImVec4 fine_clip_rect(clip_min->x, clip_min->y, clip_max->x, clip_max->y);
		draw_list->AddText(NULL, 0.0f, pos, color, text, text_display_end, 0.0f, &fine_clip_rect);
	}
	else
	{
		draw_list->AddText(NULL, 0.0f, pos, color, text, text_display_end, 0.0f, NULL);
	}
}

void ImExt::ImDraw::RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImU32 color, const ImVec2& align, const ImRect* clip_rect)
{
	// Hide anything after a '##' string
	const char* text_display_end = FindRenderedTextEnd(text, text_end);
	const int text_len = (int)(text_display_end - text);
	if (text_len == 0)
		return;

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	RenderTextClippedEx(window->DrawList, pos_min, pos_max, text, text_display_end, text_size_if_known, color, align, clip_rect);
	if (g.LogEnabled)
		LogRenderedText(&pos_min, text, text_display_end);
}
#pragma endregion

bool ImExt::Button(const char* label, const ImVec2& size, const float dt, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 item_size = CalcItemSize(ImVec2(size.x, size.y + label_size.y),
		label_size.x + style.FramePadding.x * 2.0f,
		label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, ImVec2(pos.x + item_size.x, pos.y + item_size.y));
	ItemSize(item_size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
	if (pressed)
		MarkItemEdited(id);

	//Animation
	float last_active_id_timer = g.LastActiveIdTimer;
	if (held || pressed)
		g.LastActiveIdTimer = 0.f;

	if (g.LastActiveIdTimer == 0.f && g.LastActiveId == id && !pressed)
		g.LastActiveIdTimer = last_active_id_timer;

	float t = held ? 1.0f : 0.0f;

	if (g.LastActiveId == id)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / 0.08f * dt);
		t = held ? (t_anim) : (1.0f - t_anim);
	}

	const float scale = item_size.x / 30.f * t;
	const ImRect render_bb = ImRect(ImVec2(pos.x + (scale / 2), pos.y + (scale / 2 * (item_size.y / item_size.x))), ImVec2(pos.x + (item_size.x - scale), pos.y + (item_size.y - (scale * (item_size.y / item_size.x)))));

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	const ImVec2 pos_min = ImVec2(render_bb.Min.x + style.FramePadding.x / 2, render_bb.Min.y + style.FramePadding.y / 2);
	const ImVec2 pos_max = ImVec2(render_bb.Max.x - style.FramePadding.x, render_bb.Max.y - style.FramePadding.y);
	RenderFrame(pos_min, pos_max, col, true, style.FrameRounding);

	if (g.LogEnabled)
		LogSetNextTextDecoration("[", "]");
	RenderTextClipped(pos_min, pos_max, label, NULL, &label_size, style.ButtonTextAlign, &render_bb);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}

IMGUI_API bool ImExt::ProgressButton(const char* label, bool* v, float* v_progress, const ImVec2& size, const float v_speed, const float dt, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 item_size = CalcItemSize(ImVec2(size.x, size.y + label_size.y),
		label_size.x + style.FramePadding.x * 2.0f,
		label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, ImVec2(pos.x + item_size.x, pos.y + item_size.y));
	ItemSize(item_size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
	if (pressed)
		MarkItemEdited(id);

	//Animation
	float last_active_id_timer = g.LastActiveIdTimer;
	if (held || pressed)
		g.LastActiveIdTimer = 0.f;

	if (g.LastActiveIdTimer == 0.f && g.LastActiveId == id && !pressed)
		g.LastActiveIdTimer = last_active_id_timer;

	float t = held ? 1.0f : 0.0f;

	if (g.LastActiveId == id)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / 0.08f * dt);
		t = held ? (t_anim) : (1.0f - t_anim);
	}

	const float scale = item_size.x / 30.f * t;
	const ImRect render_bb = ImRect(ImVec2(pos.x + (scale / 2), pos.y + (scale / 2 * (pos.y / pos.x))), ImVec2(pos.x + (item_size.x - scale), pos.y + (item_size.y - (scale * (item_size.y / item_size.x)))));

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	const ImVec2 pos_min = ImVec2(render_bb.Min.x + style.FramePadding.x / 2, render_bb.Min.y + style.FramePadding.y / 2);
	const ImVec2 pos_max = ImVec2(render_bb.Max.x - style.FramePadding.x, render_bb.Max.y - style.FramePadding.y);
	RenderFrame(pos_min, pos_max, col, true, style.FrameRounding);

	if (g.LogEnabled)
		LogSetNextTextDecoration("[", "]");
	RenderTextClipped(pos_min, pos_max, label, NULL, &label_size, style.ButtonTextAlign, &render_bb);

	float progress_size = pos_min.x;
	if (v && v_progress)
	{
		if (*v_progress == 1.0f && pressed)
			*v = !*v;

		*v_progress = held ? (*v_progress < 1.0f ? *v_progress + v_speed : 1.0f) : 0.0f;

		progress_size += *v_progress * (pos_max.x - pos_min.x);
		if (*v_progress > 0.0f)
		{
			ImColor frame_color = ImColor(0.5f + (*v_progress) / 2.f, 0.5f + (*v_progress) / 2.f, 0.5f + (*v_progress) / 2.f, *v_progress);
			ImColor text_color = ImColor(1.f - frame_color.Value.x, 1.f - frame_color.Value.y, 1.f - frame_color.Value.z, *v_progress);
			RenderFrame(pos_min, ImVec2(progress_size, pos_max.y), frame_color, true, style.FrameRounding);
			ImDraw::RenderTextClipped(pos_min, pos_max, label, NULL, &label_size, text_color, style.ButtonTextAlign, &render_bb);
		}
	}

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}

IMGUI_API bool ImExt::ToggleButton(const char* label, bool* v, const ImVec2& size, const float dt, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 item_size = CalcItemSize(ImVec2(size.x, size.y + label_size.y),
		label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, ImVec2(pos.x + item_size.x, pos.y + item_size.y));
	ItemSize(item_size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}

	// Animation
	float last_active_id_timer = g.LastActiveIdTimer;
	if (held || pressed)
		g.LastActiveIdTimer = 0.f;

	if (g.LastActiveIdTimer == 0.f && g.LastActiveId == id && !pressed)
		g.LastActiveIdTimer = last_active_id_timer;

	float t = held ? 1.0f : 0.0f;

	if (g.LastActiveId == id)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / 0.08f * dt);
		t = held ? (t_anim) : (1.0f - t_anim);
	}

	const float scale = item_size.x / 30.f * t;
	const ImRect render_bb = ImRect(ImVec2(pos.x + (scale / 2), pos.y + (scale / 2 * (item_size.y / item_size.x))), ImVec2(pos.x + (item_size.x - scale), pos.y + (item_size.y - (scale * (item_size.y / item_size.x)))));

	// Render
	const ImU32 col = *v ? GetColorU32(ImGuiCol_ButtonActive) : GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	const ImVec2 pos_min = ImVec2(render_bb.Min.x + style.FramePadding.x / 2, render_bb.Min.y + style.FramePadding.y / 2);
	const ImVec2 pos_max = ImVec2(render_bb.Max.x - style.FramePadding.x, render_bb.Max.y - style.FramePadding.y);
	RenderFrame(pos_min, pos_max, col, true, style.FrameRounding);

	if (g.LogEnabled)
		LogSetNextTextDecoration("[", "]");
	RenderTextClipped(pos_min, pos_max, label, NULL, &label_size, style.ButtonTextAlign, &render_bb);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}

IMGUI_API bool ImExt::ProgressToggleButton(const char* label, bool* v, float* v_progress, const ImVec2& size, const float v_speed, const float dt, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 item_size = CalcItemSize(ImVec2(size.x, size.y + label_size.y),
		label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, ImVec2(pos.x + item_size.x, pos.y + item_size.y));
	ItemSize(item_size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
	if (pressed)
		MarkItemEdited(id);

	// Animation
	float last_active_id_timer = g.LastActiveIdTimer;
	if (held || pressed)
		g.LastActiveIdTimer = 0.f;

	if (g.LastActiveIdTimer == 0.f && g.LastActiveId == id && !pressed)
		g.LastActiveIdTimer = last_active_id_timer;

	float t = held ? 1.0f : 0.0f;

	if (g.LastActiveId == id)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / 0.08f * dt);
		t = held ? (t_anim) : (1.0f - t_anim);
	}

	const float scale = item_size.x / 30.f * t;
	const ImRect render_bb = ImRect(ImVec2(pos.x + (scale / 2), pos.y + (scale / 2 * (item_size.y / item_size.x))), ImVec2(pos.x + (item_size.x - scale), pos.y + (item_size.y - (scale * (item_size.y / item_size.x)))));

	// Render
	const ImU32 col = *v ? GetColorU32(ImGuiCol_ButtonActive) : GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	const ImVec2 pos_min = ImVec2(render_bb.Min.x + style.FramePadding.x / 2, render_bb.Min.y + style.FramePadding.y / 2);
	const ImVec2 pos_max = ImVec2(render_bb.Max.x - style.FramePadding.x, render_bb.Max.y - style.FramePadding.y);
	RenderFrame(pos_min, pos_max, col, true, style.FrameRounding);

	if (g.LogEnabled)
		LogSetNextTextDecoration("[", "]");
	RenderTextClipped(pos_min, pos_max, label, NULL, &label_size, style.ButtonTextAlign, &render_bb);

	float progress_size = pos_min.x;
	if (v && v_progress)
	{
		if (*v_progress == 1.0f && pressed)
			*v = !*v;

		*v_progress = held ? (*v_progress < 1.0f ? *v_progress + v_speed : 1.0f) : 0.0f;

		progress_size += *v_progress * (pos_max.x - pos_min.x);
		if (*v_progress > 0.0f)
		{
			ImColor frame_color = ImColor(0.5f + (*v_progress) / 2.f, 0.5f + (*v_progress) / 2.f, 0.5f + (*v_progress) / 2.f, *v_progress);
			ImColor text_color = ImColor(1.f - frame_color.Value.x, 1.f - frame_color.Value.y, 1.f - frame_color.Value.z, *v_progress);
			RenderFrame(pos_min, ImVec2(progress_size, pos_max.y), frame_color, true, style.FrameRounding);
			ImDraw::RenderTextClipped(pos_min, pos_max, label, NULL, &label_size, text_color, style.ButtonTextAlign, &render_bb);
		}
	}

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}

IMGUI_API bool ImExt::ToggleSwitch(const char* label, bool* v, const float dt)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	float height = ImGui::GetFrameHeight();
	const ImVec2 pos = window->DC.CursorPos;

	float width = height * 2.f;
	float radius = height * 0.50f;

	const ImRect total_bb(pos, ImVec2(pos.x + width + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), pos.y + label_size.y + style.FramePadding.y * 2.0f));

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	float last_active_id_timer = g.LastActiveIdTimer;

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
		g.LastActiveIdTimer = 0.f;
	}

	if (g.LastActiveIdTimer == 0.f && g.LastActiveId == id && !pressed)
		g.LastActiveIdTimer = last_active_id_timer;

	float t = *v ? 1.0f : 0.0f;
	float circle_t = held ? 1.0f : 0.0f;

	if (g.LastActiveId == id)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / 0.16f * dt);
		t = *v ? (t_anim) : (1.0f - t_anim);
		circle_t = held ? (t_anim) : (1.0f - t_anim);
	}

	ImU32 col_bg = GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);

	const ImRect frame_bb(pos, ImVec2(pos.x + width, pos.y + height));

	RenderFrame(frame_bb.Min, frame_bb.Max, col_bg, true, height * 0.5f);
	RenderNavHighlight(total_bb, id);

	ImVec2 label_pos = ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y);
	RenderText(label_pos, label);
	window->DrawList->AddCircleFilled(ImVec2(pos.x + radius + t * (width - radius * 2.0f), pos.y + radius), radius - (circle_t * radius) / 5.f, ImGui::GetColorU32(ImGuiCol_CheckMark), 16);

	return pressed;
}

bool ImExt::RadioButton(const char* label, bool active, const float dt)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const float square_sz = GetFrameHeight();
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, ImVec2(pos.x + square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), pos.y + label_size.y + style.FramePadding.y * 2.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
		MarkItemEdited(id);

	// Animation
	float last_active_id_timer = g.LastActiveIdTimer;
	if (held || pressed)
		g.LastActiveIdTimer = 0.f;

	if (g.LastActiveIdTimer == 0.f && g.LastActiveId == id && !pressed)
		g.LastActiveIdTimer = last_active_id_timer;

	float t = active ? 1.0f : 0.0f;
	float circle_t = held ? 1.0f : 0.0f;

	if (g.LastActiveId == id)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / 0.16f * dt);
		t = active ? (t_anim) : (1.0f - t_anim);
		circle_t = held ? (t_anim) : (1.0f - t_anim);
	}

	// Render
	const ImRect check_bb(pos, ImVec2(pos.x + square_sz, pos.y + square_sz));
	ImVec2 center = check_bb.GetCenter();
	center.x = IM_ROUND(center.x);
	center.y = IM_ROUND(center.y);
	const float radius = (square_sz - 1.0f) * 0.5f;

	RenderNavHighlight(total_bb, id);
	window->DrawList->AddCircleFilled(center, radius - (circle_t * radius) / 5.f, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);
	if (active)
	{
		const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
		window->DrawList->AddCircleFilled(center, (radius - pad) * t, GetColorU32(ImGuiCol_CheckMark), 16);
	}

	if (style.FrameBorderSize > 0.0f)
	{
		window->DrawList->AddCircle(ImVec2(center.x + 1, center.y + 1), radius, GetColorU32(ImGuiCol_BorderShadow), 16, style.FrameBorderSize);
		window->DrawList->AddCircle(center, radius, GetColorU32(ImGuiCol_Border), 16, style.FrameBorderSize);
	}

	ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
	if (g.LogEnabled)
		LogRenderedText(&label_pos, active ? "(x)" : "( )");
	if (label_size.x > 0.0f)
		RenderText(label_pos, label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}

bool ImExt::RadioButton(const char* label, int* v, int v_button, const float dt)
{
	const bool pressed = RadioButton(label, *v == v_button);
	if (pressed)
		*v = v_button;
	return pressed;
}

bool ImExt::Checkbox(const char* label, bool* v, const float dt)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const float square_sz = GetFrameHeight();
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, ImVec2(pos.x + square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), pos.y + label_size.y + style.FramePadding.y * 2.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
	{
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return false;
	}

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);

	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}

	// Animation
	float last_active_id_timer = g.LastActiveIdTimer;
	if (held || pressed)
		g.LastActiveIdTimer = 0.f;

	if (g.LastActiveIdTimer == 0.f && g.LastActiveId == id && !pressed)
		g.LastActiveIdTimer = last_active_id_timer;

	float t = held ? 1.0f : 0.0f;
	float mark_t = *v ? 1.0f : 0.0f;

	if (g.LastActiveId == id)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / 0.16f * dt);
		t = held ? (t_anim) : (1.0f - t_anim);
		mark_t = *v ? (t_anim) : (0.0f);
	}

	const float scale = 5.f * t;
	const ImRect check_bb(pos, ImVec2(pos.x + square_sz, pos.y + square_sz));
	const ImRect rect_bb(ImVec2(pos.x + scale / 2, pos.y + scale / 2), ImVec2(pos.x + square_sz - scale, pos.y + square_sz - scale));

	RenderNavHighlight(total_bb, id);
	RenderFrame(rect_bb.Min, rect_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
	ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
	bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
	if (mixed_value)
	{
		// Undocumented tristate/mixed/indeterminate checkbox (#2644)
		// This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
		ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
		const ImVec2 pos_min = ImVec2(rect_bb.Min.x + pad.x + scale, rect_bb.Min.y + pad.y + scale);
		const ImVec2 pos_max = ImVec2(rect_bb.Max.x - pad.x + scale, rect_bb.Max.y - pad.y + scale);
		window->DrawList->AddRectFilled(pos_min, pos_max, check_col, style.FrameRounding);
	}
	else if (*v || mark_t > 0.f)
	{
		const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
		const ImVec2 pos_min = ImVec2(check_bb.Min.x + pad, check_bb.Min.y + pad);

		float sz = square_sz - pad * 2.0f;
		float thickness = ImMax(sz * mark_t / 5.0f, 1.0f) * mark_t;
		sz -= thickness * 0.5f;
		const ImVec2 mark_pos = ImVec2(pos_min.x + thickness * 0.25f, pos_min.y + thickness * 0.25f);

		float third = sz / 3.0f;
		float bx = mark_pos.x + third;
		float by = mark_pos.y + sz - third * 0.5f;
		window->DrawList->PathLineTo(ImVec2(bx - third, by - third));
		window->DrawList->PathLineTo(ImVec2(bx, by));
		window->DrawList->PathLineTo(ImVec2(bx + third * 2.0f, by - third * 2.0f));
		window->DrawList->PathStroke(check_col, 0, thickness);
	}

	ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
	if (g.LogEnabled)
		LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");
	if (label_size.x > 0.0f)
		RenderText(label_pos, label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	return pressed;
}

bool ImExt::BeginCombo(const char* label, const char* preview_value, const ImVec2& size, const float dt, ImGuiComboFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
	ImGuiWindow* window = GetCurrentWindow();
	g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
	if (window->SkipItems)
		return false;

	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
	bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
	const ImVec2 preview_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 item_size = CalcItemSize(ImVec2(size.x, size.y + preview_size.y / 2), preview_size.x + style.FramePadding.x * 2.0f, preview_size.y + style.FramePadding.y * 2.0f);

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : item_size.y;
	const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : item_size.x;

	const ImRect bb(pos, ImVec2(pos.x + item_size.x, pos.y + item_size.y));
	ItemSize(item_size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	// Open on click
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);
	if (pressed)
	{
		OpenPopupEx(popup_id, ImGuiPopupFlags_None);
		popup_open = true;
	}

	//Animation
	float last_active_id_timer = g.LastActiveIdTimer;
	if (held || pressed)
		g.LastActiveIdTimer = 0.f;

	if (g.LastActiveIdTimer == 0.f && g.LastActiveId == id && !pressed)
		g.LastActiveIdTimer = last_active_id_timer;

	float t = held ? 1.0f : 0.0f;
	float popup_t = popup_open ? 1.0f : 0.0f;

	if (g.LastActiveId == id)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / 0.16f * dt);
		t = held && !pressed ? (t_anim) : (1.0f - t_anim);
		popup_t = popup_open ? (t_anim) : (1.0f - t_anim);
	}

	const float scale = item_size.x / 30.f * t;
	const ImRect render_bb = ImRect(ImVec2(pos.x + (scale / 2), pos.y + (scale / 2 * (item_size.y / item_size.x))), ImVec2(pos.x + (item_size.x - scale), pos.y + (item_size.y - (scale * (item_size.y / item_size.x)))));

	// Render
	const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	const float preview_zone = ImMax(render_bb.Min.x, render_bb.Max.x - arrow_size);
	RenderNavHighlight(bb, id);
	const ImVec2 pos_min = ImVec2(render_bb.Min.x, render_bb.Min.y);
	const ImVec2 pos_max = ImVec2(render_bb.Max.x, render_bb.Max.y);

	if (!(flags & ImGuiComboFlags_NoPreview)) // if preview need
		window->DrawList->AddRectFilled(pos_min, pos_max, frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);

	if (!(flags & ImGuiComboFlags_NoArrowButton)) // if arrow need
	{
		ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		ImU32 text_col = GetColorU32(ImGuiCol_Text);
		window->DrawList->AddRectFilled(ImVec2(preview_zone, render_bb.Min.y), render_bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
		if (preview_zone + arrow_size - style.FramePadding.x <= render_bb.Max.x)
		{
			ImVec2 arrow_min = ImVec2(preview_zone, pos_min.y);
			ImVec2 arrow_center = ImVec2(arrow_min.x + (pos_max.x - arrow_min.x) / 2, arrow_min.y + (pos_max.y - pos_min.y) / 2);

			ImVec2 a = ImVec2(arrow_center.x - (arrow_center.x - arrow_min.x) / 2, arrow_center.y - (arrow_center.y - arrow_min.y) / 3);
			ImVec2 b = ImVec2(arrow_center.x + (arrow_center.x - arrow_min.x) / 2, arrow_center.y - (arrow_center.y - arrow_min.y) / 3);
			ImVec2 c = ImVec2(arrow_center.x, arrow_center.y + (pos_max.y - arrow_center.y) / 3);
			window->DrawList->AddTriangleFilled(a, b, c, text_col);
			//RenderArrow(window->DrawList, ImVec2(preview_zone + style.FramePadding.x, render_bb.Min.y + style.FramePadding.y + (render_bb.Max.y - render_bb.Min.y) / render_bb.Min.y), text_col, ImGuiDir_Down, 1.0f);
		}
	}
	RenderFrameBorder(render_bb.Min, render_bb.Max, style.FrameRounding);

	// Custom preview
	if (flags & ImGuiComboFlags_CustomPreview)
	{
		g.ComboPreviewData.PreviewRect = ImRect(render_bb.Min.x, render_bb.Min.y, preview_zone, render_bb.Max.y);
		IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
		preview_value = NULL;
	}

	// Render preview and label
	if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
	{
		if (g.LogEnabled)
			LogSetNextTextDecoration("{", "}");
		RenderTextClipped(render_bb.Min, render_bb.Max, preview_value, NULL, NULL, style.ButtonTextAlign, &render_bb);
	}

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);

	g.NextWindowData.Flags = backup_next_window_data_flags;
	return BeginComboPopup(id, popup_id, render_bb, popup_t, flags);
}

bool ImExt::BeginComboPopup(ImGuiID parent_id, ImGuiID popup_id, const ImRect& bb, const float time, ImGuiComboFlags flags)
{
	ImGuiContext& g = *GImGui;
	float scrollbar_size = ImGui::GetStyle().ScrollbarSize;
	float popup_border_size = ImGui::GetStyle().PopupBorderSize;
	ImGui::GetStyle().PopupBorderSize = 0;
	ImGui::GetStyle().ScrollbarSize = ImClamp(time, 0.0001f, 1.f);

	bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
	if (!popup_open)
	{
		g.NextWindowData.ClearFlags();
		return false;
	}

	// Set popup size
	float w = bb.GetWidth();
	if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
	{
		g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
	}
	else
	{
		if ((flags & ImGuiComboFlags_HeightMask_) == 0)
			flags |= ImGuiComboFlags_HeightRegular;
		IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_)); // Only one
		int popup_max_height_in_items = -1;
		if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
		else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
		else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
		SetNextWindowSizeConstraints(ImVec2(w, 0.0f), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items) * time));
	}

	// This is essentially a specialized version of BeginPopupEx()
	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

	// Set position given a custom constraint (peak into expected window size so we can position it)
	// FIXME: This might be easier to express with an hypothetical SetNextWindowPosConstraints() function?
	// FIXME: This might be moved to Begin() or at least around the same spot where Tooltips and other Popups are calling FindBestWindowPosForPopupEx()?
	if (ImGuiWindow* popup_window = FindWindowByName(name))
		if (popup_window->WasActive)
		{
			// Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
			ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
			popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
			ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
			ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
			SetNextWindowPos(pos);
			SetNextWindowBgAlpha(time);
		}

	// We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(g.Style.FramePadding.x, g.Style.WindowPadding.y)); // Horizontally align ourselves with the framed text
	bool ret = Begin(name, NULL, window_flags);
	PopStyleVar();
	ImGui::GetStyle().PopupBorderSize = popup_border_size;
	ImGui::GetStyle().ScrollbarSize = scrollbar_size;
	if (!ret)
	{
		EndPopup();
		IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
		return false;
	}
	return true;
}