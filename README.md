# HiBean ESP32 BLE Roaster Control for Skywalker v1

This code implements an Arduino based roaster controller for ESP32 series boards, tested with Waveshare ESP32-S3-Zero and Espressif ESP32C6-devkitC both connected to a Skywalker v1 roaster.  This is NOT for v2 Skywalker.

Originally this code base was a spike to implement PID control on an older codebase (thus the name "QuickSV"), but it evolved into a refactor of the entire Skywalker v1 esp32 codebase and that code is what is shown here.

Notable changes of this build (1.1.4) over prior builds...

* Espressif ESP32 moved away from Bluedroid (a full bluetooth stack) to NimBLE (a BLE only stack) which required quite a bit of work to ensure solid BLE performance. (see build notes below)
* An improved and object'y roaster-read interface for reliable, fast and non-blocking temperature reads from the stock roaster temperature probe without the need for additional signal filtering.
* CMD inbound-message queueing which enables HiBean automations to send mulitple, rapid commands and not get lost by the ESP32.
* PID tuning to be less nervous.

## Build Notes
When setting up your IDE, be sure you have the EXACT board selected, and if not, install it via Boards Manager.  "ESP32 Family" is not sufficient.  There are very specific pin definitions which are unique to each board.

When installing the espressif ESP32 library, the BLE improvements require the most recent esp32-arduino core as there are fixes in there specifically to support Bluedroid->NimBLE migration.  Those changes are in the 3.3.3 release of the esp32-arduino core, so that or newer should work.

When building this sketch, you will need to have PID_V1 library installed in your dev envionment.

If you have problems, please open a git issue and it will be looked at as soon as is convenient.  This is a volunteer effort so operators are not standing by.

## **PID Commands & Behavior**
These commands are generally documented here to ensure HiBean/Artisan compatibility of the esp32 code.  You can issue these commands manually from within HiBean if you enable the developer terminal and you can send the below commands by hand - mostly useful for small experiments or troubleshooting.

The **PID_v1** library is used to regulate heating power based on the measured temperature. It operates in **two modes**:
- **Automatic (PID ON):** Uses PID logic to adjust the heater output.
- **Manual (PID OFF):** Allows manual control of heater power.

## **Available Commands (case-INsensitive)**
| **Command**     | **Description** |
|-----------------|----------------|
| `PID;ON`        | Enables PID control (automatic mode). |
| `PID;OFF`       | Disables PID control (switches to manual mode). |
| `PID;SV;XXX`    | Sets the PID **setpoint temperature** (XXX is in °C, e.g., `PID;SV;250` sets the target to 250°C). |
| `PID;T;PP.P;II.I;DD.D`   |  Apply provided tunings to the PID control (not persisted). |
| `PID;CT;XXXX`    | Temporarily sets PID cycle (sample) time to XXXX ms (not persisted). |
| `PID;PM;E`      | Temporarily change pMode: E = P_ON_E to M = P_ON_M(default), or reverse (not persisted). |
| `OT1;XX`        | Manually sets heater power to **XX%** (only works in MANUAL mode). |
| `READ`          | Retrieves current temperature, set temperature, heater, and vent power. |

## **Other Control Commands**
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

## **Usage Example**
- Enable PID control:
  ```
  PID;ON
  ```
- Set target temperature to 250°C:
  ```
  PID;SV;250
  ```
- Manually set heater to 70% power:
  ```
  OT1;70
  ```
- Read current system status:
  ```
  READ
  ```