/*
 * Custom board file for Waveshare RP2350B-Plus-W
 * Based on pico2_w.h from Raspberry Pi Pico SDK 2.2.0.
 *
 * Differences from pico2_w:
 *   - RP2350B (80-pin, 48 GPIOs) instead of RP2350A — DO NOT define PICO_RP2350A
 *   - 16 MB on-board flash (vs 4 MB)
 *   - CYW43/RM2 module wired to GP36-39 (vs GP23/24/25/29 on Pico 2 W)
 *       GP36 = WL_REG_ON   GP37 = WL_DATA/IRQ   GP38 = WL_CS   GP39 = WL_CLK
 *   - LED2 on GP23, LED1 on RM2's internal CYW43 GPIO0 (unchanged)
 *
 * Pin mapping confirmed against Waveshare RP2350B-Plus-W schematic.
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_WAVESHARE_RP2350B_PLUS_W_H
#define _BOARDS_WAVESHARE_RP2350B_PLUS_W_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)

// For board detection
#define WAVESHARE_RP2350B_PLUS_W

// --- RP2350 VARIANT ---
// IMPORTANT: We are RP2350B (80 pins, GP0-47). Do NOT define PICO_RP2350A here,
// otherwise SDK clamps NUM_BANK0_GPIOS=30 and PIO refuses to address GP > 31,
// which would break the CYW43 PIO SPI bus on GP37/38/39.

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- LED ---
// LED1 lives on CYW43's internal GPIO0 (use cyw43_arch_gpio_put).
// LED2 is on RP2350 GP23 (toggle as a normal GPIO).

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 4
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 5
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 18
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 19
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 16
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 17
#endif

// --- FLASH ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

// Waveshare RP2350B-Plus-W ships with 16 MB on-board flash.
pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (16 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

// --- CYW43 / RM2 ---
#ifndef CYW43_WL_GPIO_COUNT
#define CYW43_WL_GPIO_COUNT 3
#endif

#ifndef CYW43_WL_GPIO_LED_PIN
#define CYW43_WL_GPIO_LED_PIN 0
#endif

// SMPS power mode pin lives on CYW43's internal GPIO1 (cyw43_gpio_set).
#ifndef CYW43_WL_GPIO_SMPS_PIN
#define CYW43_WL_GPIO_SMPS_PIN 1
#endif

// VBUS sensed via CYW43 internal GPIO2 (same as Pico 2 W; confirmed on schematic
// by C3 GPIO2 -> VBUS_DET).
#ifndef CYW43_WL_GPIO_VBUS_PIN
#define CYW43_WL_GPIO_VBUS_PIN 2
#endif

// On Waveshare, GP29 is a free user GPIO; CYW43 doesn't share it.
#ifndef CYW43_USES_VSYS_PIN
#define CYW43_USES_VSYS_PIN 0
#endif

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

// CYW43 SPI pins are fixed at compile time.
#ifndef CYW43_PIN_WL_DYNAMIC
#define CYW43_PIN_WL_DYNAMIC 0
#endif

// --- Wireless pin assignment for Waveshare RP2350B-Plus-W ---
// Confirmed against schematic: RM2 module signals routed to RP2350B GP36-39.

// REG_ON: power-up enable for the CYW43 chip
#ifndef CYW43_DEFAULT_PIN_WL_REG_ON
#define CYW43_DEFAULT_PIN_WL_REG_ON 36u
#endif

// SPI data lines — single-pin bidirectional via PIO (DATA_IN == DATA_OUT == HOST_WAKE)
#ifndef CYW43_DEFAULT_PIN_WL_DATA_OUT
#define CYW43_DEFAULT_PIN_WL_DATA_OUT 37u
#endif

#ifndef CYW43_DEFAULT_PIN_WL_DATA_IN
#define CYW43_DEFAULT_PIN_WL_DATA_IN 37u
#endif

#ifndef CYW43_DEFAULT_PIN_WL_HOST_WAKE
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE 37u
#endif

// SPI chip select
#ifndef CYW43_DEFAULT_PIN_WL_CS
#define CYW43_DEFAULT_PIN_WL_CS 38u
#endif

// SPI clock
#ifndef CYW43_DEFAULT_PIN_WL_CLOCK
#define CYW43_DEFAULT_PIN_WL_CLOCK 39u
#endif

#endif
