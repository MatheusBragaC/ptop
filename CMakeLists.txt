cmake_minimum_required(VERSION 3.10)
project(ptop C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

# Compiler Warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")

# Include Directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ui
)

# Source Files configuration
# We will define sources for each module to keep it organized

# Common
set(COMMON_SOURCES
    src/common/utils.h 
    src/common/cfg.h
    src/common/logger.h
    src/common/logger.c
)

# Core (Data Collection)
set(CORE_SOURCES
    src/core/model.h
    src/core/collector.h
    src/core/collector_linux.c
    src/core/parser.c
    src/core/parser.h
)

# UI (Display & TUI)
set(UI_SOURCES
    src/ui/display.c
    src/ui/display.h
    src/ui/tui.c
    src/ui/tui.h
)

# Main Executable
add_executable(ptop 
    src/main.c
    ${COMMON_SOURCES}
    ${CORE_SOURCES}
    ${UI_SOURCES}
)

add_subdirectory(tests)

