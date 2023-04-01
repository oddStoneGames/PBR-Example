# PBR Example <!-- omit in toc -->

## Contents <!-- omit in toc -->

- [Introduction](#introduction)
  - [Features](#features)
- [Setup](#setup)
- [Build](#build)
  - [Supported Platforms](#supported-platforms)
  - [Dependencies](#dependencies)
  - [Build with CMake](#build-with-cmake)
- [License](#license)

## Introduction 

[![VideoThumbnail](http://img.youtube.com/vi/JNWdO1HaOb4/0.jpg)](https://www.youtube.com/watch?v=JNWdO1HaOb4)

This project is a showcase of some advanced opengl rendering techniques.  

### Features
- PBR Workflow (Physically Based Rendering)
- Deferred & Forward Lighting
- SSAO (Screen Space Ambient Occlusion)
- FXAA (Fast Approximate Anti Aliasing)
- Bloom
- Tone Mapping
- HDR Rendering
- Parallax Occlusion Mapping
- Shadow Mapping
- .glTF Model Loader

## Setup 
 
Prerequisites: [git](https://git-scm.com/downloads) with [git large file storage (git-lfs)](https://docs.github.com/en/repositories/working-with-files/managing-large-files/installing-git-large-file-storage).

Clone the repo using the following command:

```
git clone https://github.com/oddStoneGames/PBR-Example.git
cd PBR-Example
```

## Build 
 
### Supported Platforms  
- Windows
- Linux

### Dependencies 
 
- Hardware with support for OpenGL 4.2 Core
- CMake v3.10+
- C++14 Compiler 
- On Linux, these [libraries](https://www.glfw.org/docs/latest/compile_guide.html) are required to build GLFW

### Build with CMake 
 
`Step 1.` Make a directory for the build files.

```
mkdir build
```

`Step 2.` Generate the project files.

```
cmake -S . -B ./build
```

`Step 3.` Build the project.

```
cmake --build build --config Release --target PBR-Example
```
`Step 4.` Run the executable PBR-Example which is located in the build or build/Release folder.

## License 
 
See [LICENSE](LICENSE).

This project has some third-party dependencies, each of which may have independent licensing:

- [glad](https://glad.dav1d.de/): Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs.
- [glfw](https://github.com/glfw/glfw): A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input
- [glm](https://github.com/g-truc/glm): OpenGL Mathematics
- [dear imgui](https://github.com/ocornut/imgui): Immediate Mode Graphical User Interface
- [stb](https://github.com/nothings/stb): Single-file public domain (or MIT licensed) libraries

HDR Maps:
- [Cannon](https://polyhaven.com/a/cannon)
- [Kiara 1 Dawn](https://polyhaven.com/a/kiara_1_dawn)
- [Kloppenheim 02](https://polyhaven.com/a/kloppenheim_02)
- [Photo Studio Loft Hall](https://polyhaven.com/a/photo_studio_loft_hall)
- [Stuttgart Suburbs](https://polyhaven.com/a/stuttgart_suburbs)
- [Venice Sunset](https://polyhaven.com/a/venice_sunset)

Models:
- [Aladdin's Lamp](https://sketchfab.com/3d-models/alladins-lamp-4aee21ad3f804c1abe1d846eb31d6269)
- [Bed](https://sketchfab.com/3d-models/king-floor-bed-6ee7831cc521471384191baea365e211)
- [Wall Lamp](https://sketchfab.com/3d-models/metal-wall-lamp-384b79f0be904b9fb97905d65dc8ccfd)
- [External Lamp](https://sketchfab.com/3d-models/external-lamp-4e9a1cc5b36148a89ad407f4c1cb904d)
- [Turtle](https://sketchfab.com/3d-models/model-64a-eastern-painted-turtle-154214dec4c14e959e975aa8d17b1578)
- [Indoor Plant](https://sketchfab.com/3d-models/indoor-plant-bpaOec5a2WUaiKuF0ATci1cjCK7)
- [Indoor Plant 2](https://sketchfab.com/3d-models/indoor-plant-02-fbx-f9593646aa7f4ded8d56b5d0aef1c28b)
- [Palm Plant](https://sketchfab.com/3d-models/palm-plant-fc5680053bb54ff6bacae64be12200ad)