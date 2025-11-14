# Set the minimum version of CMake
cmake_minimum_required(VERSION 3.10.0)
project(MapDesigner VERSION 0.0.1 LANGUAGES C CXX)


set(CMAKE_C_STANDARD 99)
set(CMAKE_C++_STANDARD 11)


# Add the main executable
add_executable(${PROJECT_NAME}Console src/map_designer_console.c)

if (WIN32)
    message(STATUS "Configuring for Windows...")

    # Use vcpkg for SDL2 on Windows
    find_package(SDL2 CONFIG QUIET)
    if(SDL2_FOUND)
        message(STATUS "SDL2 found, " ${PROJECT_NAME} " will be build")
        add_executable(${PROJECT_NAME} src/map_designer_sdl.c)
        include_directories(${SDL2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
    else()
        message(WARNING "SDL2 not found, " ${PROJECT_NAME} " will not be built")
    endif()

elseif(APPLE)
    message(STATUS "Configuring for macOS...")

    include_directories(/opt/homebrew/include/)

    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        message(STATUS "SDL2 found, " ${PROJECT_NAME} " will be build")
        add_executable(${PROJECT_NAME} src/map_designer_sdl.c)
        include_directories(${SDL2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE "/opt/homebrew/lib/libSDL2.dylib" m)
    else()
        message(WARNING "SDL2 not found, " ${PROJECT_NAME} " will not be built")
    endif()

else()
    message(STATUS "Configuring for Linux...")

    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        message(STATUS "SDL2 found, " ${PROJECT_NAME} " will be build")
        add_executable(${PROJECT_NAME} src/map_designer_sdl.c)
        include_directories(${SDL2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2 m)
    else()
        message(WARNING "SDL2 not found, " ${PROJECT_NAME} " will not be built")
    endif()

endif()

if (NOT WIN32)    
    target_link_libraries(${PROJECT_NAME}Console PRIVATE m)
endif()
