# Dota 2 External Patcher — VS Code / CMake build
# Requires: Visual Studio Build Tools / MSVC, CMake 3.20+
# Recommended extension: CMake Tools

cmake_minimum_required(VERSION 3.20)
project(external_patcher VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use UNICODE (matches original project settings)
add_definitions(-DUNICODE -D_UNICODE)

# Static runtime for Release (matches original vcxproj)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(external_patcher
    core/main.cpp
    core/memory.cpp
)

target_include_directories(external_patcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

# Link required Windows libraries
target_link_libraries(external_patcher PRIVATE
    kernel32
    user32
    psapi
)

# MSVC-specific Release flags (match original vcxproj)
if(MSVC)
    target_compile_options(external_patcher PRIVATE
        $<$<CONFIG:Release>:/O2 /Oi /Ot /Oy /GL>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
    target_link_options(external_patcher PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
endif()

# Keep the executable in the project root with the same name as the batch build
set_target_properties(external_patcher PROPERTIES
    OUTPUT_NAME "external_patcher"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
