#include "UI.h"
#include <vector>

#include "ImGui/imgui_extentions.h"

bool test_progress_button;
float test_progress_button_progress;
bool test_progress_toggle_button;
float test_progress_toggle_button_progress;
bool test_toggle_button;
bool test_toggle_switch;
bool test_checkbox;
int test_radiobutton;
float test_drag_float_single;
const char* test_combo_preview = NULL;
std::vector<const char*> test_combo_vector = {
	"Element A",
	"Element B",
	"Element C",
};

void UI::DrawUI(const ImVec2& size)
{
	ImGui::Begin("Test UI", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	{
		ImGui::Text("Buttons");
		ImExt::Button("Button", size);
		ImExt::ProgressButton("Progress Button", &test_progress_button, &test_progress_button_progress, size);
		ImGui::Text("Progress Button State: %s", test_progress_button ? "true" : "false");
		ImGui::Text("Toggle Buttons");
		ImExt::ToggleButton("Toggle Button", &test_toggle_button, size);
		ImExt::ProgressToggleButton("Progress Toggle Button", &test_progress_toggle_button, &test_progress_toggle_button_progress, size);
		ImGui::Text("Progress Toggle Button State: %s", test_progress_toggle_button ? "true" : "false");
		ImExt::ToggleSwitch("Toggle Switch", &test_toggle_switch);
		ImExt::Checkbox("Checkbox", &test_checkbox);
		ImGui::Text("Radio Buttons");
		ImExt::RadioButton("First Radio", &test_radiobutton, 0);
		ImExt::RadioButton("Second Radio", &test_radiobutton, 1);
		ImExt::RadioButton("Third Radio", &test_radiobutton, 2);

		ImGui::Text("Combo");
		if (test_combo_preview == NULL)
			test_combo_preview = "Select Element...";
		if (ImExt::BeginCombo("##combo", test_combo_preview, size))
		{
			for (auto i : test_combo_vector)
			{
				bool selected = (test_combo_preview == i);
				if (ImGui::Selectable(i, selected))
					test_combo_preview = i;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}
	ImGui::End();
}