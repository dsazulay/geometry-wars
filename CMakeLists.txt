cmake_minimum_required(VERSION 3.23)

project(geometry-wars VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

if (NOT IS_DIRECTORY $ENV{VULKAN_SDK})
    message(WARNING "Vulkan SDK not detected, if installed please set VULKAN_SDK env var or manually provide libraries")
else()
    message(STATUS "Found Vulkan SDK in $ENV{VULKAN_SDK}")
endif()

# SDL3, Volk, VMA, glm and Slang are taken from the SDK
include_directories(SYSTEM $ENV{VULKAN_SDK}/include)

find_library(SDL_LIBRARY NAMES SDL3 HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

add_executable(geometry-wars src/main.cpp)

target_link_libraries(geometry-wars PRIVATE ${SDL_LIBRARY})
if (TARGET Vulkan::Vulkan)
    target_link_libraries(geometry-wars PRIVATE Vulkan::Vulkan)
elseif (APPLE)
    target_link_libraries(geometry-wars PRIVATE $ENV{VULKAN_SDK}/lib/libvulkan.dylib)
elseif (NOT WIN32)
    target_link_libraries(geometry-wars PRIVATE $ENV{VULKAN_SDK}/lib/libvulkan.so)
endif()

target_compile_options(geometry-wars PRIVATE -Wall -Wextra -Wshadow -pedantic -g)
