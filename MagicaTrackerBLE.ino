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
#include "Button2.h"

// ========== DISPLAY INCLUDES ==========
#include "DisplayManager.h"

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
#define BUTTON2_PIN 0   // Toggle deep sleep

// Istanza libreria Button2
Button2 button1, button2;

// ========== VARIABILI GLOBALI ==========

BLEServer* pServer = NULL;
bool deviceConnected = false;
bool bleEnabled = false;  // Stato del servizio BLE (inizialmente disabilitato)

// ========== DISPLAY MANAGER ==========
DisplayManager displayManager;

// ========== DEEP SLEEP VARIABILI ==========
RTC_DATA_ATTR bool deepSleepMode = false;  // Stato persistente attraverso deep sleep
RTC_DATA_ATTR int bootCount = 0;           // Contatore boot per debug

// ========== TIMEOUT AUTO SLEEP ==========
#define AUTO_SLEEP_TIMEOUT 10000  // x secondi di inattività prima di deep sleep
unsigned long lastActivityTime = 0;  // Timestamp ultimo input utente
unsigned long lastCountdownTime = 0; // Per evitare spam countdown
bool countdownActive = false;       // Flag per countdown attivo
bool autoSleepEnabled = true;       // Flag per controllo esplicito auto timeout deep sleep

// ========== CALLBACK ==========

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      // Il sistema di refresh ottimizzato rileverà automaticamente il cambiamento
      Serial.println("CONNESSO - Tracking attivo");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      // Il sistema di refresh ottimizzato rileverà automaticamente il cambiamento
      Serial.println("DISCONNESSO - Riavvio advertising");
      delay(500);
      pServer->startAdvertising();
    }
};

// ========== FUNZIONI HELPER ==========

void enableBLE() {
  if (bleEnabled) return; // Già abilitato
  
  // Verifica che non siamo in deep sleep mode
  if (deepSleepMode) {
    Serial.println("⚠️ BLE non può essere abilitato in modalità DEEP SLEEP");
    Serial.println("💡 Premi BUTTON2 per uscire dal deep sleep");
    return;
  }

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
  // Il sistema di refresh ottimizzato rileverà automaticamente il cambiamento
  Serial.println("✓ BLE attivo e visibile");
  Serial.println("✓ In attesa connessione Magica...\n");
}

// ========== DISABLE BLE ==========
// Questa funzione ferma BLE advertising
// Viene chiamata quando si disabilita BLE manualmente o prima di deep sleep
void disableBLE() {
  if (!bleEnabled) return; // Già disabilitato

  Serial.println("🔄 Disabilitazione BLE...");

  if (pServer) {
    // Ferma l'advertising prima di dormire
    pServer->getAdvertising()->stop();
    deviceConnected = false;
  }

  bleEnabled = false;
  // Il sistema di refresh ottimizzato rileverà automaticamente il cambiamento
  Serial.println("✓ BLE disabilitato\n");
}

// ========== NUOVA IMPLEMENTAZIONE DEEP SLEEP ==========
void enterDeepSleep() {
  Serial.println("🔋 Entrando in DEEP SLEEP per risparmio energetico...");
  
  // IMPORTANTE: Spegni display prima di deep sleep
  displayManager.turnOff();

  // Verifica che GPIO 0 sia valido per RTC wake up
  if (!esp_sleep_is_valid_wakeup_gpio(GPIO_NUM_0)) {
    Serial.println("❌ ERRORE: GPIO 0 non valido per wake up da deep sleep!");
    return;
  }

  // PULIZIA: Disabilita tutti i wake up precedenti per evitare conflitti
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // IMPORTANTE: Prima configura il pin come input con pull-up per stabilità
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // Breve delay per stabilizzare il pin (ridotto al minimo necessario)
  delay(5);

  // Verifica stato pin prima di configurare wake up
  int pinState = digitalRead(BUTTON2_PIN);

  // Sicurezza: se il pin è già LOW, non entrare in deep sleep per evitare loop
  if (pinState == LOW) {
    Serial.println("⚠️ ATTENZIONE: GPIO 0 è LOW - pulsante premuto o pin instabile!");
    Serial.println("⏸️  Annullamento deep sleep per sicurezza");
    return;
  }

  // Configura wake up source: ext0 - svegliati quando pulsante va LOW (premuto)
  // NOTA: Per pulsante collegato tra GPIO0 e GND:
  // - Non premuto: pin HIGH (grazie a pull-up)
  // - Premuto: pin LOW → wake up
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Wake on LOW (pulsante premuto)

  // Nota: Non configurare RTC GPIO aggiuntive per evitare conflitti con altre librerie

  deepSleepMode = true;  // Salva stato in RTC memory
  Serial.println("✅ Wake up configurato su GPIO 0 (BUTTON2) - Wake on LOW");
  Serial.printf("💤 Buona notte! Premi BUTTON2 per svegliare... (Boot #%d)\n\n", ++bootCount);

  // Entra in deep sleep - consumo scende a ~0.15mA
  esp_deep_sleep_start();

  // Questa linea non verrà mai eseguita
  Serial.println("ERRORE: Uscito da deep sleep senza wake up!");
}

// ========== BUTTON HANDLERS ==========

// Handler per BUTTON1: Toggle BLE advertising (solo se non in deep sleep)
void handleButton1Tap(Button2& btn) {
  // Tap handler come fallback se Button2 chiama tap invece di click
  handleButton1Click(btn);
}

void handleButton1Click(Button2& btn) {
  // Verifica che non siamo in deep sleep mode
  if (deepSleepMode) {
    Serial.println("⚠️ BUTTON1: BLE non disponibile in modalità DEEP SLEEP");
    Serial.println("💡 Premi BUTTON2 per uscire dal deep sleep");
    return;
  }

  unsigned long currentTime = millis();
  lastActivityTime = currentTime;  // Reset timer attività
  countdownActive = false;         // Reset countdown

  if (bleEnabled) {
    // Disabilita BLE advertising + riabilita auto timeout
    Serial.println("🔄 BUTTON1: BLE attivo → BLE disabilitato");
    disableBLE();
    autoSleepEnabled = true;  // Riabilita auto timeout
    lastActivityTime = millis();  // Reset timer per auto sleep (NON entra in deep sleep immediatamente)
    countdownActive = false;  // Reset countdown per sicurezza
    Serial.println("✓ Auto timeout deep sleep riabilitato (10 sec) - Timer resettato");
  } else {
    // Abilita BLE advertising + disabilita auto timeout
    Serial.println("🔄 BUTTON1: BLE disabilitato → BLE attivo");
    autoSleepEnabled = false;  // Disabilita auto timeout
    enableBLE();
    Serial.println("✓ Auto timeout deep sleep disabilitato");
  }
  
  // Il sistema di refresh ottimizzato rileverà automaticamente il cambiamento di bleEnabled
}

// Handler per BUTTON2: Toggle deep sleep mode (da qualsiasi stato)
void handleButton2Click(Button2& btn) {
  unsigned long currentTime = millis();
  lastActivityTime = currentTime;  // Reset timer attività
  countdownActive = false;         // Reset countdown

  if (deepSleepMode) {
    // Siamo in deep sleep mode → esci e fai reset completo
    Serial.println("🔄 BUTTON2: Uscita da DEEP SLEEP - Reset completo...");
    deepSleepMode = false;  // Reset flag prima del restart
    delay(100);  // Breve delay per permettere messaggio Serial
    ESP.restart();  // Reset completo - ricomincia da setup()
    return;  // Questa linea non verrà mai raggiunta
  } else {
    // Non siamo in deep sleep mode → abilitalo (entrata)
    Serial.println("🔄 BUTTON2: Abilitazione DEEP SLEEP mode");
    
    // Se BLE è attivo, disabilitalo prima di deep sleep
    // disableBLE() gestisce automaticamente la disconnessione e lo stop dell'advertising
    if (bleEnabled) {
      Serial.println("📡 Disabilitazione BLE prima di deep sleep...");
      disableBLE();
      // Piccolo delay per permettere al display di aggiornarsi e alla disconnessione di completarsi
      delay(200);
    }

    // Attendi che il pulsante venga rilasciato per evitare wake up immediato
    Serial.println("🔍 Attendo rilascio pulsante BUTTON2...");
    unsigned long waitStart = millis();
    while (digitalRead(BUTTON2_PIN) == LOW && (millis() - waitStart) < 2000) {
      delay(1);  // Delay minimo solo per non saturare la CPU (debounce gestito da Button2)
    }

    // Entra in deep sleep
    enterDeepSleep();  // Vai in deep sleep (displayManager.turnOff() chiamato dentro)
  }
}

// ========== HELPER FUNCTIONS DEEP SLEEP ==========

// Funzione per stampare la causa del wake up
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("🌅 Wake up: GPIO esterno (BUTTON2)"); break;
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
  delay(500);  // Tempo ridotto per aprire Serial Monitor (era 1000ms)

  bootCount++;
  Serial.printf("\n=== ESP32 BLE Tracker per Magica - Boot #%d ===\n", bootCount);

  // PULIZIA: Disabilita eventuali wake up sources residue per evitare auto-wake up
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // Verifica causa wake up
  print_wakeup_reason();

  // Gestione wake up da deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("✅ Svegliato da BUTTON2 - Uscita da DEEP SLEEP");
    
    // IMPORTANTE: Deinizializza RTC GPIO prima di riconfigurare il pin
    rtc_gpio_deinit(GPIO_NUM_0);
    delay(50);  // Delay per permettere la deinizializzazione
    
    // Riconfigura il pin come normale GPIO input
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
    delay(50);  // Delay per stabilizzare il pin
    
    // CRITICO: Attendi che il pulsante venga rilasciato prima del restart
    // Questo evita che il pulsante sia ancora premuto quando Button2 viene inizializzato
    Serial.println("🔍 Attendo rilascio pulsante BUTTON2 prima del restart...");
    unsigned long waitStart = millis();
    while (digitalRead(BUTTON2_PIN) == LOW && (millis() - waitStart) < 3000) {
      delay(10);  // Polling ogni 10ms
    }
    
    if (digitalRead(BUTTON2_PIN) == LOW) {
      Serial.println("⚠️  Pulsante ancora premuto dopo 3 secondi - procedo comunque");
    } else {
      Serial.println("✓ Pulsante rilasciato - procedo con restart");
    }
    
    Serial.println("🔄 Reset completo per ricominciare da zero...");
    deepSleepMode = false;  // Reset flag prima del restart
    delay(200);  // Delay per permettere messaggi Serial
    ESP.restart();  // Reset completo - ricomincia da setup()
    return;  // Questa linea non verrà mai raggiunta
  } else if (deepSleepMode) {
    // Stato deep sleep attivo ma wake up non da EXT0
    // Questo può succedere se il sistema si riavvia per altri motivi
    Serial.println("⚠️  Stato deep sleep attivo ma nessuna causa wake up valida");
    Serial.println("🔄 Possibile wake up spontaneo - mantenendo deep sleep mode");
    // Manteniamo deepSleepMode = true per entrare subito in sleep nel loop
    lastActivityTime = millis();  // Reset timer attività
    // Riaccendi display temporaneamente per debug
    displayManager.turnOn();
    displayManager.init();
  } else {
    // Avvio normale (non da deep sleep)
    lastActivityTime = millis();  // Inizializza timer attività
    autoSleepEnabled = true;  // Auto timeout attivo all'avvio (BLE disabled)
    // Inizializza display per avvio normale
    displayManager.init();
  }

  // Inizializza pulsanti sempre (BUTTON2 deve funzionare anche per uscire dal deep sleep)
  // BUTTON1 (pin 35): Input-only pin, NON supporta INPUT_PULLUP interno
  // Il pin 35 su ESP32 è ADC-only e richiede pull-up esterno sulla scheda
  pinMode(BUTTON1_PIN, INPUT);  // Configura manualmente come INPUT
  button1.begin(BUTTON1_PIN, INPUT, true);  // INPUT con activeLow=true
  button1.setDebounceTime(50);  // 50ms per maggiore stabilità
  button1.setDoubleClickTime(400);  // 400ms per distinguere click singolo da doppio
  button1.setTapHandler(handleButton1Tap);  // Tap handler come fallback
  button1.setClickHandler(handleButton1Click);
  
  // BUTTON2 (pin 0): Supporta INPUT_PULLUP (default)
  // IMPORTANTE: Assicurati che il pin sia configurato correttamente prima di begin()
  // Se viene da deep sleep, potrebbe essere ancora configurato come RTC GPIO
  // Reset esplicito del pin RTC se necessario
  rtc_gpio_deinit(GPIO_NUM_0);  // Deinizializza RTC GPIO se configurato
  delay(50);  // Delay aumentato per permettere la deinizializzazione completa
  
  pinMode(BUTTON2_PIN, INPUT_PULLUP);  // Configura esplicitamente come INPUT_PULLUP
  delay(50);  // Delay aumentato per stabilizzare completamente il pin
  
  // Verifica che il pin sia HIGH (non premuto) prima di inizializzare Button2
  // Se il pin è LOW, aspetta che venga rilasciato
  if (digitalRead(BUTTON2_PIN) == LOW) {
    Serial.println("⚠️  BUTTON2 è premuto durante inizializzazione - attendo rilascio...");
    unsigned long waitStart = millis();
    while (digitalRead(BUTTON2_PIN) == LOW && (millis() - waitStart) < 2000) {
      delay(10);
    }
    if (digitalRead(BUTTON2_PIN) == LOW) {
      Serial.println("⚠️  BUTTON2 ancora premuto dopo 2 secondi - procedo comunque");
    } else {
      Serial.println("✓ BUTTON2 rilasciato");
    }
  }
  
  button2.begin(BUTTON2_PIN);  // Usa default INPUT_PULLUP
  delay(50);  // Delay aumentato dopo begin() per stabilizzare completamente
  
  // Reset Button2 per assicurare stato pulito
  button2.reset();
  delay(50);  // Delay aumentato dopo reset
  
  button2.setDebounceTime(50);  // Debounce time aumentato per maggiore stabilità
  button2.setClickHandler(handleButton2Click);
  
  // Verifica finale che Button2 sia pronto
  delay(50);  // Delay finale per assicurare che tutto sia stabilizzato
  
  Serial.println("✓ Pulsanti inizializzati");
  Serial.println("✓ BUTTON1 (pin 35): Toggle BLE on/off (solo se non in deep sleep)");
  Serial.println("✓ BUTTON2 (pin 0): Toggle deep sleep on/off (da qualsiasi stato)");
  Serial.println("✓ BLE inizialmente DISABILITATO (no advertising)");
  Serial.printf("✓ AUTO SLEEP: entra in deep sleep dopo %d secondi di inattività (attivo all'avvio)\n", AUTO_SLEEP_TIMEOUT / 1000);
  Serial.println("✓ BUTTON1: quando abilita BLE → disabilita auto timeout, quando disabilita BLE → riabilita auto timeout");
  Serial.println("✓ BUTTON2: quando esce da deep sleep → reset completo e ricomincia da zero\n");
  
  // Disegna UI iniziale solo se non siamo in deep sleep mode
  if (!deepSleepMode) {
    displayManager.update(bleEnabled, deviceConnected, AUTO_SLEEP_TIMEOUT);
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
  
  // Calcola tempo rimanente per auto sleep
  unsigned long timeToSleep = 0;
  if (autoSleepEnabled) {
    unsigned long timeSinceActivity = currentTime - lastActivityTime;
    timeToSleep = (timeSinceActivity < AUTO_SLEEP_TIMEOUT) ? 
                   (AUTO_SLEEP_TIMEOUT - timeSinceActivity) : 0;
  }
  
  // Aggiorna UI continuamente
  displayManager.update(bleEnabled, deviceConnected, timeToSleep);

  // ========= GESTIONE PULSANTI CON BUTTON2 ==========
  // I callback vengono chiamati automaticamente quando necessario
  // BUTTON2 può sempre funzionare per toggle deep sleep
  button2.loop();
  
  // BUTTON1 funziona solo se non siamo in deep sleep (controllo già dentro handleButton1Click)
  if (!deepSleepMode) {
    button1.loop();
  }

  // IMPORTANTE: Ricalcola currentTime dopo gestione pulsanti per evitare race condition
  // I pulsanti potrebbero aver resettato lastActivityTime, quindi dobbiamo ricalcolare
  currentTime = millis();

  // Controllo timeout auto sleep (solo se auto timeout è abilitato e non già in deep sleep)
  if (!deepSleepMode && autoSleepEnabled && currentTime - lastActivityTime >= AUTO_SLEEP_TIMEOUT) {
    Serial.println("⏰ Timeout inattività raggiunto - Entrando in DEEP SLEEP automatico...");
    deepSleepMode = true;  // Abilita deep sleep mode
    enterDeepSleep();
    return;
  }

  // Countdown negli ultimi 10 secondi (solo se auto timeout è abilitato e non in deep sleep)
  if (!deepSleepMode && autoSleepEnabled) {
    unsigned long timeSinceActivity = currentTime - lastActivityTime;
    unsigned long timeToSleep = AUTO_SLEEP_TIMEOUT - timeSinceActivity;

    if (timeToSleep <= 10000) {  // Ultimi 10 secondi
      if (!countdownActive) {
        countdownActive = true;
        Serial.printf("⏰ Auto sleep tra %lu secondi - Premi BUTTON1 per annullare (abilita BLE)\n", timeToSleep / 1000);
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
    // Se auto timeout è disabilitato, reset countdown per sicurezza
    countdownActive = false;
  }

  // Heartbeat se BLE attivo e connesso
  if (bleEnabled && deviceConnected) {
    Serial.print(".");  // Heartbeat visivo
  }

  // Delay minimo solo per non saturare la CPU (debounce gestito da Button2)
  delay(10);
}