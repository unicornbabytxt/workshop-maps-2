cmake_minimum_required(VERSION 3.20)
project(WorkshopMaps CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

# Path to a checkout of https://github.com/bakkesmodorg/BakkesModSDK
set(BAKKESMOD_SDK "${CMAKE_SOURCE_DIR}/BakkesModSDK" CACHE PATH "BakkesMod SDK folder")

add_library(WorkshopMaps SHARED
    src/WorkshopMaps.cpp
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_stdlib.cpp
)

target_include_directories(WorkshopMaps PRIVATE src "${BAKKESMOD_SDK}/include")
target_link_libraries(WorkshopMaps PRIVATE "${BAKKESMOD_SDK}/lib/pluginsdk.lib" shell32 ole32 oleaut32 ws2_32 iphlpapi)
target_compile_definitions(WorkshopMaps PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
target_compile_options(WorkshopMaps PRIVATE /utf-8 /MP)
set_target_properties(WorkshopMaps PROPERTIES OUTPUT_NAME "WorkshopMaps2")
set(WM_BUILD "0" CACHE STRING "Build number, set by CI")
target_compile_definitions(WorkshopMaps PRIVATE WM_BUILD=${WM_BUILD})
