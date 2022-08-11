ImMotions - make motions for your Dear ImGui!
=====

### Dear ImGui extentions project.

### Requirements:
 - Dear-ImGui 1.88+

### Usage

**The controls use dependencies from ImGui, while not replacing the code of standard widgets/not overloading functions.
Extensions use their own namespace - ImExt.**

### You can use this as follows: 

 1. Add "imgui_extentions.h" & "imgui_extentions.cpp" in project
 2. Include header with directive:
```
#include "imgui_extentions.h"
```
 3. Place controls in window loop
```
ImExt::Button("Button", {120.f, 27.f});
```
