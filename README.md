# bytesized (Game Engine)
Minimal game engine written in C++/OpenGL.

## Requirements
The engine uses glew and SDL2 for GL, window and input bindings.
* glew installed on your system
* SDL2 installed on your system

## How to use
Fastest way to get started is to add this repository as a git submodule, then compile and link using cmake.

```cmake
# Example app cmake file
cmake_minimum_required(VERSION 3.15)
project(my_app)

add_subdirectory(bytesized)

add_executable(my_app main.cpp app.cpp)
target_link_libraries(my_app PUBLIC bytesized)
```

## Run tests
How to run the unit tests.

`cmake -B build && cmake --build build && ctest --test-dir build --rerun-failed --output-on-failure`

## Features
This section aims to keep track of features both completed and under development.

|Section         |Feature         |Status          |Updated         |
|----------------|----------------|----------------|----------------|
|3D model loading|Reading .glb    |Completed       |2024-11-13      |
|3D model loading|Embedding .glb  |Completed       |2024-11-13      |
|2D image loading|Embedding image |Completed       |2024-11-13      |
|2D sprites      |W/ texture atlas|Completed       |2025-04-19      |
|Parsing GLTF    |Nodes/meshes    |Completed       |2024-11-13      |
|Parsing GLTF    |Textures/images |Completed       |2024-11-13      |
|Parsing GLTF    |Skins           |Completed       |2024-11-13      |
|Parsing GLTF    |Animations      |Completed       |2024-11-13      |
|Rendering       |Node hierarchies|Completed       |2024-11-13      |
|Rendering       |Skeletal anims. |Completed       |2024-11-13      |
|Rendering       |Text/bdf font   |Completed       |2024-12-19      |
|Rendering       |Normal mapping  |Backlog         |-               |
|Rendering       |Parallax mapping|Backlog         |-               |
|Collision       |Skeletal IK     |Completed       |2025-04-19      |
|Collision       |AABB            |Completed       |2025-04-19      |
|Collision       |Sphere          |Completed       |2025-04-19      |
|Collision       |OOBB            |Completed       |2025-04-19      |
|Collision       |Convex poly(GJK)|Completed       |2025-04-19      |
|Collision       |Ragdoll         |Backlog         |-               |
|Physics         |Collision resp. |Completed       |2025-04-19      |
|System          |ECS             |Completed       |2024-11-16      |
|Editor          |Camera control  |Completed       |2024-12-19      |
|Editor          |Object transform|Completed       |2024-12-19      |
|UI              |Splitview(panel)|Backlog         |-               |
|Console         |-               |Completed       |2024-12-19      |
