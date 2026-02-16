/*
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
// LED handler
// -----------------------------------------------------------------------------

enum LedColor {
  LED_BLUE,
  LED_RED,
  LED_GREEN,
  LED_BLACK
};

unsigned long LED_LAST_ON_MS;
unsigned long LED_FLASH_DELAY_MS = 1000;
unsigned int currentLEDColor;

void setLedColor(LedColor color) {
  uint8_t r = 0, g = 0, b = 0;

  switch (color) {
    case LED_BLUE:  r = 0;  g = 0;  b = 10; break;
    case LED_RED:   r = 10; g = 0;  b = 0;  break;
    case LED_GREEN: r = 0;  g = 10; b = 0;  break;
    case LED_BLACK: r = 0;  g = 0;  b = 0;  break;
  }
  
  rgbLedWrite(LED_BUILTIN, g, r, b);
}

void handleLED() {
  unsigned long t_now = millis();

  if ((t_now - LED_LAST_ON_MS) >= LED_FLASH_DELAY_MS) {
    if (currentLEDColor == LED_BLUE && deviceConnected) {
      setLedColor(LED_BLACK);
      currentLEDColor = LED_BLACK;
    } 
    else if (currentLEDColor == LED_BLUE) {
      setLedColor(LED_RED);
      currentLEDColor = LED_RED;
      LED_LAST_ON_MS = t_now;
    } 
    else {
      setLedColor(LED_BLUE);
      currentLEDColor = LED_BLUE;
      LED_LAST_ON_MS = t_now;
    }
  }
}
