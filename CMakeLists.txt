cmake_minimum_required(VERSION 3.25)
project(ModernInventory VERSION 0.1.0 LANGUAGES CXX)

# Toolchain / language
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies (via vcpkg or VS-integrated vcpkg)
find_package(CommonLibSSE CONFIG REQUIRED)

# Main SKSE plugin target
add_commonlibsse_plugin(${PROJECT_NAME}
  SOURCES
    src/main.cpp
)

# Precompiled header (first include in every .cpp)
target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h)

# Include paths (root for PCH.h, and public headers)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Match CommonLibSSE build settings (avoid Debug iterator/CRT mismatches)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  _ITERATOR_DEBUG_LEVEL=0
  _SECURE_SCL=0
)

# Optional: copy the DLL to your MO2 mods folder automatically
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
  set(MODS_OUT "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
  message(STATUS "Deploying to: ${MODS_OUT}")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODS_OUT}/SKSE/Plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:${PROJECT_NAME}>" "${MODS_OUT}/SKSE/Plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
  )
endif()
