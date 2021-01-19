
/**
 * ESP32: UART-BLE Bridge
 * This is a simple project which uses and ESP32 to act as a bridge to communicate messages received from a UART sender towards a BLE Client receiver, and vice versa. 
 * 
 * Communication Diagram:
 * Arduino -- (UART) -- ESP32 -- (BLE) -- Android
 **/

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
HardwareSerial ArduinoSerial(2);

NimBLEServer *myServer = NULL;
BLECharacteristic *characteristicTX = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

void setupBLE();
void loop();
void sendBLE(String);
void sendUART(String);

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Log.println("device connected.");
    };
 
    void onDisconnect(NimBLEServer* pServer) {
         deviceConnected = false;
        Log.println("device disconnected.");
    }
};

// This callback is called whenever the Android sends a message through the CHARACTERISTIC_UUID_RX
class CharacteristicRXCallback: public BLECharacteristicCallbacks {
     void onWrite(BLECharacteristic *characteristic) {
        // read value sent through BLE by Android
        String rxValue = characteristic->getValue().c_str(); 
        // verify that the value is more than 0
        if (rxValue.length() > 0) {
            // print received messages from Android App into serial and to the Arduino
            Log.println(rxValue);
            sendUART(rxValue);
        }
    }//onWrite
};

void connectedTask(void *param) {
    for(;;) {
        if (deviceConnected) {
            // do somthing when device is connected          
            if(ArduinoSerial.available()) {
                // listen to any messages coming in from Arduino which is connected to the ESP32 through UART. Print received messages to Log and BLE Client
                String message = ArduinoSerial.readStringUntil('\n');
                Log.println(message);
                sendBLE(message);
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

extern "C" void app_main()
{
    Log.begin(115200);
    ArduinoSerial.begin(115200);
    
    setupBLE();
    Log.println("Setup done");

    while(1) loop();
}

void loop() {
    if (deviceConnected) {
        if (Serial.available()) {
            // if user write anything in Serial Monitor, echo it to BLE Client
            String message = Serial.readStringUntil('\n');
            Log.println(message);
            sendBLE(message);
        }
    }
    delay(1000);
}

void setupBLE() {
    // Create the BLE Device
    NimBLEDevice::init("Waterbox");
 
    // Create the NimBLE Server, set Callbacks, create & set Service
    myServer = NimBLEDevice::createServer();
    myServer->setCallbacks(new ServerCallbacks());
    NimBLEService *myService = myServer->createService(SERVICE_UUID);
 
    // Create characteristicTX to send messages to BLE Client (Android App)
    // Create characteristicRX to receive messages from BLE Client (Android App)
    characteristicTX = myService->createCharacteristic(
                       CHARACTERISTIC_UUID_TX,
                       NIMBLE_PROPERTY::NOTIFY
                     );
    BLECharacteristic *characteristicRX = myService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        NIMBLE_PROPERTY::WRITE
                    );
    // characteristicTX->addDescriptor(new NimBLE2902());
 
    characteristicRX->setCallbacks(new CharacteristicRXCallback()); // callbacks enable one to react of ble events such as characteristic write
 
    // Start the service
    myService->start();

    xTaskCreate(connectedTask, "connectedTask", 5000, NULL, 1, NULL);

    NimBLEAdvertising *myAdvertising = NimBLEDevice::getAdvertising();
    myAdvertising->addServiceUUID(myService->getUUID());
    myAdvertising->setScanResponse(true);
    myAdvertising->start();
}

void sendUART(String msg) {
    ArduinoSerial.println(msg);
}

void sendBLE(String msg) {
    const char *msgchar = msg.c_str();
    characteristicTX->setValue(msgchar);
    characteristicTX->notify();
}