#pragma once

#include <cstdint>

// Serial-only hardware diagnostics for bring-up. Stateless: every function
// reads live from the chip/heap/PSRAM and prints immediately.
namespace HardwareDiagnostics {

void printBootBanner();
void printChipInfo();
void printMemoryInfo();
void printDisplayStatus(bool initialized);

} // namespace HardwareDiagnostics
