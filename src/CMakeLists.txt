cmake_minimum_required(VERSION 3.22)
project(u64OS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_language(C)
enable_language(CXX)
enable_language(ASM)

if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Unsupported compilation environment.")
endif()

if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/arch/x86/linker.lds")
    include_directories("arch/x86/include/")
else()
    message(FATAL_ERROR "Unsupported architecture.")
endif()

add_compile_options(-pipe -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mcmodel=large -fno-asynchronous-unwind-tables -fno-exceptions)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib -z max-page-size=0x1000 -Wl,--build-id=none -static")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(arch)
add_subdirectory(kernel)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/for_occupation.cpp" "")
add_executable(kernel.bin "${CMAKE_CURRENT_BINARY_DIR}/for_occupation.cpp")

target_link_libraries(kernel.bin
    PRIVATE
        Arch
        Kernel
)