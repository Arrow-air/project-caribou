// CBC_PCB pin map — extracted from CBC_PCB.kicad_sch netlist (ordered board, PR #51)
// MCU: U301 ESP32-S3-WROOM-1-N16R8
#pragma once

// --- Power switching / kill chain ---
// PRECHARGE_ESP_SIGNAL -> R211 -> Q202 (LMUN5235) -> U206 opto -> precharge FET.
// HIGH = precharge path on.
#define PIN_PRECHARGE 4

// SCHMITT_OUT from U202 (74LVC1G17), driven by the divided 12V_TRIGGER_SIG (J501.7).
// HIGH = kill trigger present. The same signal drives U203 (CPC1106N, 1-Form-B
// normally-CLOSED SSR), which opens the MOSFET_CONTROL_SIGNAL path in hardware,
// independent of firmware.
#define PIN_KILL_SENSE 5

// ESP_ENABLE_SIGNAL -> U201 (74LVC2G02) cross-coupled NOR latch, SET input (1A).
// Pulse HIGH to latch the main switch ON. 10k pulldown (R201) keeps it low at boot.
// NOTE: the latch has NO reset input from the ESP32 — firmware can arm but can
// NOT disarm. Only a battery power-cycle (POR network D201/R205/C202) resets it.
#define PIN_LATCH_SET 6

// --- Temperature (DS18B20, separate 1-Wire buses) ---
#define PIN_TEMP1 15 // TEMP1_DQ -> U501
#define PIN_TEMP2 7  // TEMP2_DQ -> U502

// --- Shared SPI (ESP32-S3 default FSPI pins) ---
#define PIN_SPI_MOSI 11
#define PIN_SPI_SCK 12
#define PIN_SPI_MISO 13

// --- CAN 1: battery bus (Tattu) ---
// U402 MCP2515 (16 MHz crystal X402) -> U403 TJA1049T/3 -> CANH_BAT/CANL_BAT -> CN201.
// 120R termination R405 via jumper H401.
#define PIN_CAN_BAT_CS 10
#define PIN_CAN_BAT_INT 21
#define PIN_CAN_BAT_RST 48

// --- CAN 2: drone bus (DroneCAN, galvanically isolated) ---
// U401 MCP2515 (16 MHz crystal X401) -> U404 ADM3053 (isolated, GND_CD domain)
// -> L401 -> J501 pins 5 (CANL) / 6 (CANH). 120R termination R408 via jumper H402.
#define PIN_CAN_DRONE_CS 14
#define PIN_CAN_DRONE_INT 47
#define PIN_CAN_DRONE_RST 9
