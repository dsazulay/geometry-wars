cmake_minimum_required(VERSION 3.23)
include(FetchContent)

project(geometry-wars VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)

if (NOT IS_DIRECTORY $ENV{VULKAN_SDK})
    message(WARNING "Vulkan SDK not detected, if installed please set VULKAN_SDK env var or manually provide libraries")
else()
    message(STATUS "Found Vulkan SDK in $ENV{VULKAN_SDK}")
endif()

# SDL3, Volk, VMA, glm and Slang are taken from the SDK
include_directories(SYSTEM $ENV{VULKAN_SDK}/include)
include_directories(SYSTEM vendor/ktx/include)
include_directories(SYSTEM vendor/ktx/other_include)

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(glm)

# ktx
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/vkloader.c)
add_library(ktx STATIC ${KTX_SOURCES})

find_library(Slang_LIBRARY NAMES slang slang-compiler HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

find_library(SDL_LIBRARY NAMES SDL3 HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

add_executable(geometry-wars
    src/main.cpp
    src/vulkan_engine.cpp
    src/game.cpp
    src/base/logger.cpp
    src/asset/resource_manager.cpp
)

target_link_libraries(geometry-wars PRIVATE ${SDL_LIBRARY} glm ktx ${Slang_LIBRARY})
if (TARGET Vulkan::Vulkan)
    target_link_libraries(geometry-wars PRIVATE Vulkan::Vulkan)
elseif (APPLE)
    target_link_libraries(geometry-wars PRIVATE $ENV{VULKAN_SDK}/lib/libvulkan.dylib)
elseif (NOT WIN32)
    target_link_libraries(geometry-wars PRIVATE $ENV{VULKAN_SDK}/lib/libvulkan.so)
endif()

target_compile_options(geometry-wars PRIVATE -Wall -Wextra -Wshadow -pedantic -g)
target_compile_definitions(geometry-wars PRIVATE $<$<CONFIG:Debug>:DEBUG>)   
