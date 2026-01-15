//This code works for the several soil sensor: ESP32 DHT11 CP2104 WIFI Bluetooth Temperature Humidity Soil Moisture Sensor Detection Module 18650
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "DHT.h"

#define DHTPIN 22
#define DHTTYPE DHT11
#define SOIL_PIN 32

DHT dht(DHTPIN, DHTTYPE);

//////////////// BLE SERVICE //////////////////
#define SERVICE_UUID        "7E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "7E400002-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *bleServer;
BLECharacteristic *sensorChar;
bool deviceConnected = false;

//////////////// BLE CALLBACKS //////////////////
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    BLEDevice::startAdvertising();
  }
};

//////////////// BUFFER FOR PACKET //////////////////
char packetBuffer[64];

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(SOIL_PIN, INPUT);

  ////// BLE INIT ////// 
  BLEDevice::init("AllenAlienESP32");
  BLEDevice::setMTU(247);

  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new MyServerCallbacks());

  BLEService *service = bleServer->createService(SERVICE_UUID);

  sensorChar = service->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  sensorChar->addDescriptor(new BLE2902());

  service->start();

  BLEAdvertising *adv = bleServer->getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);

  adv->start();

  Serial.println("BLE JSON sensor service started and advertising!");
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();
  int soilValue     = analogRead(SOIL_PIN);
  int soilPercent   = map(soilValue, 4095, 0, 0, 100); //This value can be calibrated according to Project's need. 
  soilPercent       = constrain(soilPercent, 0, 100); // A simple mapping of the analog Value. 

  snprintf(packetBuffer, sizeof(packetBuffer),
           "{\"temp\":%.1f,\"hum\":%.1f,\"soil\":%d}",
           temperature, humidity, soilPercent);

  Serial.println(packetBuffer);

  if (deviceConnected) {
    sensorChar->setValue((uint8_t*)packetBuffer, strlen(packetBuffer));
    sensorChar->notify();
  }

  delay(1500);
}
