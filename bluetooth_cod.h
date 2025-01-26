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
    {1, {
        {1, "Desktop Workstation"},
        {2, "Server-class Computer"},
        {3, "Laptop"},
        {4, "Handheld PC/PDA (clamshell)"},
        {5, "Palm-size PC/PDA"},
        {6, "Wearable Computer (watch size)"},
        {7, "Tablet"}
    }},
    {2, {
        {1, "Cellular"},
        {2, "Cordless"},
        {3, "Smartphone"},
        {4, "Wired Modem or Voice Gateway"},
        {5, "Common ISDN Access"}
    }},
    {4, {
        {1, "Wearable Headset Device"},
        {2, "Hands-free Device"},
        {4, "Microphone"},
        {5, "Loudspeaker"},
        {6, "Headphones"},
        {7, "Portable Audio"},
        {8, "Car Audio"},
        {9, "Set-top box"},
        {10, "HiFi Audio Device"}
    }},
};

} // namespace BluetoothCoD

#endif // BLUETOOTH_COD_H
