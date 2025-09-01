cmake_minimum_required(VERSION 3.25)
project(ModernInventory VERSION 0.1.0 LANGUAGES CXX)

# Toolchain & language
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(CommonLibSSE CONFIG REQUIRED)    # via vcpkg or registry
# (Optional) spdlog via vcpkg or FetchContent; otherwise roll your own simple log

# ImGui as an object or static lib
add_library(imgui STATIC
  third_party/imgui/imgui.cpp
  third_party/imgui/imgui_draw.cpp
  third_party/imgui/imgui_tables.cpp
  third_party/imgui/imgui_widgets.cpp
  third_party/imgui/backends/imgui_impl_dx11.cpp
  third_party/imgui/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui PUBLIC
  third_party/imgui
  third_party/imgui/backends
)
# No exceptions/RTTI required; keep defaults

# Main plugin
add_commonlibsse_plugin(${PROJECT_NAME}
  SOURCES
    src/plugin/Plugin.cpp
    src/hooks/Events.cpp
    src/renderer/D3D11Hook.cpp
    src/renderer/ImGuiLayer.cpp
    src/ui/RightPane.cpp
    src/game/Preview3D.cpp
    src/config/Settings.cpp
    src/util/Log.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui CommonLibSSE::CommonLibSSE)

# Precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

# Post-build copy (DLL + default INI) — EDIT the mods path or use env var
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
  set(MODS_OUT "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
  message(STATUS "Deploying to: ${MODS_OUT}")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODS_OUT}/SKSE/Plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:${PROJECT_NAME}>" "${MODS_OUT}/SKSE/Plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/Data/SKSE/Plugins/${PROJECT_NAME}.ini" "${MODS_OUT}/SKSE/Plugins/${PROJECT_NAME}.ini"
  )
endif()
