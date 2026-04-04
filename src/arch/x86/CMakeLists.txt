set(TARGET_NAME X86)
set(SOURCE_FILE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(boot)
add_subdirectory(traps)

file(GLOB SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
                      "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

if (SOURCE_FILE)
    add_library(${TARGET_NAME} "")
    target_sources(${TARGET_NAME} PUBLIC ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PUBLIC Boot Traps)
else()
    add_library(${TARGET_NAME} INTERFACE)
    target_link_libraries(${TARGET_NAME} INTERFACE Boot Traps)
endif()