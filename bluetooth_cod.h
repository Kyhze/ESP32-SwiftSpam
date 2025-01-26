#ifndef BLUETOOTH_COD_H
#define BLUETOOTH_COD_H

#include <map>
#include <string>

namespace BluetoothCoD {

// Major Service Class (bit flags)
const std::map<int, std::string> MajorServiceClasses = {
    {0x200000, "Audio (Speaker, Microphone, Headset service, ...)"},
    {0x100000, "Networking (LAN, Ad hoc, ...)"},
    {0x080000, "Rendering (Printing, Speakers, ...)"},
    {0x040000, "Capturing (Scanner, Microphone, ...)"},
    {0x020000, "Object Transfer (v-Inbox, v-Folder, ...)"},
    {0x010000, "Telephony (Cordless telephony, Modem, Headset service, ...)"},
    {0x008000, "Information (WEB-server, WAP-server, ...)"}
};

// Major Device Class
const std::map<int, std::string> MajorDeviceClasses = {
    {0, "Miscellaneous"},
    {1, "Computer (desktop, notebook, PDA, organizer, ...)"},
    {2, "Phone (cellular, cordless, pay phone, modem, ...)"},
    {3, "LAN/Network Access Point"},
    {4, "Audio/Video (headset, speaker, stereo, video display, VCR, ...)"},
    {5, "Peripheral (mouse, joystick, keyboard, ...)"},
    {6, "Imaging (printer, scanner, camera, display, ...)"},
    {7, "Wearable"},
    {8, "Toy"},
    {9, "Health"}
};

// Minor Device Class (nested by Major Device Class)
const std::map<int, std::map<int, std::string>> MinorDeviceClasses = {
    {1, { // Computer
        {1, "Desktop Workstation"},
        {2, "Server-class Computer"},
        {3, "Laptop"},
        {4, "Handheld PC/PDA (clamshell)"},
        {5, "Palm-size PC/PDA"},
        {6, "Wearable Computer (watch size)"},
        {7, "Tablet"}
    }},
    {2, { // Phone
        {1, "Cellular"},
        {2, "Cordless"},
        {3, "Smartphone"},
        {4, "Wired Modem or Voice Gateway"}
    }},
    {4, { // Audio/Video
        {1, "Wearable Headset Device"},
        {2, "Hands-free Device"},
        {4, "Microphone"},
        {5, "Loudspeaker"},
        {6, "Headphones"},
        {7, "Portable Audio"},
        {8, "Car Audio"},
        {9, "Set-top box"},
        {10, "HiFi Audio Device"},
        {15, "Video Display and Loudspeaker"}
    }},
    {5, { // Peripheral
        {1, "Keyboard"},
        {2, "Pointing Device"},
        {3, "Combo Keyboard/Pointing Device"},
        {4, "Joystick"},
        {5, "Gamepad"},
        {6, "Remote Control"}
    }},
    {6, { // Imaging
        {4, "Display"},
        {5, "Camera"},
        {6, "Scanner"},
        {7, "Printer"}
    }},
    {7, { // Wearable
        {1, "Wristwatch"},
        {5, "Glasses"}
    }},
    {9, { // Health
        {1, "Blood Pressure Monitor"},
        {2, "Thermometer"},
        {3, "Weighing Scale"},
        {4, "Glucose Meter"},
        {5, "Pulse Oximeter"},
        {6, "Heart/Pulse Rate Monitor"},
        {8, "Step Counter"}
    }}
};

} // namespace BluetoothCoD

#endif // BLUETOOTH_COD_H
