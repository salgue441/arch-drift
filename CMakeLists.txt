cmake_minimum_required(VERSION 3.28)

project(archdrift
    VERSION 0.1.0
    DESCRIPTION "Archdrift — Linux arcade kart racer (C++23 / Vulkan)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(KART_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

include(FetchContent)
# Allow Populate() for Vulkan-Headers so we only use include/ (no C++ modules target).
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

# --- GLFW -----------------------------------------------------------------
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

# --- Vulkan headers (system or FetchContent) ------------------------------
find_package(Vulkan QUIET)
if(NOT Vulkan_FOUND)
    FetchContent_Declare(
        VulkanHeaders
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG v1.3.296
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(VulkanHeaders)
    if(NOT vulkanheaders_POPULATED)
        FetchContent_Populate(VulkanHeaders)
    endif()

    find_library(Vulkan_LIBRARY
        NAMES vulkan vulkan-1 libvulkan.so.1
        PATHS /usr/lib64 /usr/lib /lib64 /lib
        REQUIRED
    )
    set(Vulkan_INCLUDE_DIRS "${vulkanheaders_SOURCE_DIR}/include")
    add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${Vulkan_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}"
    )
endif()

# --- glslangValidator (local tools/ or download) --------------------------
set(KART_GLSLANG_VALIDATOR "")
if(EXISTS "${CMAKE_SOURCE_DIR}/tools/glslang/bin/glslangValidator")
    set(KART_GLSLANG_VALIDATOR "${CMAKE_SOURCE_DIR}/tools/glslang/bin/glslangValidator")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/tools/glslang/bin/glslang")
    set(KART_GLSLANG_VALIDATOR "${CMAKE_SOURCE_DIR}/tools/glslang/bin/glslang")
else()
    find_program(KART_GLSLANG_VALIDATOR NAMES glslangValidator glslang)
endif()

if(NOT KART_GLSLANG_VALIDATOR)
    set(_kart_glslang_archive "${CMAKE_BINARY_DIR}/_deps/glslang-16.6.0-linux-x86_64-release.tar.gz")
    set(_kart_glslang_root "${CMAKE_BINARY_DIR}/_deps/glslang-bin")
    if(NOT EXISTS "${_kart_glslang_root}/bin/glslangValidator")
        message(STATUS "Downloading glslangValidator for shader compilation...")
        file(DOWNLOAD
            "https://github.com/KhronosGroup/glslang/releases/download/16.6.0/glslang-16.6.0-linux-x86_64-release.tar.gz"
            "${_kart_glslang_archive}"
            SHOW_PROGRESS
            STATUS _dl_status
        )
        list(GET _dl_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            message(FATAL_ERROR "Failed to download glslangValidator")
        endif()
        file(MAKE_DIRECTORY "${_kart_glslang_root}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${_kart_glslang_archive}"
            WORKING_DIRECTORY "${_kart_glslang_root}"
            RESULT_VARIABLE _tar_result
        )
        if(NOT _tar_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract glslangValidator")
        endif()
    endif()
    set(KART_GLSLANG_VALIDATOR "${_kart_glslang_root}/bin/glslangValidator")
endif()

message(STATUS "Using glslangValidator: ${KART_GLSLANG_VALIDATOR}")

# --- Shaders --------------------------------------------------------------
set(KART_SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(KART_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${KART_SHADER_OUTPUT_DIR}")

set(KART_SHADER_SOURCES
    "${KART_SHADER_SOURCE_DIR}/triangle.vert"
    "${KART_SHADER_SOURCE_DIR}/triangle.frag"
)

set(KART_SHADER_SPV
    "${KART_SHADER_OUTPUT_DIR}/triangle.vert.spv"
    "${KART_SHADER_OUTPUT_DIR}/triangle.frag.spv"
)

add_custom_command(
    OUTPUT "${KART_SHADER_OUTPUT_DIR}/triangle.vert.spv"
    COMMAND "${KART_GLSLANG_VALIDATOR}" -V "${KART_SHADER_SOURCE_DIR}/triangle.vert"
            -o "${KART_SHADER_OUTPUT_DIR}/triangle.vert.spv"
    DEPENDS "${KART_SHADER_SOURCE_DIR}/triangle.vert"
    COMMENT "Compiling triangle.vert"
    VERBATIM
)

add_custom_command(
    OUTPUT "${KART_SHADER_OUTPUT_DIR}/triangle.frag.spv"
    COMMAND "${KART_GLSLANG_VALIDATOR}" -V "${KART_SHADER_SOURCE_DIR}/triangle.frag"
            -o "${KART_SHADER_OUTPUT_DIR}/triangle.frag.spv"
    DEPENDS "${KART_SHADER_SOURCE_DIR}/triangle.frag"
    COMMENT "Compiling triangle.frag"
    VERBATIM
)

add_custom_target(shaders ALL DEPENDS ${KART_SHADER_SPV})

# --- Executable -----------------------------------------------------------
add_executable(archdrift
    src/main.cpp
    src/platform/Window.cpp
    src/render/Renderer.cpp
)

add_dependencies(archdrift shaders)

target_include_directories(archdrift PRIVATE "${CMAKE_SOURCE_DIR}/src")

target_compile_definitions(archdrift PRIVATE
    KART_SHADER_DIR="${KART_SHADER_OUTPUT_DIR}"
)

if(TARGET glfw)
    target_link_libraries(archdrift PRIVATE glfw Vulkan::Vulkan)
else()
    target_link_libraries(archdrift PRIVATE glfw Vulkan::Vulkan)
endif()

# Some Findglfw3 exports glfw, others glfw3::glfw
if(TARGET glfw3::glfw)
    target_link_libraries(archdrift PRIVATE glfw3::glfw)
endif()

target_compile_options(archdrift PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

if(KART_WARNINGS_AS_ERRORS)
    target_compile_options(archdrift PRIVATE -Werror)
endif()
