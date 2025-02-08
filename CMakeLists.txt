cmake_minimum_required(VERSION 3.10)
project(capture_win)

set(CMAKE_CXX_STANDARD 17)

# Add httplib as a subdirectory
add_subdirectory(external/httplib)

# Add these lines to set the runtime library to static
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

# Add the executable
add_executable(capture_win
    capture_win/src/main.cpp
    # ...other source files...
)

# Include directories
target_include_directories(capture_win PRIVATE external/httplib)

# Link other necessary libraries
target_link_libraries(capture_win PRIVATE gdiplus httplib)
