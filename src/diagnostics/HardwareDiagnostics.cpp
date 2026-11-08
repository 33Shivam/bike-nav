#include "HardwareDiagnostics.h"
#include <Arduino.h>
#include <esp_chip_info.h>
#include <esp_flash.h>

namespace HardwareDiagnostics {

void printBootBanner() {
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F(" BikeNav - Hardware Bring-up"));
    Serial.println(F(" Board: Waveshare ESP32-S3-LCD-1.28 (Non-Touch)"));
    Serial.println(F("========================================"));
}

void printChipInfo() {
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);

    uint32_t flashSize = 0;
    esp_flash_get_size(nullptr, &flashSize);

    Serial.printf("Chip model     : %s\n", ESP.getChipModel());
    Serial.printf("Chip revision  : %d\n", chipInfo.revision);
    Serial.printf("CPU cores      : %d\n", chipInfo.cores);
    Serial.printf("CPU frequency  : %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash size     : %u MB\n", flashSize / (1024 * 1024));
}

void printMemoryInfo() {
    Serial.printf("Heap total     : %u bytes\n", ESP.getHeapSize());
    Serial.printf("Heap free      : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Heap min free  : %u bytes\n", ESP.getMinFreeHeap());

    if (psramFound()) {
        Serial.printf("PSRAM total    : %u bytes\n", ESP.getPsramSize());
        Serial.printf("PSRAM free     : %u bytes\n", ESP.getFreePsram());
    } else {
        Serial.println(F("PSRAM          : not detected"));
    }
}

void printDisplayStatus(bool initialized) {
    Serial.printf("Display init   : %s\n", initialized ? "OK" : "FAILED");
}

} // namespace HardwareDiagnostics
