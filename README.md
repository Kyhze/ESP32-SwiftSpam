**- What is this?**

This will spam Swift Pair BLE beacons to nearby (very close range) Windows devices using an ESP32 device (with Bluetooth support).

The result will be an endless stream of Windows notifications prompting the user that a new device with randomly generated names was found, granted that Bluetooth is turned on and Swift Pair is enabled (which should be the default).

**- Tested on:**
- ESP32-WROOM-32
- Windows 11 Pro 24H2 (build 26100.2605)
- My ESP32 seems to occasionally crash (most likely caused by OOM/heap exhaustion) after letting it run for an extended amount of time (~10 mins). This results in:
```
Guru Meditation Error: Core  0 panic'ed (InstrFetchProhibited). Exception was unhandled.

Core  0 register dump:
PC      : 0x00000000  PS      : 0x00060f30  A0      : 0x800d2832  A1      : 0x3ffefcf0  
A2      : 0x00000003  A3      : 0x00060023  A4      : 0x00060023  A5      : 0x3f40e398  
A6      : 0x00000001  A7      : 0x3ffc32e4  A8      : 0x800812e0  A9      : 0x3ffefcc0  
A10     : 0x3ffc32e4  A11     : 0xffffffff  A12     : 0x00000001  A13     : 0x3ffed360  
A14     : 0x3ffc5790  A15     : 0x00000001  SAR     : 0x0000001f  EXCCAUSE: 0x00000014  
EXCVADDR: 0x00000000  LBEG    : 0x400910d4  LEND    : 0x400910ea  LCOUNT  : 0xffffffff  


Backtrace: 0xfffffffd:0x3ffefcf0 0x400d282f:0x3ffefd10 0x400943ae:0x3ffefd30
```
But since the device automatically reboots when that happens, it doesn't really disrupt the intended behavior (unless you've manually adjusted the delay, in which case it'll get reset).

Compiled on Arduino IDE 2.3.4 with NimBLE-Arduino 2.2.0 library.

**- Usage:**

```set delay <10-1000>``` from serial console to adjust the spam delay (in ms).

Note that lower delays do not necessarily translate to more spam.\
The default is 130ms, which seems to be a relatively sweet spot on my devices, but your mileage may vary.