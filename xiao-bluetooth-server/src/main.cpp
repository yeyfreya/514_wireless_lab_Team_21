#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data

const int trigPin = D2 /* Your Trigger Pin Number */;
const int echoPin = D1 /* Your Echo Pin Number */;

// Variables for distance measurement
unsigned long duration;
float distance;
const float speedOfSound = 0.0343; // Speed of sound in cm/us

// Moving average filter
const int numReadings = 10;
float readings[numReadings]; // the readings from the sensor
int readIndex = 0; // the index of the current reading
float total = 0; // the running total
float average = 0; // the average

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "8aafb443-8c53-4fee-b924-86e222348122"
#define CHARACTERISTIC_UUID "d3cc5929-1fca-454b-b359-73c9fa97af7e"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// TODO: add DSP algorithm functions here

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    // Initialize all the readings to 0:
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
    }

    // TODO: name your device to avoid conflictions
    BLEDevice::init("Team21_Server");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)

    // TODO: use your defined DSP algorithm to process the readings

    // Trigger the sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Read the echo
    duration = pulseIn(echoPin, HIGH);
    distance = (duration * speedOfSound) / 2; // Calculate the distance

    // Subtract the last reading:
    total = total - readings[readIndex];
    // Read from the sensor:
    readings[readIndex] = distance;
    // Add the reading to the total:
    total = total + readings[readIndex];
    // Advance to the next position in the array:
    readIndex = readIndex + 1;

    // If we're at the end of the array...
    if (readIndex >= numReadings) {
        // ...wrap around to the beginning:
        readIndex = 0;
    }

    // Calculate the average:
    average = total / numReadings;
    
    // Print the raw and denoised data
    Serial.print("Raw Distance: ");
    Serial.print(distance);
    Serial.print(" cm, Denoised Distance: ");
    Serial.print(average);
    Serial.println(" cm");


    // Transmit the data over BLE only if the denoised distance is less than 30 cm
    if (average < 30.0) {
        // Convert the distance to a string or a suitable format for transmission
        String distanceStr = String(average);

        // Set the value of the characteristic
        pCharacteristic->setValue(distanceStr.c_str());

        // Notify the connected client
        pCharacteristic->notify();

        // Optional: Print to the serial monitor for debugging
        Serial.println("Distance sent over BLE: " + distanceStr);
    }

    delay(100); // Delay between readings

    
    if (deviceConnected) {
        // Send new readings to database
        // TODO: change the following code to send your own readings and processed data
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
        pCharacteristic->setValue("Hello World");
        pCharacteristic->notify();
        Serial.println("Notify value: Hello World");
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}
