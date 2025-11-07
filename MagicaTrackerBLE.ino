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

// ========== DEEP SLEEP INCLUDES ==========
#include "driver/rtc_io.h"  // Per RTC GPIO control
#include "esp_sleep.h"      // Per deep sleep functions

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

// ========== DEEP SLEEP VARIABILI ==========
RTC_DATA_ATTR bool deepSleepMode = false;  // Stato persistente attraverso deep sleep
RTC_DATA_ATTR int bootCount = 0;           // Contatore boot per debug

// ========== TIMEOUT AUTO SLEEP ==========
#define AUTO_SLEEP_TIMEOUT 10000  // x secondi di inattività prima di deep sleep
unsigned long lastActivityTime = 0;  // Timestamp ultimo input utente
unsigned long lastCountdownTime = 0; // Per evitare spam countdown
bool countdownActive = false;       // Flag per countdown attivo

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

// ========== DISABLE BLE (USATO INTERNO PRIMA DI DEEP SLEEP) ==========
// Questa funzione ferma BLE advertising prima di entrare in deep sleep
// Viene chiamata automaticamente quando si preme BUTTON1 con BLE attivo
void disableBLE() {
  if (!bleEnabled) return; // Già disabilitato

  Serial.println("🔄 Disabilitazione BLE prima di deep sleep...");

  if (pServer) {
    // Ferma l'advertising prima di dormire
    pServer->getAdvertising()->stop();
    deviceConnected = false;
  }

  bleEnabled = false;
  Serial.println("✓ BLE disabilitato - Pronto per deep sleep\n");
}

// ========== NUOVA IMPLEMENTAZIONE DEEP SLEEP ==========
void enterDeepSleep() {
  Serial.println("🔋 Entrando in DEEP SLEEP per risparmio energetico...");

  // Verifica che GPIO 35 sia valido per RTC wake up
  if (!esp_sleep_is_valid_wakeup_gpio(GPIO_NUM_35)) {
    Serial.println("❌ ERRORE: GPIO 35 non valido per wake up da deep sleep!");
    return;
  }

  // PULIZIA: Disabilita tutti i wake up precedenti per evitare conflitti
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // IMPORTANTE: Prima configura il pin come input con pull-up per stabilità
  pinMode(BUTTON1_PIN, INPUT_PULLUP);

  // Breve delay per stabilizzare il pin
  delay(10);

  // DEBUG: Verifica stato pin prima di configurare wake up
  int pinState = digitalRead(BUTTON1_PIN);
  Serial.printf("🔍 Stato GPIO 35 prima del deep sleep: %d (dovrebbe essere 1 = HIGH)\n", pinState);

  // Sicurezza: se il pin è già LOW, non entrare in deep sleep per evitare loop
  if (pinState == LOW) {
    Serial.println("⚠️ ATTENZIONE: GPIO 35 è LOW - pulsante premuto o pin instabile!");
    Serial.println("⏸️  Annullamento deep sleep per sicurezza");
    return;
  }

  // Configura wake up source: ext0 - svegliati quando pulsante va LOW (premuto)
  // NOTA: Per pulsante collegato tra GPIO35 e GND:
  // - Non premuto: pin HIGH (grazie a pull-up)
  // - Premuto: pin LOW → wake up
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);  // Wake on LOW (pulsante premuto)

  // Nota: Non configurare RTC GPIO aggiuntive per evitare conflitti con altre librerie

  deepSleepMode = true;  // Salva stato in RTC memory
  Serial.println("✅ Wake up configurato su GPIO 35 (BUTTON1) - Wake on LOW");
  Serial.printf("💤 Buona notte! Premi BUTTON1 per svegliare... (Boot #%d)\n\n", ++bootCount);

  // Entra in deep sleep - consumo scende a ~0.15mA
  esp_deep_sleep_start();

  // Questa linea non verrà mai eseguita
  Serial.println("ERRORE: Uscito da deep sleep senza wake up!");
}

// ========== HELPER FUNCTIONS DEEP SLEEP ==========

// Funzione per stampare la causa del wake up
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("🌅 Wake up: GPIO esterno (BUTTON1)"); break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("🌅 Wake up: GPIO esterno multiplo"); break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("🌅 Wake up: Timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("🌅 Wake up: Touch pad"); break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("🌅 Wake up: ULP program"); break;
    default:
      Serial.printf("🌅 Wake up: Altro (%d)\n", wakeup_reason); break;
  }
}

// ========== SETUP ==========

void setup() {
  Serial.begin(115200);
  delay(1000);  // Tempo per aprire Serial Monitor

  bootCount++;
  Serial.printf("\n=== ESP32 BLE Tracker per Magica - Boot #%d ===\n", bootCount);

  // PULIZIA: Disabilita eventuali wake up sources residue per evitare auto-wake up
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // Verifica causa wake up
  print_wakeup_reason();

  // Gestione wake up da deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("✅ Svegliato da BUTTON1 - Uscita da DEEP SLEEP");
    deepSleepMode = false;  // Reset stato deep sleep
    lastActivityTime = millis();  // Reset timer attività dopo wake up
  } else if (deepSleepMode) {
    Serial.println("⚠️  Stato deep sleep attivo ma nessuna causa wake up valida");
    Serial.println("🔄 Possibile wake up spontaneo - verificando stabilità pin...");
    deepSleepMode = false;  // Reset di sicurezza
    lastActivityTime = millis();  // Reset timer attività
  } else {
    // Avvio normale (non da deep sleep)
    lastActivityTime = millis();  // Inizializza timer attività
  }

  // Inizializza pulsanti solo se non siamo in deep sleep mode
  if (!deepSleepMode) {
  button1.begin();
  button2.begin();
  Serial.println("✓ Pulsanti inizializzati");
    Serial.println("✓ Premi BUTTON1 (pin 35) per BLE ↔ DEEP SLEEP");
    Serial.println("✓ BLE inizialmente DISABILITATO (no advertising)");
    Serial.printf("✓ AUTO SLEEP: entra in deep sleep dopo %d secondi di inattività (solo se BLE disabilitato)\n", AUTO_SLEEP_TIMEOUT / 1000);
    Serial.println("✓ DEEP SLEEP\n");
  } else {
    Serial.println("🔋 Modalità DEEP SLEEP attiva - Andando immediatamente in sleep...\n");
  }
}

// ========== LOOP ==========

void loop() {
  // Se siamo in modalità deep sleep, vai immediatamente in sleep
  if (deepSleepMode) {
    enterDeepSleep();
    return;  // Questa linea non verrà mai raggiunta
  }

  unsigned long currentTime = millis();

  // Gestione pulsante BUTTON1 per toggle BLE ↔ DEEP SLEEP
  if (button1.pressed()) {
    lastActivityTime = currentTime;  // Reset timer attività
    countdownActive = false;         // Reset countdown

    if (bleEnabled) {
      // Invece di disableBLE(), entra in deep sleep per risparmio energetico
      Serial.println("🔄 Transizione: BLE attivo → DEEP SLEEP");
      disableBLE();  // Prima disabilita BLE (legacy function)

      // Attendi che il pulsante venga rilasciato per evitare wake up immediato
      Serial.println("🔍 Attendo rilascio pulsante...");
      while (digitalRead(BUTTON1_PIN) == LOW) {
        delay(10);  // Piccolo delay per debounce
      }
      delay(100);  // Delay aggiuntivo per sicurezza

      enterDeepSleep();  // Poi vai in deep sleep
    } else {
      // Da inattivo a BLE attivo
      Serial.println("🔄 Transizione: Inattivo → BLE attivo");
      enableBLE();
    }
  }

  // Controllo timeout auto sleep (solo se BLE è disabilitato)
  if (!bleEnabled && currentTime - lastActivityTime >= AUTO_SLEEP_TIMEOUT) {
    Serial.println("⏰ Timeout inattività raggiunto - Entrando in DEEP SLEEP automatico...");
    enterDeepSleep();
    return;
  }

  // Countdown negli ultimi 10 secondi (solo se BLE è disabilitato)
  if (!bleEnabled) {
    unsigned long timeSinceActivity = currentTime - lastActivityTime;
    unsigned long timeToSleep = AUTO_SLEEP_TIMEOUT - timeSinceActivity;

    if (timeToSleep <= 10000) {  // Ultimi 10 secondi
      if (!countdownActive) {
        countdownActive = true;
        Serial.printf("⏰ Auto sleep tra %lu secondi - Premi BUTTON1 per annullare\n", timeToSleep / 1000);
        lastCountdownTime = currentTime;
      } else if (currentTime - lastCountdownTime >= 1000) {  // Aggiorna ogni secondo
        unsigned long secondsLeft = timeToSleep / 1000;
        if (secondsLeft > 0) {
          Serial.printf("⏰ %lu...\n", secondsLeft);
        }
        lastCountdownTime = currentTime;
      }
    } else {
      countdownActive = false;  // Reset se siamo usciti dal countdown
    }
  } else {
    // Se BLE è attivo, reset countdown per sicurezza
    countdownActive = false;
  }

  // Heartbeat se BLE attivo e connesso
  if (bleEnabled && deviceConnected) {
    Serial.print(".");  // Heartbeat visivo
  }

  delay(100);  // Piccolo delay per stabilità pulsanti
}