#include <Arduino.h>
#include <HardwareSerial.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLE2902.h>

#define SERVICE_UUID "ab0828b1-198e-4351-b779-901fa0e0371e"
#define CHARACTERISTIC_UUID_RX "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX "0972EF8C-7613-4075-AD52-756F33D4DA91"

HardwareSerial Log(1);
HardwareSerial Messenger(2);

NimBLEServer *myServer = NULL;
BLECharacteristic *characteristicTX = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

void setupBLE();
void loop();

//callback para eventos das características
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
     void onWrite(BLECharacteristic *characteristic) {
        // read value sent through BLE
        String rxValue = characteristic->getValue().c_str(); 
        // verify that the value is more than 0
        if (rxValue.length() > 0) {
            Serial.println(rxValue);
        }
    }//onWrite
};

void connectedTask(void *param) {
    for(;;) {
        if (deviceConnected) {
            // do somthing when device is connected
            if(Messenger.available()) {
                String message = Messenger.readStringUntil('\n');
                Log.println(message);
            }
        } 
        
        // conncetion management
        if (!deviceConnected && oldDeviceConnected) {
            myServer->startAdvertising(); // restart advertising
            printf("start advertising\n");
            oldDeviceConnected = deviceConnected;
        }
        
        if (deviceConnected && !oldDeviceConnected) {
            oldDeviceConnected = deviceConnected;
        }
        vTaskDelay(250); // Delay between loops to reset watchdog timer portMAX_DELAY
    }
    vTaskDelete(NULL);
}

/**
 * This program is used to be a bridge to communicate the main arduino nano
 * with the Android Gateway App 
 **/
extern "C" void app_main()
{
    Log.begin(115200);
    Messenger.begin(115200);
    setupBLE();
    Log.println("Setup done");
    while(1) loop();
}

void loop() {
    if (deviceConnected) {
        String str = "";
        const char *txString = str.c_str();
        characteristicTX->setValue(txString);
        characteristicTX->notify();
    }
    delay(1000);
}

void setupBLE() {
    // Create the BLE Device
    NimBLEDevice::init("Waterbox");
 
    // Create the NimBLE Server
    myServer = NimBLEDevice::createServer();
    myServer->setCallbacks(new BLEServerCallbacks());
 
    // Create the NimBLE Service
    NimBLEService *myService = myServer->createService(SERVICE_UUID);
 
    // Create a NimBLE Characteristic
    characteristicTX = myService->createCharacteristic(
                       CHARACTERISTIC_UUID_TX,
                       NIMBLE_PROPERTY::NOTIFY
                     );
 
    // characteristicTX->addDescriptor(new NimBLE2902());

    BLECharacteristic *characteristicRX = myService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        NIMBLE_PROPERTY::WRITE
                    );
 
    characteristicRX->setCallbacks(new CharacteristicCallbacks());
 
    // Start the service
    myService->start();

    xTaskCreate(connectedTask, "connectedTask", 5000, NULL, 1, NULL);

    NimBLEAdvertising *myAdvertising = NimBLEDevice::getAdvertising();
    myAdvertising->addServiceUUID(myService->getUUID());
    myAdvertising->setScanResponse(true);
    myAdvertising->start();
}

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
    };
 
    void onDisconnect(NimBLEServer* pServer) {
         deviceConnected = false;
    }
};