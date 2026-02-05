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

 /* Replaces Classic Bluetooth with NimBLE (NUS).
 *
 * Service UUID:
 *     6e400001-b5a3-f393-e0a9-e50e24dcca9e
 * Characteristics UUIDs:
 *   - Write Characteristic (RX):
 *     6e400002-b5a3-f393-e0a9-e50e24dcca9e
 *   - Notify Characteristic (TX):
 *     6e400003-b5a3-f393-e0a9-e50e24dcca9e
 *
 * Sends notifications for temperature/status data.
 * Expects commands via the write characteristic.*/

#include <NimBLEDevice.h>
#include <queue>    // for std::queue
#include <string>   // for std::string
#include "SkiPIDConfig.h"

// -----------------------------------------------------------------------------
// NimBLE UUIDs for Hibean roaster Control writes and notifies
// -----------------------------------------------------------------------------
#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e" // NUS service
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e" // Write
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e" // Notify

// -----------------------------------------------------------------------------
// NimBLE UUIDs for PID Config
// -----------------------------------------------------------------------------
#define PID_TUNE          "6dbf0201-758d-4b5e-bc11-40cfaea42dfe" // ppp.pp,iii.ii,ddd.dd
#define PID_MODE          "6dbf0202-758d-4b5e-bc11-40cfaea42dfe" // "P_ON_M" | "P_ON_E"
#define PID_SAMPLE_TIME   "6dbf0203-758d-4b5e-bc11-40cfaea42dfe" // iiii (ms)
#define PID_MAX_POWER     "6dbf0204-758d-4b5e-bc11-40cfaea42dfe" // 0-100 (%)

// -----------------------------------------------------------------------------
// NimBLE Globals
// -----------------------------------------------------------------------------
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;

bool deviceConnected = false;
extern String firmWareVersion;
extern String sketchName;
extern std::queue<String> messageQueue;
extern PID myPID;
extern PIDConfig myPIDConfig;

// -----------------------------------------------------------------------------
// NimBLE Server Callbacks
// -----------------------------------------------------------------------------
class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    deviceConnected = true;

    // Change NimBLE connection parameters per apple NimBLE guidelines
    // (for this client, min interval 15ms (/1.25), max 30ms (/1.25), latency 4 frames, timeout 5sec(/10ms)
    // https://docs.silabs.com/bluetooth/4.0/bluetooth-miscellaneous-mobile/selecting-suitaNimBLE-connection-parameters-for-apple-devices
    pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 4, 500);
   
    D_println("NimBLE: Client connected.");
  }
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    deviceConnected = false;
    D_println("NimBLE: Client disconnected. Restarting advertising...");
    pServer->getAdvertising()->start();
  }
};

// -----------------------------------------------------------------------------
// NimBLE Characteristic Callbacks
// -----------------------------------------------------------------------------
class RoasterCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    String rxValue = String(pCharacteristic->getValue().c_str());
    rxValue.remove(rxValue.lastIndexOf("\n")); //remove trailing newlines

    if (rxValue.length() > 0) {
      String input = String(rxValue.c_str());
      D_print("NimBLE Write Received: ");  D_println(input);
      messageQueue.push(rxValue);    }
  }
};

class PIDTuneCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    String rxValue = String(pCharacteristic->getValue().c_str());
    
    double pidTune[3]; //pp.p;ii.i;dd.d
    int paramCount = 0;
    while(rxValue.length() > 0) {
        int index = rxValue.indexOf(',');
        if (index == -1) { // No delim found
            pidTune[paramCount++] = rxValue.toDouble(); //remaining string
            break;
        } else {
            pidTune[paramCount++] = rxValue.substring(0, index).toDouble(); //grab first
            rxValue = rxValue.substring(index+1); //trim string
        }
      }
    myPIDConfig.setKp(pidTune[0]); myPIDConfig.setKi(pidTune[1]); myPIDConfig.setKd(pidTune[2]);
    myPIDConfig.apply(myPID);
  }
  void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
      D_println("PIDTuneRead Received.");
      pCharacteristic->setValue(String(myPIDConfig.getKp()) + ',' + String(myPIDConfig.getKi()) + ',' + String(myPIDConfig.getKd()));
  }
};

class PIDModeCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    String rxValue = String(pCharacteristic->getValue().c_str());
    
    if(rxValue == "P_ON_E") {
      myPIDConfig.setPMode(P_ON_E);
    } else {
      myPIDConfig.setPMode(P_ON_M);
    }
    myPIDConfig.apply(myPID);
  }
  void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
      D_println("PMode Received.");
      if (myPIDConfig.getPMode() == P_ON_E) {
        pCharacteristic->setValue("P_ON_E");
      } else {
        pCharacteristic->setValue("P_ON_M");
      }
  }
};

class PIDSampleTimeCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    String rxValue = String(pCharacteristic->getValue().c_str()); 
    myPIDConfig.setSampleTime(rxValue.toInt());
    myPIDConfig.apply(myPID);
  }
  void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
      D_println("SampleTime Received.");
      pCharacteristic->setValue(String(myPIDConfig.getSampleTime()));
  }
};

class PIDMaxPowerCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    String rxValue = String(pCharacteristic->getValue().c_str()); 
    myPIDConfig.setMaxPower(rxValue.toInt());
    myPIDConfig.apply(myPID);
  }
  void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
      D_println("MaxPower Received.");
      pCharacteristic->setValue(String(myPIDConfig.getMaxPower()));
  }
};

// HiBean notify response to write()
void notifyNimBLEClient(const String& message) {
    D_println("Attempting to notify NimBLE client with: " + message);

    if (deviceConnected && pTxCharacteristic) {
        pTxCharacteristic->setValue(message.c_str());
        pTxCharacteristic->notify();
       D_println("Notification sent successfully.");
    } else {
      D_println("Notification failed. Device not connected or TX characteristic unavailaNimBLE.");
    }
}

void extern initBLE() {
    NimBLEDevice::init("ESP32_Skycommand_NimBLE");

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    // Roaster notifes to HiBean
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );

    // Hibean commands to Roaster
    NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new RoasterCallbacks());

    // PID_TUNE handler
    NimBLECharacteristic* pidTuneCharacteristic = pService->createCharacteristic(
        PID_TUNE, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    pidTuneCharacteristic->setCallbacks(new PIDTuneCallback());
    BLEDescriptor* pidTuneDescriptor = pidTuneCharacteristic->createDescriptor(PID_TUNE, NIMBLE_PROPERTY::READ);
    pidTuneDescriptor->setValue("PID Tune: p.pp,i.ii,d.dd");
    pidTuneCharacteristic->addDescriptor(pidTuneDescriptor);

    // PID_MODE handler
    NimBLECharacteristic* pidModeCharacteristic = pService->createCharacteristic(
        PID_MODE, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    pidModeCharacteristic->setCallbacks(new PIDModeCallback());
    BLEDescriptor* pidModeDescriptor = pidModeCharacteristic->createDescriptor(PID_MODE, NIMBLE_PROPERTY::READ);
    pidModeDescriptor->setValue("PID Mode: P_ON_M | P_ON_E");
    pidModeCharacteristic->addDescriptor(pidModeDescriptor);

    // PID_SAMPLE_TIME handler
    NimBLECharacteristic* pidSampletimeCharacteristic = pService->createCharacteristic(
        PID_SAMPLE_TIME, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    pidSampletimeCharacteristic->setCallbacks(new PIDSampleTimeCallback());
    BLEDescriptor* pidSampleTimeDescriptor = pidSampletimeCharacteristic->createDescriptor(PID_SAMPLE_TIME, NIMBLE_PROPERTY::READ);
    pidSampleTimeDescriptor->setValue("PID Sample Time: iiii (ms)");
    pidSampletimeCharacteristic->addDescriptor(pidSampleTimeDescriptor);

    // PID_MAX_POWER handler
    NimBLECharacteristic* pidMaxPowerCharacteristic = pService->createCharacteristic(
        PID_MAX_POWER, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    pidMaxPowerCharacteristic->setCallbacks(new PIDMaxPowerCallback());
    NimBLEDescriptor* pidMaxPowerDescriptor = pidMaxPowerCharacteristic->createDescriptor(PID_MAX_POWER, NIMBLE_PROPERTY::READ);
    pidMaxPowerDescriptor->setValue("PID Max Power: 0-100 (%)");
    pidMaxPowerCharacteristic->addDescriptor(pidMaxPowerDescriptor);

    pService->start();

    // esp32 information to HiBean for support/debug purposes
    NimBLEService* devInfoService = pServer->createService("180A");
    NimBLECharacteristic* boardCharacteristic = devInfoService->createCharacteristic("2A29", NIMBLE_PROPERTY::READ);
      boardCharacteristic->setValue(boardID_BLE);
    NimBLECharacteristic* sketchNameCharacteristic = devInfoService->createCharacteristic("2A28", NIMBLE_PROPERTY::READ);
      sketchNameCharacteristic->setValue(sketchName);
    NimBLECharacteristic* firmwareCharacteristic = devInfoService->createCharacteristic("2A26", NIMBLE_PROPERTY::READ);
      firmwareCharacteristic->setValue(sketchName + " " + firmWareVersion);
    
    devInfoService->start();

    NimBLEAdvertising* pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
    
	  D_println("NimBLE Advertising started...");
}
