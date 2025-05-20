#ifndef __PSRAM_BUFFER_H
#define __PSRAM_BUFFER_H

#include "Arduino.h"
#include <esp_heap_caps.h>

// Buffer sizes for SD card operations
#define PSRAM_BUFFER_SIZE_DEFAULT (32 * 1024)
#define PSRAM_BUFFER_SIZE_LARGE (3 * 1024 * 1024)
#define PSRAM_BUFFER_SIZE_MAX (7 * 1024 * 1024)

// Percentage of total PSRAM to use (0.0 to 1.0)
#define PSRAM_USAGE_PERCENT 0.7 // Use up to 70% of available PSRAM

// Class to manage PSRAM buffer for SD card operations
class PSRAMBuffer {
private:
    uint8_t* buffer;
    size_t bufferSize;
    bool initialized;

public:
    PSRAMBuffer(size_t size = PSRAM_BUFFER_SIZE_DEFAULT) : buffer(nullptr),
                                                           bufferSize(size),
                                                           initialized(false)
    {
    }

    // Calculate optimal buffer size based on available PSRAM
    size_t calculateOptimalSize(size_t requestedSize = 0)
    {
        if (!psramFound())
        {
            return PSRAM_BUFFER_SIZE_DEFAULT; // Default if no PSRAM
        }

        size_t totalPsram = ESP.getPsramSize();
        size_t freePsram = ESP.getFreePsram();
        size_t maxAllowedSize = freePsram * PSRAM_USAGE_PERCENT;

        // If a specific size is requested, try to honor it within limits
        if (requestedSize > 0)
        {
            return min(requestedSize, maxAllowedSize);
        }

        // Otherwise calculate based on available PSRAM
        if (maxAllowedSize > PSRAM_BUFFER_SIZE_MAX)
        {
            return PSRAM_BUFFER_SIZE_MAX;
        }
        else if (maxAllowedSize > PSRAM_BUFFER_SIZE_LARGE)
        {
            return PSRAM_BUFFER_SIZE_LARGE;
        }
        else
        {
            return min(maxAllowedSize, (size_t)PSRAM_BUFFER_SIZE_DEFAULT);
        }
    }

    ~PSRAMBuffer() {
        if (buffer != nullptr) {
            free(buffer);
            buffer = nullptr;
        }
    }

    bool init(size_t requestedSize = 0)
    {
        if (initialized)
        {
            // If already initialized but a new size is requested, reinitialize
            if (requestedSize > 0 && requestedSize != bufferSize)
            {
                if (buffer != nullptr)
                {
                    free(buffer);
                    buffer = nullptr;
                }
                initialized = false;
            }
            else
            {
                return true; // Already initialized with the right size
            }
        }

        // Calculate optimal buffer size
        if (requestedSize > 0)
        {
            bufferSize = calculateOptimalSize(requestedSize);
        }
        else if (bufferSize == 0)
        { // If no size was set in constructor
            bufferSize = calculateOptimalSize();
        }

        Serial.printf("Initializing PSRAM buffer with size: %u bytes (%.2f KB)\n",
                      bufferSize, bufferSize / 1024.0);

        // Try to allocate from PSRAM first
        if (psramFound() && heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > bufferSize)
        {
            buffer = (uint8_t*)heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM);
            if (buffer != nullptr) {
                initialized = true;
                Serial.println("Buffer allocated in PSRAM");
                return true;
            }
        }

        // Fallback to regular memory if PSRAM allocation fails
        Serial.println("PSRAM allocation failed, falling back to regular memory");
        buffer = (uint8_t*)malloc(bufferSize);
        if (buffer != nullptr) {
            initialized = true;
            return true;
        }

        Serial.println("Buffer allocation failed completely");
        return false;
    }

    uint8_t* getBuffer() {
        return buffer;
    }

    size_t getSize() {
        return bufferSize;
    }

    bool isInitialized() {
        return initialized;
    }

    bool isPSRAM() {
        return heap_caps_check_integrity_addr((intptr_t)buffer, true);
    }

    // Resize the buffer to a new size
    bool resize(size_t newSize)
    {
        if (newSize == bufferSize)
        {
            return true; // Already the right size
        }

        // Calculate optimal size based on request
        size_t optimalSize = calculateOptimalSize(newSize);

        // If we're already initialized, we need to reallocate
        if (initialized)
        {
            uint8_t *newBuffer = nullptr;

            // Try to allocate from PSRAM first
            if (psramFound() && heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > optimalSize)
            {
                newBuffer = (uint8_t *)heap_caps_malloc(optimalSize, MALLOC_CAP_SPIRAM);
            }

            // Fallback to regular memory
            if (newBuffer == nullptr)
            {
                newBuffer = (uint8_t *)malloc(optimalSize);
            }

            if (newBuffer == nullptr)
            {
                return false; // Allocation failed
            }

            // Copy data from old buffer if it exists
            if (buffer != nullptr)
            {
                // Copy the minimum of the two sizes
                memcpy(newBuffer, buffer, min(bufferSize, optimalSize));
                free(buffer);
            }

            buffer = newBuffer;
            bufferSize = optimalSize;
            return true;
        }
        else
        {
            // Not initialized yet, just update the size and init will handle it
            bufferSize = optimalSize;
            return init();
        }
    }
};

// Function to print PSRAM information
inline void printPSRAMInfo(bool showRecommendation = true)
{
    Serial.println("\n=== PSRAM Information ===");

    // Check if PSRAM is enabled
    if (psramFound()) {
        Serial.println("PSRAM is enabled");

        // Get PSRAM size and usage
        size_t psramSize = ESP.getPsramSize();
        size_t freePsram = ESP.getFreePsram();
        size_t usedPsram = psramSize - freePsram;
        float usagePercent = usedPsram * 100.0 / psramSize;

        Serial.printf("Total PSRAM: %u bytes (%.2f KB / %.2f MB)\n",
                     psramSize, psramSize / 1024.0, psramSize / (1024.0 * 1024.0));
        Serial.printf("Free PSRAM: %u bytes (%.2f KB / %.2f MB)\n",
                     freePsram, freePsram / 1024.0, freePsram / (1024.0 * 1024.0));
        Serial.printf("Used PSRAM: %u bytes (%.2f KB / %.2f MB)\n",
                      usedPsram, usedPsram / 1024.0, usedPsram / (1024.0 * 1024.0));
        Serial.printf("PSRAM Usage: %.2f%%\n", usagePercent);

        // Show heap info
        Serial.println("\nPSRAM Heap Information:");
        Serial.printf("Largest free block: %u bytes (%.2f KB)\n",
                      heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
                      heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024.0);

        // Show recommendations if requested
        if (showRecommendation)
        {
            Serial.println("\nRecommended Buffer Sizes:");
            size_t maxAllowedSize = freePsram * PSRAM_USAGE_PERCENT;

            Serial.printf("Maximum recommended buffer: %u bytes (%.2f KB)\n",
                          maxAllowedSize, maxAllowedSize / 1024.0);

            // Calculate optimal buffer sizes for different scenarios
            size_t optimalDefault = min(maxAllowedSize, (size_t)PSRAM_BUFFER_SIZE_DEFAULT);
            size_t optimalLarge = min(maxAllowedSize, (size_t)PSRAM_BUFFER_SIZE_LARGE);
            size_t optimalMax = min(maxAllowedSize, (size_t)PSRAM_BUFFER_SIZE_MAX);

            Serial.printf("Default buffer: %u bytes (%.2f KB)\n",
                          optimalDefault, optimalDefault / 1024.0);
            Serial.printf("Large buffer: %u bytes (%.2f KB)\n",
                          optimalLarge, optimalLarge / 1024.0);
            Serial.printf("Maximum buffer: %u bytes (%.2f KB)\n",
                          optimalMax, optimalMax / 1024.0);
        }
    } else {
        Serial.println("PSRAM is not enabled or not found");
    }

    Serial.println("=========================\n");
}

#endif // __PSRAM_BUFFER_H
