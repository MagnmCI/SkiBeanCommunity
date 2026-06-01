#ifndef CT1780_ASYNC_H
#define CT1780_ASYNC_H

#include <Arduino.h>
#include <OneWire.h>

class CT1780Async {
private:
    OneWire _wire;
    unsigned long _lastConversionTime;
    bool _isConverting;
    float _currentTemperature;
    const unsigned long _conversionInterval = 750; // Hardware digitizing time in ms

    // Low-level 1-wire Dallas CRC8 lookup method natively embedded
    uint8_t crc8(const uint8_t *addr, uint8_t len) {
        uint8_t crc = 0;
        while (len--) {
            uint8_t inbyte = *addr++;
            for (uint8_t i = 8; i; i--) {
                uint8_t mix = (crc ^ inbyte) & 0x01;
                crc >>= 1;
                if (mix) crc ^= 0x8C;
                inbyte >>= 1;
            }
        }
        return crc;
    }

    void triggerConversionSingle() {
        _wire.reset();
        _wire.write(0xCC); // SKIP ROM: Instantly broadcast to single sensor
        _wire.write(0x44); // CMD_CONVERT_T
    }

    float readScratchpadSingle() {
        uint8_t scratchpad[9];
        
        _wire.reset();
        _wire.write(0xCC); // SKIP ROM
        _wire.write(0xBE); // CMD_READ_SCRATCHPAD

        for (int i = 0; i < 9; i++) {
            scratchpad[i] = _wire.read();
        }

        // Validate raw bus packet integrity
        if (crc8(scratchpad, 8) != scratchpad[8]) {
            return NAN; 
        }

        // Fast payload register bit reconstruction
        int16_t rawTemperature = (scratchpad[1] << 8) | scratchpad[0];
        rawTemperature >>= 2;
        if (rawTemperature & 0x2000) {
            rawTemperature |= 0xC000;
        }
        
        float temp = rawTemperature * 0.25f; // 0.25°C resolution matching MAX31850 engine
        if (temp > 1372.0f || temp < -270.0f) {
            return NAN;
        }
        return temp;
    }

public:
    // Constructor maps directly to physical micro IO pins
    CT1780Async(uint8_t pin) : _wire(pin), _lastConversionTime(0), _isConverting(false), _currentTemperature(NAN) {}

    // Non-blocking tick function. Call this every iteration of loop()
    void update() {
        unsigned long now = millis();

        if (!_isConverting) {
            triggerConversionSingle();
            _lastConversionTime = now;
            _isConverting = true;
        } 
        else if (now - _lastConversionTime >= _conversionInterval) {
            float freshReading = readScratchpadSingle();
            if (!isnan(freshReading)) {
                _currentTemperature = freshReading;
            }
            _isConverting = false; // Flag ready for next asynchronous iteration
        }
    }

    // Instantly returns the most recent clean temperature cache (Zero overhead)
    float getTemperature() const {
        return _currentTemperature;
    }
};

#endif