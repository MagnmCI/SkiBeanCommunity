// -----------------------------------------------------------------------------
// Timing Constants
// -----------------------------------------------------------------------------
const int PREAMBLE        = 7000;
const int PULSE_ONE       = 1200;
const int PULSE_ZERO      = 650;
const int POST_PULSE_DELAY= 750;
const int START_PULSE     = 7500;
const int START_DELAY     = 3800;

// -----------------------------------------------------------------------------
// Buffer Sizes
// -----------------------------------------------------------------------------
const int ROASTER_LENGTH    = 7;   // 7 bytes received from roaster
const int CONTROLLER_LENGTH = 6;   // 6 bytes sent to roaster

// -----------------------------------------------------------------------------
// Allocate buffers
// -----------------------------------------------------------------------------
uint8_t receiveBuffer[ROASTER_LENGTH];
uint8_t sendBuffer[CONTROLLER_LENGTH];

// -----------------------------------------------------------------------------
// Control Byte Indices
// -----------------------------------------------------------------------------
enum ControlBytes {
    VENT_BYTE = 0,
    DRUM_BYTE = 3,
    COOL_BYTE = 2,
    FILTER_BYTE = 1,
    HEAT_BYTE = 4,
    CHECK_BYTE = 5
};

// Raw temperature values
uint16_t rawTempX, rawTempY;
char CorF            = 'C';           // 'C' or 'F'

// Global temp value
double extern temp;

void pulsePin(int pin, int duration) {
    digitalWrite(pin, LOW);
    delayMicroseconds(duration);
    digitalWrite(pin, HIGH);
}

// Control Bytes & Checksum
void setControlChecksum() {
    uint8_t sum = 0;
    for (int i = 0; i < (CONTROLLER_LENGTH - 1); i++) {
        sum += sendBuffer[i];
    }
    sendBuffer[CHECK_BYTE] = sum;  // Correct use of CHECK_BYTE
}

void setValue(uint8_t* bytePtr, uint8_t value) {
    *bytePtr = value;
    setControlChecksum();
}

void extern sendRoasterMessage() {
    // Start pulse
    pulsePin(TX_PIN, START_PULSE);
    delayMicroseconds(START_DELAY);

    // Send each byte, bit by bit
    for (int i = 0; i < CONTROLLER_LENGTH; i++) {
        for (int j = 0; j < 8; j++) {
            if (bitRead(sendBuffer[i], j) == 1) {
                // '1' bit
                pulsePin(TX_PIN, 1500);
            } else {
                // '0' bit
                pulsePin(TX_PIN, PULSE_ZERO);
            }
            delayMicroseconds(POST_PULSE_DELAY);
        }
    }
}


// -----------------------------------------------------------------------------
// Interrupt to watch for start of roaster message
// https://forum.arduino.cc/t/detecting-pulses-of-certain-lengths-using-interrupts/360570/12 
// -----------------------------------------------------------------------------
unsigned long lastPulse;
volatile bool roasterStartFound = 0;

void watchRoasterStart() {
  unsigned long now = micros();
  
  if ((now - lastPulse) >= PREAMBLE) {
    roasterStartFound = 1;
  } else {
    roasterStartFound = 0;
  }
  lastPulse = now;
}

void getMessage(int bytes, int pin) {
    unsigned long timeIntervals[ROASTER_LENGTH * 8];
    unsigned long pulseDuration = 0;
    int bits = bytes * 8;

    // Read bits
    for (int i = 0; i < bits; i++) {
      timeIntervals[i] = pulseIn(pin, LOW);
    }

    // Clear receiveBuffer
    for (int i = 0; i < bytes; i++) {
      receiveBuffer[i] = 0;
    }

    // Convert intervals into bits
    for (int i = 0; i < bits; i++) {
      if (timeIntervals[i] > PULSE_ONE) {
        receiveBuffer[i / 8] |= (1 << (i % 8));
      }
    }
}

bool calculateRoasterChecksum() {
    uint8_t sum = 0;
    for (int i = 0; i < (ROASTER_LENGTH - 1); i++) {
        sum += receiveBuffer[i];
    }
    return (sum == receiveBuffer[ROASTER_LENGTH - 1]);
}

double calculateTemp() {
    rawTempX = ((receiveBuffer[0] << 8) + receiveBuffer[1]);
    rawTempY = ((receiveBuffer[2] << 8) + receiveBuffer[3]);

    double x = 0.001 * rawTempX;
    double y = 0.001 * rawTempY;
    double v;  // Declare v here

    if (rawTempX > 836 || rawTempY > 221) {
        v = -224.2 * y * y * y + 385.9 * y * y - 327.1 * y + 171;
    } else {
        v = -278.33 * x * x * x + 491.944 * x * x - 451.444 * x + 310.668;
    }

    if (CorF == 'F') {
        v = 1.8 * v + 32.0;
    }
    return v;
}

MedianFilter<double> tempFilter(5);
void filtTemp(double v){
  if(v < 0 && v >= 300) { return; } //don't process blatantly bogus values
  tempFilter.AddValue(v);
  temp = tempFilter.GetFiltered();
}

void extern getRoasterMessage() {
    bool passedChecksum = false;
    getMessage(ROASTER_LENGTH, RX_PIN);

    if ( calculateRoasterChecksum() ) {
      // Valid checksum, compute temperature with filtering
      filtTemp(calculateTemp());
    } else {
      D_println("Not valid roaster message.");
    }
}