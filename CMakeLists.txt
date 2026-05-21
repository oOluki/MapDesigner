# Set the minimum version of CMake
cmake_minimum_required(VERSION 3.10.0)
project(MapDesigner VERSION 0.0.1 LANGUAGES C CXX)


set(CMAKE_C_STANDARD 99)
set(CMAKE_C++_STANDARD 11)


# Add the main executable
add_executable(${PROJECT_NAME} src/map_designer_console.c)

if (WIN32)

    message(STATUS "Configuring for Windows...")

elseif(APPLE)
    message(STATUS "Configuring for macOS...")

    include_directories(/opt/homebrew/include/)

else()

    message(STATUS "Configuring for Linux...")

endif()

if (NOT WIN32)    
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()
