# Cross-platform CMake build configuration
# Supports Windows (MSVC/MinGW), Linux, and macOS

cmake_minimum_required(VERSION 3.15)
project(ServerControl VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Platform-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    set(PLATFORM_LIBS ws2_32)
    if(MSVC)
        add_compile_options(/W3 /O2 /MD)
    endif()
else()
    set(PLATFORM_LIBS pthread)
    add_compile_options(-Wall -O2)
endif()

# Server executable
add_executable(servercontrol
    server.cpp
)
target_link_libraries(servercontrol ${PLATFORM_LIBS})

# Control executable
add_executable(control
    control/main.cpp
    control/ui/UI.cpp
    control/core/TaskManager.cpp
    control/config/Config.cpp
    control/network/Server.cpp
    control/http/HttpClient.cpp
)
target_link_libraries(control ${PLATFORM_LIBS})

# On Unix-like systems, link ncurses for control
if(UNIX)
    find_package(Curses REQUIRED)
    target_link_libraries(control ${CURSES_LIBRARIES})
    target_include_directories(control PRIVATE ${CURSES_INCLUDE_DIR})
endif()

# Installation
install(TARGETS servercontrol control
    RUNTIME DESTINATION bin
)

# CPack configuration for packaging
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "ServerControl")
set(CPACK_PACKAGE_VERSION "2.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Production-ready server management and remote desktop control")
include(CPack)
