ImMotions - make motions for your Dear ImGui!
=====

### Dear ImGui extentions project.

### Based on: https://github.com/bludeck/imgui-animated?ysclid=l6p36iuqiq122384304

### Requirements:
 - Dear-ImGui 1.88+

### Available controls:
 - Button
 - Progress Button
 - Toggle Button
 - Progress Toggle Button
 - Radio Button
 - bludeck's Toggle Switch
 - Check Box

### Usage

**The controls use dependencies from ImGui, while not replacing the code of standard widgets/not overloading functions.
Extensions use their own namespace - ImExt.**

### You can use this as follows: 

 1. Download [latest release](https://github.com/VfxFly/ImMotion/releases/tag/ImMotion) from this repository
 2. Add "imgui_extentions.h" & "imgui_extentions.cpp" in project
 3. Include header in code file
```
#include "imgui_extentions.h"
```
 4. Place controls in window
```
ImExt::Button("Button", {120.f, 27.f});
```
Result:
<br>![button_example](https://github.com/VfxFly/ImMotion/blob/690cd0c802e56d2b48152d6e150a31205e93c926/Resources/Button.gif)

```
std::vector<const char*> elements;
if (ImExt::BeginCombo("##combo", combo_preview, size))
{
	for (auto i : elements)
	{
		bool selected = (combo_preview == i);
		if (ImGui::Selectable(i, selected))
			test_combo_preview = i;
		if (selected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
}
```
Result:
<br>![combo_example](https://github.com/VfxFly/ImMotion/blob/690cd0c802e56d2b48152d6e150a31205e93c926/Resources/BeginCombo.gif)

### How to control animation speed?
**Use control functions argument "dt"**
```
ImExt::Button("Button", {120.f, 27.f}, 0.5f);
```

### All controls preview
Taken in an [example-project](https://github.com/VfxFly/ImMotion/tree/main/Example/ImMotion)
<br>![controls_example](https://github.com/VfxFly/ImMotion/blob/76f4480b84a368058dd831015a7bbd43e7e95047/Resources/ImMotion.gif)
