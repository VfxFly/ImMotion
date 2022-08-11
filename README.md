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

 1. Add "imgui_extentions.h" & "imgui_extentions.cpp" in project
 2. Include header in code file
```
#include "imgui_extentions.h"
```
 3. Place controls in window
```
ImExt::Button("Button", {120.f, 27.f});
```
Result:
<br>![button_example](https://github.com/VfxFly/ImMotion/blob/690cd0c802e56d2b48152d6e150a31205e93c926/Resources/Button.gif)

### How to control animation speed?
**Use control functions argument "dt"**

All controls preview:
<br>![controls_example](https://github.com/VfxFly/ImMotion/blob/76f4480b84a368058dd831015a7bbd43e7e95047/Resources/ImMotion.gif)
