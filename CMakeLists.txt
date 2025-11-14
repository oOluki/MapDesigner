# Set the minimum version of CMake
cmake_minimum_required(VERSION 3.10.0)
project(MapDesigner VERSION 0.0.1 LANGUAGES C CXX)


set(CMAKE_C_STANDARD 99)


# Add the main executable
add_executable(${PROJECT_NAME}Console src/map_designer_console.c)
add_executable(${PROJECT_NAME} src/map_designer_sdl.c)


if (NOT WIN32)    
    target_link_libraries(${PROJECT_NAME}Console PRIVATE m)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
endif()
