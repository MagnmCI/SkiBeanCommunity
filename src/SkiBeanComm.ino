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

/***************************************************
 * HiBean ESP32 BLE Roaster Control
 ***************************************************/

#include <Arduino.h>
#include <PID_v1.h>
#include "../lib/SkiBLE.h"
#include "../lib/SkiLED.h"
#include "../lib/SkiCMD.h"
#include "../lib/SkiPIDConfig.h"
#include "../lib/SkiParser.h"

// -----------------------------------------------------------------------------
// Current Sketch and Release Version (for BLE device info)
// -----------------------------------------------------------------------------
#define FW_VERSION "v1.2.4"
String firmWareVersion = String(FW_VERSION);
String sketchName = String(__FILE__).substring(String(__FILE__).lastIndexOf('/')+1);

// -----------------------------------------------------------------------------
// Global Bean Temperature Variable
// -----------------------------------------------------------------------------
double temp          = 0.0; // temperature
char CorF = 'C';            // default units

// -----------------------------------------------------------------------------
// Instantiate Parser for read messages from roaster
// -----------------------------------------------------------------------------
SkyRoasterParser roaster;

// -----------------------------------------------------------------------------
// Track BLE writes from HiBean
// -----------------------------------------------------------------------------
std::queue<String> messageQueue;  // Holds commands written by Hibean to us

// -----------------------------------------------------------------------------
// Setup PID and Config interface
// -----------------------------------------------------------------------------
double pInput, pOutput;
double pSetpoint = 0.0; // Desired temperature (adjustable on the fly)
int manualHeatLevel = 50;

PIDConfig myPIDConfig;
PID myPID(&pInput, &pOutput, &pSetpoint,
        myPIDConfig.getKp(), myPIDConfig.getKi(), myPIDConfig.getKd(),
        myPIDConfig.getPMode(), DIRECT);  //pid instance with our default values

void setup() {
    esp_log_level_set("*", ESP_LOG_INFO);
    Serial.begin(115200);
    ESP_LOGI("setup","Starting HiBean ESP32 BLE Roaster Control.");
    delay(3000); //let fw upload finish before we take over hwcdc serial tx/rx

    ESP_LOGI("CT1780","search CT1780...");
    while(!CT1780.searchDevice(/*newAddr=*/sensorCt1780.uniqueAddr)) {
        ESP_LOGE("CT1780","Fail! It could be the following: the bus is short-circuited, there are no devices, you have retrieved all the devices, or you have provided the wrong parameters!");
        delay(1000);
    }
    ESP_LOGI("CT1780","Succeed!");

    // set pinmode on tx for commands to roaster, take it high
    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, HIGH);

    // start parser on rx pin for bean temp readings from roaster
    roaster.begin(RX_PIN);
    roaster.enableDebug(false);

    // Start BLE
    initBLE();

    // Set PID to start in MANUAL mode
    myPID.SetMode(MANUAL);

    // clamp output limits to 0-100(% heat), set sample interval 
    myPID.SetOutputLimits(0.0,myPIDConfig.getMaxPower());
    myPID.SetSampleTime(myPIDConfig.getSampleTime());

    // Ensure heat starts at 0% for safety
    manualHeatLevel = 0;
    handleHEAT(manualHeatLevel);

    shutdown();
}

void loop() {
    // roaster shut down, clear our buffers   
    if (itsbeentoolong()) { shutdown(); }

    // roaster message found, go get it, validate and update temp
    if(roaster.msgAvailable()) {
        uint8_t msg[7];
        roaster.getMessage(msg);

        if(roaster.validate(msg)) {
            temp = roaster.getTemperature(msg);
        } else {
            ESP_LOGE("loop", "Roaster message checksum failed!");
        }
    }

    // process incoming ble commands from HiBean, could be read or write
    while (!messageQueue.empty()) {
        String msg = messageQueue.front(); //grab the first one
        messageQueue.pop(); //remove it from the queue
        parseAndExecuteCommands(msg);  // process the command it
    }

    // Ensure PID or manual heat control is handled
    handlePIDControl();
    
    // update the led so user knows we're running
    handleLED();
}
