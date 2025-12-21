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
# Speed up the build by disabling SFML stuff you don't need
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

# --- GLM (Math Library) ---
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(glm)

# --- GLAD ---
# Updated paths to match your vendor/glad/src/glad.c structure
add_library(glad STATIC "vendor/glad/src/glad.c")
target_include_directories(glad PUBLIC "vendor/glad/include")

# --- Executable ---
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/Camera.cpp
        src/Renderer.cpp
)

# --- Include Paths ---
# This tells the compiler where to find your .hpp files
target_include_directories(${PROJECT_NAME} PRIVATE include)

# --- Linking ---
target_link_libraries(${PROJECT_NAME} PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        glad
        glm::glm
)

# --- Asset Copying ---
# This ensures your shaders and textures are in the same folder as the .exe
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)