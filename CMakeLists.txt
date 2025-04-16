# Copyright (c) 2022 Mr. Green's Workshop https://www.MrGreensWorkshop.com
# SPDX-License-Identifier: Apache-2.0

set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})


# Set minimum CMake version
cmake_minimum_required(VERSION 3.17)

# Include the subsidiary .cmake file to get the SDK
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set the name and version of the project
project(PicoW_USB_BT_Audio VERSION 1.0.0)

# Set the board to Raspberry Pi Pico W
set(PICO_BOARD pico_w)

# Set top source code folder
set(SOURCE_FOLDER "src")

# Set build type (options: 'Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo')
set(default_build_type "Release")

# Set CMAKE_BUILD_TYPE
set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build, options are: 'Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo'." FORCE)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

# Initialize the SDK
pico_sdk_init()

# Add an executable target for the project
add_executable(${PROJECT_NAME})

# Get all the pio files, when the glob value changes, cmake will run again and update the files
file(GLOB_RECURSE pio_src CONFIGURE_DEPENDS "${SOURCE_FOLDER}/*.pio")

# Convert List to String
STRING(REPLACE ";" "\n" PIO_SRC_STR "${pio_src}")

# Print the pio source file list
message(STATUS "PIO code source files: \n${PIO_SRC_STR}")

# If there are any PIO source files, include them to the build.
if (NOT pio_src STREQUAL "")
  pico_generate_pio_header(${PROJECT_NAME} ${pio_src})
endif()

# Get all C and C++ files, when the glob value changes, cmake will run again and update the files
file(GLOB_RECURSE app_src CONFIGURE_DEPENDS "${SOURCE_FOLDER}/*.c" "${SOURCE_FOLDER}/*.cpp" )

# Convert List to String
STRING(REPLACE ";" "\n" APP_SRC_STR "${app_src}")

# Print the C and C++ source file list
message(STATUS "C and C++ source files: \n${APP_SRC_STR}")

# Add C and C++ source files to the build
target_sources(${PROJECT_NAME} PRIVATE ${app_src})

set(BTSTACK_3RD_PARTY_PATH ${PICO_SDK_PATH}/lib/btstack/3rd-party)
#set(BTSTACK_EXAMPLE_PATH $ENV{PICO_SDK_PATH}/lib/btstack/3rd-party)



target_compile_definitions(PicoW_USB_BT_Audio PRIVATE
        AUDIO_FREQ_MAX=48000

        # ours are zero based, so say so
        PICO_USBDEV_USE_ZERO_BASED_INTERFACES=1

        # need large descriptor
        PICO_USBDEV_MAX_DESCRIPTOR_SIZE=256


        PICO_USBDEV_ISOCHRONOUS_BUFFER_STRIDE_TYPE=1
        PICO_USBDEV_ENABLE_DEBUG_TRACE

        CYW43_LWIP=0
)


# Link the Project to extra libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  pico_stdlib
  pico_btstack_ble
  pico_btstack_classic
  pico_btstack_cyw43
  pico_cyw43_arch_threadsafe_background
  pico_btstack_sbc_encoder
  usb_device
  pico_multicore
)

# Including header files directly from project directory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Export binaries like hex, bin, and uf2 files.
pico_add_extra_outputs(${PROJECT_NAME})

# Enable or Disable UART
#pico_enable_stdio_uart(${PROJECT_NAME} 1)

# Enable or Disable USB CDC
#pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Disable DTR check for USB CDC connection
add_definitions(-DPICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1)
