#include <Arduino.h>

void connectedTask(void *);
void setupBLE();
void sendMsgUART(String);

// Arduino -----------------------------

#include <HardwareSerial.h>
HardwareSerial ArduinoSerial(2);

void sendMsgUART(String msg) {
    ArduinoSerial.println(msg);
}

// BLE ---------------------------------

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

#define SERVICE_UUID "ab0828b1-198e-4351-b779-901fa0e0371e"
#define CHARACTERISTIC_UUID_RX "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX "0972EF8C-7613-4075-AD52-756F33D4DA91"

NimBLEServer *nimbleServer = NULL;
BLECharacteristic *characteristicTX = NULL;
bool isBleConnected = false;
bool isBleConnectedBefore = false;

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        isBleConnected = true;
        Serial.println("device connected.");
    };
 
    void onDisconnect(NimBLEServer* pServer) {
         isBleConnected = false;
        Serial.println("device disconnected.");
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
            Serial.println(rxValue);
            sendMsgUART(rxValue);
        }
    }
};

void connectedTask(void *param) {
    for(;;) {
        if (isBleConnected) {

            // do somthing when device is connected          
            if(ArduinoSerial.available()) {
          
                // listen to any messages coming in from Arduino which is connected to the ESP32 through UART. Print received messages to Serial and BLE Client
                String message = ArduinoSerial.readStringUntil('\n');
                Serial.println(message);
            }
        }
        
        // connection management
        if (!isBleConnected && isBleConnectedBefore) {
            nimbleServer->startAdvertising(); // restart advertising
            printf("start advertising\n");
            isBleConnectedBefore = isBleConnected;
        }

        if (isBleConnected && !isBleConnectedBefore) {
            isBleConnectedBefore = isBleConnected;
        }
        vTaskDelay(250); // Delay between loops to reset watchdog timer portMAX_DELAY
    }
    vTaskDelete(NULL);
}

void setupBLE() {
    // Create the BLE Device
    NimBLEDevice::init("ESP32 as BLE UART");
 
    // Create the NimBLE Server, set Callbacks, create & set Service
    nimbleServer = NimBLEDevice::createServer();
    nimbleServer->setCallbacks(new ServerCallbacks());
    NimBLEService *myService = nimbleServer->createService(SERVICE_UUID);
 
    // Create characteristicTX to send messages to BLE Client (Android App)
    characteristicTX = myService->createCharacteristic(
                       CHARACTERISTIC_UUID_TX,
                       NIMBLE_PROPERTY::NOTIFY
                    );

    // Create characteristicRX to receive messages from BLE Client (Android App)
    BLECharacteristic *characteristicRX = myService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        NIMBLE_PROPERTY::WRITE
                    );
    characteristicRX->setCallbacks(new CharacteristicRXCallback()); // callbacks enable one to react of ble events such as characteristic write
 
    // Start the service
    myService->start();

    xTaskCreate(connectedTask, "connectedTask", 5000, NULL, 1, NULL);

    NimBLEAdvertising *myAdvertising = NimBLEDevice::getAdvertising();
    myAdvertising->addServiceUUID(myService->getUUID());
    myAdvertising->setScanResponse(true);
    myAdvertising->start();
}

// MAIN LOOP ---------------------------------

void setup()
{
    Serial.begin(115200);
    ArduinoSerial.begin(115200);
    
    setupBLE();
    Serial.println("Setup done");
}

void loop() {
    if (isBleConnected) {
        if (Serial.available()) {
            // if user write anything in Serial Monitor, echo it to BLE Client and Arduino
            String message = Serial.readStringUntil('\n');
            sendMsgUART(message);
            Serial.printf("Sent message: %s\n", message);
        }
    }
    delay(1000);
}