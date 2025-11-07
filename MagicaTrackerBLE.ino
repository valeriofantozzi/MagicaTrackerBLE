/*
 * ESP32 BLE Auto Tracker - Magica App
 * Versione con controllo pulsanti: BUTTON1 toggle BLE, BUTTON2 disponibile
 *
 * Hardware: LILYGO T-Display (ESP32)
 * Alimentazione: 12V auto → convertitore 5V → ESP32
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Button.h>

// ========== CONFIGURAZIONE ==========

// Nome dispositivo BLE (prova anche: "Magica", "MagicaTracker", "CarBLE")
#define DEVICE_NAME "MagicaCar"

// UUID da scoprire - questi sono esempi generici
#define SERVICE_UUID        "0000180a-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "00002a29-0000-1000-8000-00805f9b34fb"

// Pin pulsanti LILYGO T-Display
#define BUTTON1_PIN 35  // Toggle BLE service
#define BUTTON2_PIN 0   // Disponibile per altre funzioni

// Istanza libreria Button
Button button1(BUTTON1_PIN);
Button button2(BUTTON2_PIN);

// ========== VARIABILI GLOBALI ==========

BLEServer* pServer = NULL;
bool deviceConnected = false;
bool bleEnabled = false;  // Stato del servizio BLE (inizialmente disabilitato)

// ========== CALLBACK ==========

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("CONNESSO - Tracking attivo");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("DISCONNESSO - Riavvio advertising");
      delay(500);
      pServer->startAdvertising();
    }
};

// ========== FUNZIONI HELPER ==========

void enableBLE() {
  if (bleEnabled) return; // Già abilitato

  Serial.println("✓ Abilitazione servizio BLE...");

  if (!pServer) {
    // Prima inizializzazione
    BLEDevice::init(DEVICE_NAME);

    // Crea server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Crea servizio
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Crea caratteristica
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ   |
      BLECharacteristic::PROPERTY_NOTIFY
    );

    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("ESP32 Tracker");

    // Avvia servizio
    pService->start();
  }

  // Configura e avvia advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();

  bleEnabled = true;
  Serial.println("✓ BLE attivo e visibile");
  Serial.println("✓ In attesa connessione Magica...\n");
}

void disableBLE() {
  if (!bleEnabled) return; // Già disabilitato

  Serial.println("✗ Disabilitazione servizio BLE...");

  if (pServer) {
    // Ferma l'advertising (non deinizializzare completamente BLEDevice)
    pServer->getAdvertising()->stop();
    deviceConnected = false;
  }

  bleEnabled = false;
  Serial.println("✓ BLE disabilitato (advertising fermato)\n");
}

// ========== SETUP ==========

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 BLE Tracker per Magica ===\n");

  // Inizializza pulsanti
  button1.begin();
  button2.begin();

  Serial.println("✓ Pulsanti inizializzati");
  Serial.println("✓ Premi BUTTON1 (pin 35) per abilitare/disabilitare BLE");
  Serial.println("✓ BLE inizialmente DISABILITATO (no advertising)\n");
}

// ========== LOOP ==========

void loop() {
  // Gestione pulsante BUTTON1 per toggle BLE
  if (button1.pressed()) {
    if (bleEnabled) {
      disableBLE();
    } else {
      enableBLE();
    }
  }

  // Heartbeat se BLE attivo e connesso
  if (bleEnabled && deviceConnected) {
    Serial.print(".");  // Heartbeat visivo
  }

  delay(100);  // Piccolo delay per stabilità pulsanti
}