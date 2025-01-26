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
    {0x010000, "Telephony (Cordless telephony, Modem, Headset service, ...)"}
};

// Major Device Class
const std::map<int, std::string> MajorDeviceClasses = {
    {1, "Computer (desktop, notebook, PDA, organizer, ...)"},
    {2, "Phone (cellular, cordless, pay phone, modem, ...)"},
    {4, "Audio/Video (headset, speaker, stereo, video display, VCR, ...)"},
    {5, "Peripheral (mouse, joystick, keyboard, ...)"},
    {6, "Imaging (printer, scanner, camera, display, ...)"},
    {9, "Health"}
};

// Minor Device Class (nested by Major Device Class)
const std::map<int, std::map<int, std::string>> MinorDeviceClasses = {
    {1, { // Computer
        {1, "Desktop Workstation"},
        {3, "Laptop"},
        {6, "Wearable Computer (watch size)"}
    }},
    {2, { // Phone
        {3, "Smartphone"}
    }},
    {4, { // Audio/Video
        {1, "Wearable Headset Device"},
        {2, "Hands-free Device"},
        {4, "Microphone"},
        {5, "Loudspeaker"},
        {6, "Headphones"},
        {9, "Set-top box"},
        {10, "HiFi Audio Device"}
    }},
    {5, { // Peripheral
        {1, "Keyboard"},
        {2, "Pointing Device"},
        //{3, "Combo Keyboard/Pointing Device"},
        {4, "Joystick"},
        {5, "Gamepad"}
    }},
    {6, { // Imaging
        {4, "Display"},
        {5, "Camera"},
        {6, "Scanner"},
        {7, "Printer"}
    }},
    {9, { // Health
        {6, "Heart/Pulse Rate Monitor"}
    }}
};

} // namespace BluetoothCoD

#endif // BLUETOOTH_COD_H
