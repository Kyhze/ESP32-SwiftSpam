## What is this?

This will spam Swift Pair BLE beacons to nearby (close range) Windows 10/11 devices, using an ESP32 device (with Bluetooth support).

The result will be an endless stream of Windows notifications prompting the user that a new device was found, granted that Bluetooth is turned on and Swift Pair is enabled (which should be the default).

## Usage: 

```set delay <10-1000>``` from serial console to adjust the spam delay (in ms).

***Note:*** Lower delays do not necessarily translate to more spam.\
**The default is ```90ms```**, which seems to be a relatively sweet spot on my devices, but your mileage may vary.

```set name len <0-19>``` from serial console to adjust the name length.\
Set it to ```0``` to disable advertising device names.\
**The default length is ```8```.**

```set name fixed <name>``` from serial console to set a fixed device name.\
Use ```set name random``` to revert to the randomly generated names.

```set verbose``` from serial console to toggle on/off debugging output.\

## Tested on:
- ESP32-WROOM-32 (Arduino IDE set to ESP32 Dev Module)
- Windows 10 Pro 22H2 (build 19045.5371)
- Windows 11 Pro 24H2 (build 26100.2894)
  
Since version 1.4.0, a runtime limit of 5 minutes was introduced to ensure spamming would remain consistant, and prevent heap exhaustion/OOM issues after running the device for too long.

However, this has the downside of resetting any defined runtime parameters to the default values.\
Feel free to experiment.

PRs are always welcome.

**Compiled on Arduino IDE 2.3.4 with NimBLE-Arduino 2.2.0 and FreeRTOS libraries.**

## Mitigations:
You have two choices:
- Disable Swift Pairing in Windows settings
- Turn off Bluetooth entirely

## Documentation:
[Swift Pair - Microsoft Learn](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair)\
[Bluetooth Low Energy](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy#Technical_details)\
[Bluetooth SIG Assigned Numbers / GATT Specifications](https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/) 
