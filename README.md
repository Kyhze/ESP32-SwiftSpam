## What is this?

This will spam Swift Pair BLE beacons to nearby (close range) Windows 10/11 devices, using an ESP32 device (with Bluetooth support).

The result will be an endless stream of Windows notifications prompting the user that a new device was found, granted that Bluetooth is turned on and Swift Pair is enabled (which should be the default).

## Usage: 

The following commands are available from the serial console:

```set delay <10-1000>``` To adjust the spam delay (in ms).

**Note:** Lower delays do not necessarily translate to more spam.
>*The default is ```90ms```*, which seems to be a relatively sweet spot on my devices, but your mileage may vary.

```set name len <0-19>``` To adjust the random device names length.\
Set it to ```0``` to disable advertising any device names.
>*The default length is ```8```.*

```set name fixed <name>``` To set a fixed device name.\
The maximum length of fixed device names is also limited to 19.\
Use ```set name random``` to revert to randomly generated names.
>*The default is to not advertise any device name*


```set verbose``` To toggle on/off debugging output.

```set spam``` To toggle on/off beacon advertising.

```reset``` To reboot the ESP32 device.

```help``` To show all available commands.

## Tested on:
- ESP32-WROOM-32 (Arduino IDE set to ESP32 Dev Module)
- Windows 10 Pro 22H2 (build 19045.5371)
- Windows 11 Pro 24H2 (build 26100.2894)
  
Since version 1.4.0, a runtime limit of 5 minutes was introduced to ensure spamming would remain consistant, and prevent heap exhaustion/OOM issues after running the device for too long.

However, this has the downside of resetting any defined runtime parameters to the default values.\
Feel free to experiment.

PRs are always welcome.

- Pre-compiled artifacts are available [here](https://github.com/Kyhze/ESP32-SwiftSpam/actions/workflows/build.yml)

## Mitigations:
You have two choices:
- Disable Swift Pairing in Windows settings
- Turn off Bluetooth entirely

## Documentation:
[Swift Pair - Microsoft Learn](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth-swift-pair)\
[Bluetooth Low Energy](https://en.wikipedia.org/wiki/Bluetooth_Low_Energy#Technical_details)\
[Bluetooth SIG Assigned Numbers / GATT Specifications](https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/) 
