/*
 * Copyright (c) 2026 Chris Ice
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// -----------------------------------------------------------------------------
// Pin Definitions for various devices
// -----------------------------------------------------------------------------
#if SERIAL_DEBUG == 1
  const int TX_PIN = NULL;  // bogus pin
  const int RX_PIN = NULL;  // bogus pin
  const int LED_PIN = NULL; // bogus pin
  const String boardID_BLE = String("DEBUG");
#elif defined(ARDUINO_WAVESHARE_ESP32_S3_ZERO)
  const int TX_PIN = 19;  // Output pin to roaster
  const int RX_PIN = 20;  // Input pin from roaster
  const int LED_PIN = WS_RGB; // WaveShare S3 on-board LED
  const String boardID_BLE = String("ARDUINO_WAVESHARE_ESP32_S3_ZERO");
#elif defined(ARDUINO_ESP32C6_DEV)
  const int TX_PIN = 11;  // Output pin to roaster - use any free GPIO
  const int RX_PIN = 10;  // Input pin from roaster - use any free GPIO
  const int LED_PIN = PIN_RGB_LED; // rgb values in a different order for this board - not fixed
  const String boardID_BLE = String("ARDUINO_ESP32C6_DEV");
#else
  const int TX_PIN = 1;  // bogus pin
  const int RX_PIN = 2;  // bogus pin
  const int LED_PIN = 0; // bogus pin
  const String boardID_BLE = String("UNKNOWN");
#endif
