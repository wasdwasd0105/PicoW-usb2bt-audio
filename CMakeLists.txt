# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")
set(FAMILY rp2040)
set(BOARD pico_sdk)


# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)


project(PicoW_USB_BT_Audio C CXX ASM)


# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable.
add_executable(${PROJECT_NAME}
    src/main.c
    src/pico_w_led.c
    src/btstack/bt_audio.c
    src/tinyusb/uac.c
    src/tinyusb/usb_descriptors.c
)



target_compile_definitions(PicoW_USB_BT_Audio PRIVATE
    CYW43_LWIP=0
    PICO_FLASH_ASSUME_CORE1_SAFE=1
)


target_link_libraries(PicoW_USB_BT_Audio
    pico_stdlib
    pico_btstack_ble
    pico_btstack_classic
    pico_btstack_cyw43
    pico_cyw43_arch_threadsafe_background
    pico_btstack_sbc_encoder
    pico_multicore
    tinyusb_device
    tinyusb_board
)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Export binaries like hex, bin, and uf2 files.
pico_add_extra_outputs(${PROJECT_NAME})

