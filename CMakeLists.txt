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
set(PICO_BOARD pico2_w CACHE STRING "Board type")

set(PICO_EXTRAS_PATH /Users/wasdwasd0105/pico-extras)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)


project(PicoW_USB_BT_Audio C CXX ASM)


# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable.
add_executable(${PROJECT_NAME}
    src/main.c
    src/pico_w_led.c
    src/usb_sound.c
    src/btstack/bt_audio.c
)



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


target_link_libraries(PicoW_USB_BT_Audio
    pico_stdlib
    pico_btstack_ble
    pico_btstack_classic
    pico_btstack_cyw43
    pico_cyw43_arch_threadsafe_background
    pico_btstack_sbc_encoder
    usb_device
    pico_multicore
    )


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )

# Export binaries like hex, bin, and uf2 files.
pico_add_extra_outputs(${PROJECT_NAME})

# Disable DTR check for USB CDC connection
add_definitions(-DPICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1)

