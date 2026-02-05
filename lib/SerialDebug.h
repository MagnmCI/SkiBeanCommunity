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
// Set SERIAL_DEBUG to 1 to enable serial output
// Trying to serial.print AND do roaster rx/tx is not compatible with s3-zero single usb
// -----------------------------------------------------------------------------
#define SERIAL_DEBUG 0 //set to 1 to turn on

// -----------------------------------------------------------------------------
// Do not change these for debugging unless you know what you're doing
// -----------------------------------------------------------------------------
#if SERIAL_DEBUG == 1
#define D_print(...)    Serial.print(__VA_ARGS__)
#define D_println(...)  Serial.println(__VA_ARGS__)
#else
#define D_print(...)
#define D_println(...)
#endif
