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

  // TODO: this macro does not work, figure out why
  #if RGB_BUILTIN_LED_COLOR_ORDER == LED_COLOR_ORDER_GRB
  switch (color) {
    case LED_BLUE:  r = 0;  g = 0;  b = 10; break;
    case LED_RED:   r = 0; g = 10;  b = 0;  break;
    case LED_GREEN: r = 10;  g = 0; b = 0;  break;
    case LED_BLACK: r = 0;  g = 0;  b = 0;  break;
  }
  #else
  switch (color) {
    case LED_BLUE:  r = 0;  g = 0;  b = 10; break;
    case LED_RED:   r = 10; g = 0;  b = 0;  break;
    case LED_GREEN: r = 0;  g = 10; b = 0;  break;
    case LED_BLACK: r = 0;  g = 0;  b = 0;  break;
  }
  #endif

  rgbLedWrite(LED_PIN, r, g, b);
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