cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(PBR-Example)

# OPENGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
list(APPEND INCLUDES ${OPENGL_INCLUDE_DIR})
list(APPEND LIBS ${OPENGL_LIBRARIES})

# GLFW
add_subdirectory(vendor/glfw)
list(APPEND LIBS glfw)

# GLAD
add_subdirectory(vendor/glad)
list(APPEND LIBS glad)

# IMGUI
add_subdirectory(vendor/imgui)
list(APPEND LIBS imgui)

# STB_IMAGE
list(APPEND INCLUDES vendor/stb)

# GLM
list(APPEND INCLUDES vendor/glm)

# JSON
list(APPEND INCLUDES vendor/json)

# Add extra libraries based on the operating system.
if(WIN32)
    list(APPEND gdi32 user32)
else()
    list(APPEND LIBS X11 Xxf86vm Xrandr pthread Xi dl Xinerama Xcursor)
endif()

set(SOURCE_FILES    src/Scripts/AdvancedLighting.cpp src/Scripts/Model.cpp
                    src/Scripts/Model.h src/Scripts/Mesh.h
                    src/Scripts/Shader.h src/Scripts/Camera.h)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Set this project as startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC PROJECT_DIR="${PROJECT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})