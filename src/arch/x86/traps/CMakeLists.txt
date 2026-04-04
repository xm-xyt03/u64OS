set(TARGET_NAME Traps)
set(SOURCE_FILE)

file(GLOB SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/*.S"
                      "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
                      "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

if (SOURCE_FILE)
    add_library(${TARGET_NAME} ${SOURCE_FILE})
else()
    message(FATAL_ERROR "No source files provided for traps.")
endif()