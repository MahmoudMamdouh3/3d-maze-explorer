cmake_minimum_required(VERSION 3.24)
project(3d-maze-explorer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- SFML 3.0.2 ---
include(FetchContent)
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
        GIT_SHALLOW ON
)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

# --- GLM ---
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(glm)

# --- GLAD ---
add_library(glad STATIC "vendor/glad/src/glad.c")
target_include_directories(glad PUBLIC "vendor/glad/include")

# --- Executable ---
# Include ALL the new source files here
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Core/Game.cpp
        src/Core/Game.h
        src/Core/ResourceManager.cpp
        src/Core/ResourceManager.h
        src/Graphics/Shader.cpp
        src/Graphics/Shader.h
        src/Graphics/Renderer.cpp
        src/Graphics/Renderer.h
        src/Entities/Player.cpp
        src/Entities/Player.h
        src/Entities/Map.cpp
        src/Entities/Map.h
)

# --- Include Paths ---
target_include_directories(${PROJECT_NAME} PRIVATE src)

# --- Linking ---
target_link_libraries(${PROJECT_NAME} PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        glad
        glm::glm
)

# --- Asset Copying ---
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)