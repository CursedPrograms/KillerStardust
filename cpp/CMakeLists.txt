cmake_minimum_required(VERSION 3.20)
project(KillerStardust VERSION 1.0.0 LANGUAGES C CXX)
# Killer Stardust: an Arcade Phantasm game. SDL2 (window, input, sound), OpenGL 3, Dear ImGui (drawing),
# stb_image (the logos) and stb_vorbis (the theme). Only SDL2 has to be installed; the rest is fetched here.
#
#   Windows: build.bat (fetches SDL2's VC package into external/)    Linux: ./build.sh (needs libsdl2-dev)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(imgui URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.4.tar.gz)
FetchContent_MakeAvailable(imgui)

# stb: two single files
set(STB_DIR ${CMAKE_BINARY_DIR}/stb)
foreach(f stb_image.h stb_vorbis.c)
    if(NOT EXISTS ${STB_DIR}/${f})
        file(DOWNLOAD https://raw.githubusercontent.com/nothings/stb/master/${f} ${STB_DIR}/${f} STATUS st)
    endif()
endforeach()

if(WIN32 AND NOT SDL2_DIR AND EXISTS ${CMAKE_SOURCE_DIR}/external/SDL2-2.30.9/cmake)
    set(SDL2_DIR ${CMAKE_SOURCE_DIR}/external/SDL2-2.30.9/cmake)
endif()
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL)

add_executable(KillerStardust WIN32 src/main.cpp src/game.cpp src/audio.cpp src/app.rc)
target_include_directories(KillerStardust PRIVATE ${STB_DIR} src)
target_link_libraries(KillerStardust PRIVATE imgui)
if(TARGET SDL2::SDL2main)
    target_link_libraries(KillerStardust PRIVATE SDL2::SDL2main)
endif()
if(MSVC)
    target_compile_options(KillerStardust PRIVATE /W3 /utf-8)
endif()

# the game finds its assets next to the exe
add_custom_command(TARGET KillerStardust POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:KillerStardust>/assets)
if(WIN32)
    add_custom_command(TARGET KillerStardust POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:KillerStardust>)
endif()
