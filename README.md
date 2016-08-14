[![Build status](https://ci.appveyor.com/api/projects/status/m55jepbp58txins3?svg=true)](https://ci.appveyor.com/project/Wohlstand/miniphysics-by-38a)

# MiniPhysics-By-38A
A tiny physical engine made by 5438A38A in VB6 and ported into C++ with Qt and improved by Wohlstand

This engine demo implements a classical platform game physics. Such a:
* Rectangular blocks of any size
* Truangular slope blocks of any size (floor and ceiling). Auto-align character's Y position at top corner of slope while walking up on it.
* Speed-adding stack (adding speed to objects staying on moving other)
* Collision filters to block character movement at one of sides and pass character at others
* Overlapping detection (character will be painted red) when player is touches inside boundaries area of the solid blocks
* Crush detection when two moving solid boxes will crush at top and bottom, or at left and right
* Cliff detection when character is about edge, on both rectanglar and trianglar blocks
* Touched blocks detection to process special actions from touching of the special blocks (for example, hurt player or NPC on touching a danger surfaces)

# Download
[Working demo for Win32 can be got here](http://wohlsoft.ru/docs/_laboratory/_Builds/win32/mini-physics/mini-physics-demo-win32.zip)

# Controlling character
* Arrow keys (Move left-right)
* Space (jump)
* F1...F4 (toggle loop delay: F1 = 25 milliseconds between each frame, F2 = 100, F3 = 250, F4 = 500)
* 1, 2, 3 (toggle character's size: 1 = 24x30 (small playable character / ducked state), 2  = 32x32 (most regular NPC/item), 3 = 24x50 (regular playable character size))
* Q (disable/enable ability to walk over floor holes. In SMBX playable characters are can that. NPC-s can't)
* W (disable/enable automatic aligning Y on slope top corner while walking up on slope. Disabled in the sliding mode).
* F11 toggle demo level 1 (toggling while already switched just returns character to initial position)
* F12 toggle demo level 2 (toggling while already switched just returns character to initial position)

Notes: Character automatically will be teleported to up side of screen when it will fall down into the pit

# Building from sources
* Clonning requires a downloading of dependent submodules
```bash
git clone https://github.com/Wohlhabend-Networks/MiniPhysics-By-38A.git
cd MiniPhysics-By-38A
git submodule update --init --recursive
```
* This demo can be built on almost any platform supported by Qt (Linux, Windows, Mac OS X).
* For correct working OpenGL support is required.
* You are required to have [latest Qt 5 package which you can take here](https://www.qt.io/download-open-source/)
Just open the MiniPhysics.pro in the Qt Creator and process the build.

