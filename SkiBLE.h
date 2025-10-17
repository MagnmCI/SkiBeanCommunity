// -----------------------------------------------------------------------------
// All BLE related functions
// -----------------------------------------------------------------------------

/* Replaces Classic Bluetooth with BLE (NUS).
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

// -----------------------------------------------------------------------------
// BLE UUIDs for Nordic UART Service
// -----------------------------------------------------------------------------
#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e" // NUS service
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e" // Write
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e" // Notify

// -----------------------------------------------------------------------------
// BLE Globals
// -----------------------------------------------------------------------------
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;
bool deviceConnected = false;
extern String firmWareVersion;
extern String sketchName;

// -----------------------------------------------------------------------------
// Forward Declarations
// -----------------------------------------------------------------------------
void extern parseAndExecuteCommands(String input);
void extern notifyBLEClient(const String& message);

// -----------------------------------------------------------------------------
// BLE Server Callbacks
// -----------------------------------------------------------------------------
class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    deviceConnected = true;

    // Change BLE connection parameters per apple ble guidelines
    // (for this client, min interval 15ms (/1.25), max 30ms (/1.25), latency 4 frames, timeout 5sec(/10ms)
    // https://docs.silabs.com/bluetooth/4.0/bluetooth-miscellaneous-mobile/selecting-suitable-connection-parameters-for-apple-devices
    pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 4, 500);
   
    D_println("BLE: Client connected.");
  }
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    deviceConnected = false;
    D_println("BLE: Client disconnected. Restarting advertising...");
    pServer->getAdvertising()->start();
  }
} serverCallbacks;

// -----------------------------------------------------------------------------
// BLE Characteristic Callbacks
// -----------------------------------------------------------------------------
class MyCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    String rxValue = pCharacteristic->getValue().c_str();
    D_print("BLE Write Received: ");
    D_println(rxValue);
    if (rxValue.length() > 0) {
      parseAndExecuteCommands(rxValue);
    }
  }
} chrCallbacks;

void notifyBLEClient(const String& message) {
    D_println("Attempting to notify BLE client with: " + message);
    if (deviceConnected && pTxCharacteristic) {
        pTxCharacteristic->setValue(message.c_str());
        pTxCharacteristic->notify();
       D_println("Notification sent successfully.");
    } else {
      D_println("Notification failed. Device not connected or TX characteristic unavailable.");
    }
}

void extern initBLE() {
    NimBLEDevice::init("ESP32_Skycommand_BLE");

    //create a ble server, attach its callbacks
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    //create a ble service on that server
    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    // Tx characteristic: Roaster notifes to HiBean
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );

    // Rx Characteristic: Hibean commands to Roaster; notify required by Hibean, don't know why
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY
    );

    //attach event handler callback for Rx
    pRxCharacteristic->setCallbacks(&chrCallbacks);

    //start the HIBean rx/tx service
    pService->start();

    // Another service to send build information to HiBean for support purposes
    NimBLEService* devInfoService = pServer->createService("180A");
    NimBLECharacteristic* boardCharacteristic = devInfoService->createCharacteristic("2A29", NIMBLE_PROPERTY::READ);
      boardCharacteristic->setValue(boardID_BLE);

    NimBLECharacteristic* sketchNameCharacteristic = devInfoService->createCharacteristic("2A28", NIMBLE_PROPERTY::READ);
      sketchNameCharacteristic->setValue(sketchName);

    NimBLECharacteristic* firmwareCharacteristic = devInfoService->createCharacteristic("2A26", NIMBLE_PROPERTY::READ);
      firmwareCharacteristic->setValue(sketchName + " " + firmWareVersion);
    
    devInfoService->start();

    //set up ble advertising of these services
    NimBLEAdvertising* pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->addServiceUUID(devInfoService->getUUID());
    pAdvertising->start();
    
    D_println("BLE Advertising started...");
}