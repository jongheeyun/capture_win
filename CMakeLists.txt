cmake_minimum_required(VERSION 3.10)

project(capture_win)

set(CMAKE_CXX_STANDARD 17)


# Add these lines to set the runtime library to static
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")


# Add the source files
add_executable(capture_win WIN32 src/main.cpp)

# Link against the required libraries
target_link_libraries(capture_win PRIVATE gdiplus User32 Gdi32)