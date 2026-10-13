#include "sdk_project_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NVM_REQ_OK                  0
#define NVM_REQ_INTEGRITY_FAILED    2

#define LED_PORT   PTD
#define LED_PIN    15U

char txBuff[200];
uint32_t remaining = 0U;

typedef struct {
    uint16_t maxSpeedLimit;        // Speed limit in km/h
    uint8_t  engineTempLimit;      // Temperature limit in °C
    uint16_t minBatteryVoltage;    // Voltage in 0.1 V units
    uint8_t  headlightAutoMode;    // 0 = OFF, 1 = ON
} VehicleConfig_t;

// Default vehicle settings
const VehicleConfig_t Default_VehicleConfig = {
    .maxSpeedLimit     = 120U,
    .engineTempLimit   = 105U,
    .minBatteryVoltage = 120U,
    .headlightAutoMode = 1U
};

// Active settings in RAM
VehicleConfig_t Active_VehicleConfig;
uint8_t NvM_DefaultsUsedFlag = 0U;

// Simulated NvM data
VehicleConfig_t NvM_Block = {
    .maxSpeedLimit     = 100U,
    .engineTempLimit   = 100U,
    .minBatteryVoltage = 115U,
    .headlightAutoMode = 0U
};

uint32_t storedCRC;

// Calculate the block CRC
uint32_t CalculateBlockCRC(const uint8_t *dataBuffer,
                           size_t length)
{
    if (CRC_DRV_Init(INST_CRC_1, &crc_1_Cfg0)
        != STATUS_SUCCESS) {
        return 0U;
    }

    CRC_DRV_WriteData(INST_CRC_1,
                      dataBuffer,
                      (uint32_t)length);

    return CRC_DRV_GetCrcResult(INST_CRC_1);
}

int main(void)
{
    uint32_t calcBlockCRC;

    CLOCK_DRV_Init(&clockMan1_InitConfig0);

    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0,
                  g_pin_mux_InitConfigArr0);

    LPUART_DRV_Init(INST_LPUART_1,
                    &lpUartState0,
                    &lpuart_0_InitConfig0);

    // Save the simulated block CRC
    storedCRC = CalculateBlockCRC(
        (const uint8_t *)&NvM_Block,
        sizeof(NvM_Block)
    );

    // Uncomment to test CRC failure
    NvM_Block.maxSpeedLimit ^= 0xFFFFU;

    sprintf(txBuff,
            "\r\n--- ECU Startup: NvM CRC Check ---\r\n");

    LPUART_DRV_SendData(INST_LPUART_1,
                        (uint8_t *)txBuff,
                        strlen(txBuff));

    while (LPUART_DRV_GetTransmitStatus(
               INST_LPUART_1, &remaining) == STATUS_BUSY) {
    }

    // Check the stored block
    calcBlockCRC = CalculateBlockCRC(
        (const uint8_t *)&NvM_Block,
        sizeof(NvM_Block)
    );

    if (calcBlockCRC == storedCRC) {
        // Use the valid configuration
        memcpy(&Active_VehicleConfig,
               &NvM_Block,
               sizeof(Active_VehicleConfig));

        NvM_DefaultsUsedFlag = 0U;

        PINS_DRV_WritePin(LED_PORT, 16U, 0U);
        PINS_DRV_WritePin(LED_PORT, LED_PIN, 1U);

        sprintf(txBuff,
                "NvM Block Valid: NVM_REQ_OK\r\n"
                "Max Speed Limit: %u km/h\r\n"
                "Engine Temperature Limit: %u C\r\n"
                "Minimum Battery Voltage: %u.%u V\r\n"
                "Automatic Headlights: %s\r\n"
                "Defaults Used: %u\r\n",
                (unsigned)Active_VehicleConfig.maxSpeedLimit,
                (unsigned)Active_VehicleConfig.engineTempLimit,
                (unsigned)(Active_VehicleConfig.minBatteryVoltage / 10U),
                (unsigned)(Active_VehicleConfig.minBatteryVoltage % 10U),
                Active_VehicleConfig.headlightAutoMode ? "ON" : "OFF",
                (unsigned)NvM_DefaultsUsedFlag);
    } else {
        // Load defaults after CRC failure
        memcpy(&Active_VehicleConfig,
               &Default_VehicleConfig,
               sizeof(Active_VehicleConfig));

        NvM_DefaultsUsedFlag = 1U;

        PINS_DRV_WritePin(LED_PORT, 16U, 1U);
        PINS_DRV_WritePin(LED_PORT, LED_PIN, 0U);


        sprintf(txBuff,
                "CRC Failed: NVM_REQ_INTEGRITY_FAILED\r\n"
                "Default configuration loaded into RAM\r\n"
                "Max Speed Limit: %u km/h\r\n"
                "Engine Temperature Limit: %u C\r\n"
                "Minimum Battery Voltage: %u.%u V\r\n"
                "Automatic Headlights: %s\r\n"
                "Defaults Used: %u\r\n",
                (unsigned)Active_VehicleConfig.maxSpeedLimit,
                (unsigned)Active_VehicleConfig.engineTempLimit,
                (unsigned)(Active_VehicleConfig.minBatteryVoltage / 10U),
                (unsigned)(Active_VehicleConfig.minBatteryVoltage % 10U),
                Active_VehicleConfig.headlightAutoMode ? "ON" : "OFF",
                (unsigned)NvM_DefaultsUsedFlag);
    }

    LPUART_DRV_SendData(INST_LPUART_1,
                        (uint8_t *)txBuff,
                        strlen(txBuff));

    while (LPUART_DRV_GetTransmitStatus(
               INST_LPUART_1, &remaining) == STATUS_BUSY) {
    }

    while (1) {
    	//infinite loop to keep the program alive
    }
}
