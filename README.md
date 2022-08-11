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
 - Check Box

### Usage

**The controls use dependencies from ImGui, while not replacing the code of standard widgets/not overloading functions.
Extensions use their own namespace - ImExt.**

### You can use this as follows: 

 1. Add "imgui_extentions.h" & "imgui_extentions.cpp" in project
 2. Include header in code file
```
#include "imgui_extentions.h"
```
 3. Place controls in window
```
ImExt::Button("Button", {120.f, 27.f});
```
