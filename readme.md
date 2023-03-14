
Miscellaneous code written when I am **free** XD

This project can be compiled using cmake, both windows(mingw) and linux is OK,
requires library opengl and glfw.

## explanation for some directories

- brain: simple ANN with back-propagation and LSTM(rather simple)
- data_structure: some generic data structure often used
- math: geometry operations and linear math 
- pathfinding(**not updated, may not work**): path finding library and some graph's data structure
- physics: simple 2d physics simulation, using SAT and V_CLIP(proposed by Erin Catto), and Sequential Impulse for contact resolution
- vg: vector graphics library, imitating Nanovg

#### OpenGL Programming Guide
http://www.glprogramming.com/red/chapter14.html#name5

#### compile using emscripten
add `-DCMAKE_TOOLCHAIN_FILE=C:/Environment/msys64/home/amias/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
--debug-output` to cmake options
