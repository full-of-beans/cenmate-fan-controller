cmake_minimum_required(VERSION 3.28)
option(CMFC_PICO "Build RP2350 firmware instead of native tests" OFF)

if(CMFC_PICO)
    set(PICO_BOARD seeed_xiao_rp2350 CACHE STRING "CMFC001 MCU")
    if(NOT PICO_BOARD STREQUAL "seeed_xiao_rp2350")
        message(FATAL_ERROR "CMFC001 requires the Seeed XIAO RP2350 board")
    endif()
    set(PICOTOOL_GIT_BRANCH 2.3.0)
    if(NOT PICO_SDK_PATH)
        set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
    endif()
    include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
endif()

project(cenmate VERSION 0.1.0 LANGUAGES C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMFC_PICO)
    if(NOT PICO_SDK_VERSION_STRING VERSION_EQUAL "2.3.0")
        message(FATAL_ERROR "Use Pico SDK 2.3.0 with the corrected XIAO flash size")
    endif()
    pico_sdk_init()
endif()

include(FetchContent)
FetchContent_Declare(yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG 8b4a38dc994a110abaec8a400615567bd996105f
    SYSTEM EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(yyjson)
# Only decoding is used. The application supplies its own fixed allocator.
target_compile_definitions(yyjson PRIVATE YYJSON_DISABLE_WRITER=1 YYJSON_DISABLE_UTILS=1)

find_package(Git QUIET)
set(CMFC_REVISION "unknown")
if(Git_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --verify HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE revision OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(revision MATCHES "^[0-9a-f]+$")
        set(CMFC_REVISION "${revision}")
    endif()
endif()

# Physical flash is 0x200000 bytes. Preserve two 4 KiB sectors at the end.
set(CMFC_STORAGE_OFFSET 0x1fe000)
configure_file(src/build_info.hpp.in generated/build_info.hpp @ONLY)

add_library(cmfc_core STATIC src/model.cpp src/storage.cpp src/runtime.cpp src/protocol.cpp)
target_include_directories(cmfc_core PUBLIC src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(cmfc_core PRIVATE yyjson)
target_compile_options(cmfc_core PRIVATE -Wall -Wextra -Werror -Wformat=2 -Wshadow
    $<$<BOOL:${CMFC_PICO}>:-fno-exceptions;-fno-rtti>)

if(CMFC_PICO)
    add_executable(cenmate src/main.cpp src/platform.cpp src/usb_descriptors.cpp)
    target_link_libraries(cenmate PRIVATE cmfc_core pico_stdlib pico_flash pico_rand pico_unique_id
        hardware_adc hardware_pwm hardware_i2c hardware_watchdog tinyusb_device)
    target_compile_definitions(cenmate PRIVATE PICO_FLASH_ASSUME_CORE1_SAFE=1 PICO_STACK_SIZE=0x1000)
    # Bound linkable firmware flash without changing the board's physical size macro.
    set(PICO_FLASH_SIZE_BYTES_STRING "${CMFC_STORAGE_OFFSET}")
    configure_file("${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/pico_flash_region.template.ld"
        "${CMAKE_CURRENT_BINARY_DIR}/reserved/pico_flash_region.ld")
    pico_add_linker_script_override_path(cenmate "${CMAKE_CURRENT_BINARY_DIR}/reserved"
        FILES pico_flash_region.ld)
    foreach(source src/main.cpp src/platform.cpp src/usb_descriptors.cpp)
        set_source_files_properties(${source} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Werror;-Wformat=2;-Wshadow")
    endforeach()
    pico_enable_stdio_uart(cenmate 0)
    pico_enable_stdio_usb(cenmate 0)
    pico_add_extra_outputs(cenmate)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build-metadata.txt" CONTENT
        "version=${PROJECT_VERSION}\nrevision=${CMFC_REVISION}\nboard=${PICO_BOARD}\nsdk=${PICO_SDK_VERSION_STRING}\ncompiler=${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\nflash_storage_offset=${CMFC_STORAGE_OFFSET}\n")
else()
    enable_testing()
    add_executable(cmfc_tests tests/test.cpp)
    target_link_libraries(cmfc_tests PRIVATE cmfc_core yyjson)
    target_compile_options(cmfc_tests PRIVATE -Wall -Wextra -Werror -Wformat=2 -Wshadow)
    foreach(suite control sensors host power storage protocol)
        add_test(NAME ${suite} COMMAND cmfc_tests ${suite})
    endforeach()
endif()
