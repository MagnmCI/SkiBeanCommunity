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
// Message parser for messages FROM roaster (bean temperature)
// -----------------------------------------------------------------------------

extern char CorF;

class SkyRoasterParser {
public:

    void begin(uint8_t pin);
    bool msgAvailable();
    void getMessage(uint8_t *dest);
    bool validate(const uint8_t *buf);

    // --- Structured Fields ---
    double getTemperature(uint8_t *buf); // in °C units

private:
    static void IRAM_ATTR edgeISR();
    void handleEdge();

    inline void resetRx() {
        rxState     = IDLE;
        bitCount    = 0;
        byteIndex   = 0;
        currentByte = 0;
    }
    
    int pin;

    // Protocol constants
    static const uint8_t  MSG_BYTES     = 7;
    static const uint8_t  BITS_PER_BYTE = 8;
    static const unsigned long START_MIN_US = 7000;
    static const unsigned long START_MAX_US = 10000;
    static const unsigned long BIT0_MAX_US  = 900;
    static const unsigned long BIT1_MIN_US  = 1200;
    static const unsigned long BIT1_MAX_US  = 2000;
    
    static const unsigned long FRAME_TIMEOUT_US = 20000;
    static const unsigned long IDLE_GAP_US      = 3000;

    // State
    enum RxState { IDLE, RECEIVING };
    volatile RxState rxState = IDLE;

    volatile unsigned long lastEdgeTime = 0;
    volatile bool lastEdgeWasLow = false;

    uint8_t bitCount = 0;
    uint8_t byteIndex = 0;
    volatile uint8_t currentByte = 0;
    volatile uint8_t messageBuf[MSG_BYTES];
    volatile bool newMessage = false;

    static SkyRoasterParser *instance;
};

SkyRoasterParser* SkyRoasterParser::instance = nullptr;

void SkyRoasterParser::begin(uint8_t pin) {
    instance = this;
    rxState = IDLE;
    this->pin = pin;
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), SkyRoasterParser::edgeISR, CHANGE);
}

bool SkyRoasterParser::msgAvailable() {
    return newMessage;
}

void SkyRoasterParser::getMessage(uint8_t *dest) {
    noInterrupts();
    for (uint8_t i = 0; i < MSG_BYTES; i++) dest[i] = messageBuf[i];
    newMessage = false;
    interrupts();

    if (!validate(dest)) {
        ESP_LOGV("SkiParser", "Checksum failed, forcing resync");
        noInterrupts();
        resetRx();
        interrupts();
    }
}

bool SkyRoasterParser::validate(const uint8_t *buf) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < MSG_BYTES - 1; i++) sum += buf[i];
    return (sum == buf[MSG_BYTES - 1]);
}

double SkyRoasterParser::getTemperature(uint8_t *buf) {
    // Combine the first 4 bytes into a 16-bit integer (Little Endian)
    uint16_t rawTempX = ((buf[0] << 8) + buf[1]);
    uint16_t rawTempY = ((buf[2] << 8) + buf[3]);

    double x = 0.001 * rawTempX;
    double y = 0.001 * rawTempY;
    double temperature;

    if (rawTempX > 836 || rawTempY > 221) {
        temperature = -224.2 * y * y * y + 385.9 * y * y - 327.1 * y + 171;
    } else {
        temperature = -278.33 * x * x * x + 491.944 * x * x - 451.444 * x + 310.668;
    }

    if (CorF == 'F') {
        temperature = 1.8 * temperature + 32.0;
    }
    return temperature;
}

// --- Static ISR trampoline ---
void IRAM_ATTR SkyRoasterParser::edgeISR() {
    if (instance) instance->handleEdge();
}

// --- Edge handler ---
void SkyRoasterParser::handleEdge() {
    unsigned long now = micros();
    bool pinIsLow = (digitalRead(this->pin) == LOW);

    // Timeout protection (mid-frame stall)
    if (rxState == RECEIVING && (now - lastEdgeTime) > FRAME_TIMEOUT_US) {
        ESP_LOGV("SkiParser", "Frame timeout, resync");
        resetRx();
    }

    if (pinIsLow) {
        lastEdgeTime = now;
        lastEdgeWasLow = true;
        return;
    }

    // Rising edge
    if (!lastEdgeWasLow) return;
    lastEdgeWasLow = false;

    unsigned long lowDur = now - lastEdgeTime;

    ESP_LOGV("SkiParser", "Low pulse: %ld", lowDur);
    
    switch (rxState) {

    case IDLE:
        // Require clean idle gap before start
        if (lowDur >= START_MIN_US &&
            lowDur <= START_MAX_US) {

            resetRx();
            rxState = RECEIVING;

            ESP_LOGV("SkiParser", "Start detected");
        }
        break;

    case RECEIVING: {
        uint8_t bitVal;

        if (lowDur <= BIT0_MAX_US) {
            bitVal = 0;
        } else if (lowDur >= BIT1_MIN_US && lowDur <= BIT1_MAX_US) {
            bitVal = 1;
        } else {
            ESP_LOGV("SkiParser", "Invalid pulse, abort");
            resetRx();
            return;
        }

        currentByte |= (bitVal << bitCount);
        bitCount++;

        ESP_LOGV("SkiParser", "BitVal: %i", bitVal);

        if (bitCount >= BITS_PER_BYTE) {
            if (byteIndex >= MSG_BYTES) {
                ESP_LOGV("SkiParser", "Byte overflow, resync");
                resetRx();
                return;
            }

            messageBuf[byteIndex++] = currentByte;
            currentByte = 0;
            bitCount = 0;

            if (byteIndex >= MSG_BYTES) {
                newMessage = true;
                resetRx();
                ESP_LOGV("SkiParser", "Message complete");
            }
        }
        break;
        }
    }
}
