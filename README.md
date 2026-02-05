# HiBean ESP32 BLE Roaster Control for Skywalker v1

This code implements the Community Edition (Skywalker Comm) roaster controller for ESP32 series boards, and has been tested with Waveshare ESP32-S3-Zero and Espressif ESP32C6-devkitC, both connected to a Skywalker v1 roaster.  This is NOT for v2 Skywalker.

First off, a hat-tip to jmoore52, mrnoone5313 and Nirecue and their great work at [Skywalker Roaster Labs](https://github.com/jmoore52/SkywalkerRoaster).  It is their early work that inspried this version.

The [HiBean](https://www.hibean.fun/en/) roasting team generously provides support for a "Community" version of the SkyWalker roaster controller, which is implemented in Arduino on esp32.  This community version is open-source (GPLv3) and is freely available to fork and modify, or to use the pre-compiled binary versions which are provided with each [release](https://github.com/MagnmCI/SkiBeanCommunity/releases).

## Available binaries (.bin)
With every milestone featureset that's felt to be release-worthy, a release is generated and pre-compiled binaries are automatically generated for the supported esp32 platforms.

Use these .bin's with your favorite esp32 flash tool (ie. https://web.esphome.io/), and go.  After the flash, the on-board LED should alternate red/blue until it pairs with a BLE client such as Hibean.  If you need to re-flash an existing S3-Zero, hold down the BOOT button as you unplug and re-plug the device into USB - that will bring it up in DFU mode wherein it's ready to take a new flash.

## IDE / Build Notes
This repository enables development with PlatformIO-style IDE extensions to VSCode. However, you must specifically utilize the ['pioarduino'](https://github.com/pioarduino) fork of PlatformIO since it has the most current board definitions for Arduino based development.

To set up your development environment, install VSCode and the pioarduino extension, and checkout this repo to a local directory.

Within pioarduino, "Open" an existing project and select the 'platform.ini' file included in this repo, and the rest of the development enviornment and required libs and toolchains will automatically be installed and configured - you're ready to build and flash.

## **Control Commands & Behavior**
HiBean and this roaster control software loosely implement [TC4 commands](https://github.com/greencardigan/TC4-shield/blob/master/applications/Artisan/aArtisan/trunk/src/aArtisan/commands.txt) for the majority of roaster functions, and are enumerated below.


## **Available Commands (case-INsensitive)**


### **Utility Commands**
| **Command**     | **Description** |
|-----------------|----------------|
| `OT2;XX`        | Sets the vent power to **XX%**. |
| `OFF`           | Shuts down the system. |
| `ESTOP`         | Emergency stop: Sets heater to 0% and vent to 100%. |
| `DRUM;XX`       | Starts/stops the drum motor (1 = ON, 0 = OFF). |
| `FILTER;XX`     | Controls filter fan power (1 fastest - 4 slowest; 0 off). |
| `COOL;XX`       | Activates cooling function (0-100%). |
| `CHAN`          | Sends active channel configuration. |
| `UNITS;C/F`     | Sets temperature units to **Celsius (C)** or **Fahrenheit (F)**. |

### **PID Control Commands**
| **Command**     | **Description** |
|-----------------|----------------|
| `PID;ON`        | Enables PID control (automatic mode). |
| `PID;OFF`       | Disables PID control (switches to manual mode). |
| `PID;SV;XXX`    | Sets the PID **setpoint temperature** (XXX is in °C, e.g., `PID;SV;250` sets the target to 250°C). |
| `PID;T;PP.P;II.I;DD.D`   |  Apply provided tunings to the PID control (not persisted). |
| `PID;CT;XXXX`    | Temporarily sets PID cycle (sample) time to XXXX ms (not persisted). |
| `PID;PM;E`      | Temporarily change pMode: E = P_ON_E to M = P_ON_M(default), or reverse (not persisted). |
| `OT1;XX`        | Manually sets heater power to **XX%** when PID is off; sets the MAX heat power level when PID is on. |
| `READ`          | Retrieves current temperature, set temperature, heater, and vent power. |

## **Usage Example**
- Enable PID control:
  ```
  PID;ON
  ```
- Set target temperature to 250°C:
  ```
  PID;SV;250
  ```
- Manually set heater to 70% power, or PID max power limit (see OT1 above):
  ```
  OT1;70
  ```
- Read current system status:
  ```
  READ
  ```
Note that this release and those going forward expose PID controls via BLE and the details of which can be seen in the SkiBLE header file.  This change was primarly because TC4 doesn't support a complete set of PID commands, and there is no option to read current state over TC4, only write.

## Volunteer Efforts
This codebase is a volunteer effort, so please understand that you are on your own with this software.  You can log issues against this codebase and the developer may address them as they have time.

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3).
