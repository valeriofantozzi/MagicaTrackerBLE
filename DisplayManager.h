/*
 * DisplayManager.h
 * Gestione display TFT e UI per MagicaTrackerBLE
 * 
 * Hardware: LILYGO T-Display (ESP32) - ST7789 135x240
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ========== CONFIGURAZIONE DISPLAY ==========
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23
#define TFT_BL 4

// ========== COSTANTI UI ==========
#define UI_UPDATE_INTERVAL 100      // Aggiorna UI ogni 100ms
#define COUNTDOWN_THRESHOLD 10000   // Mostra countdown negli ultimi 10 secondi
#define HEARTBEAT_INTERVAL 500      // Intervallo heartbeat (ms)
#define SEARCH_DOT_INTERVAL 300     // Intervallo animazione ricerca (ms)

// Colore grigio scuro (se non definito nella libreria)
#ifndef ST77XX_DARKGREY
#define ST77XX_DARKGREY 0x4208  // RGB565: 01000 100001 001000
#endif

// ========== AREA LAYOUT (Y coordinate) ==========
#define HEADER_Y_START 5
#define HEADER_Y_END 45
#define STATUS_Y_START 50
#define STATUS_Y_END 90
#define CONNECTION_Y_START 95
#define CONNECTION_Y_END 130
#define COUNTDOWN_Y_START 130
#define COUNTDOWN_Y_END 150
#define FOOTER_Y_START 150
#define FOOTER_Y_END 165

// ========== CLASSE DISPLAY MANAGER ==========
class DisplayManager {
private:
  Adafruit_ST7789 tft;
  unsigned long lastUIUpdate;
  bool uiNeedsRedraw;
  
  // Variabili per animazioni
  unsigned long lastHeartbeat;
  bool heartbeatState;
  unsigned long lastSearchDot;
  int searchDotCount;
  
  // Funzioni private per disegno componenti
  void drawHeader();
  void drawBLEStatus(bool bleEnabled);
  void drawConnectionStatus(bool bleEnabled, bool deviceConnected);
  void drawCountdown(bool bleEnabled, unsigned long timeToSleep);
  void drawFooter();
  
public:
  DisplayManager();
  
  // Inizializzazione e controllo display
  void init();
  void turnOff();
  void turnOn();
  
  // Gestione UI
  void update(bool bleEnabled, bool deviceConnected, unsigned long timeToSleep);
  void requestRedraw();
  
  // Accesso diretto al display (se necessario)
  Adafruit_ST7789* getDisplay() { return &tft; }
};

// ========== IMPLEMENTAZIONE ==========

DisplayManager::DisplayManager() 
  : tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST),
    lastUIUpdate(0),
    uiNeedsRedraw(true),
    lastHeartbeat(0),
    heartbeatState(false),
    lastSearchDot(0),
    searchDotCount(0) {
}

void DisplayManager::init() {
  // Inizializza backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // Inizializza display
  tft.init(135, 240);
  tft.setRotation(0);  // Portrait
  tft.fillScreen(ST77XX_BLACK);
  
  // Reset variabili
  uiNeedsRedraw = true;
  lastUIUpdate = 0;
  heartbeatState = false;
  searchDotCount = 0;
  
  // Disegna UI iniziale
  drawHeader();
  drawFooter();
}

void DisplayManager::turnOff() {
  // Spegni backlight
  digitalWrite(TFT_BL, LOW);
  // Pulisci schermo
  tft.fillScreen(ST77XX_BLACK);
}

void DisplayManager::turnOn() {
  // Accendi backlight
  digitalWrite(TFT_BL, HIGH);
  // Richiedi redraw completo
  uiNeedsRedraw = true;
}

void DisplayManager::drawHeader() {
  // Header fisso
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, HEADER_Y_START);
  tft.print("MAGICA CAR");
  tft.setCursor(10, HEADER_Y_START + 20);
  tft.print("TRACKER");
  
  // Linea separatrice
  tft.drawLine(0, HEADER_Y_END, 240, HEADER_Y_END, ST77XX_WHITE);
}

void DisplayManager::drawBLEStatus(bool bleEnabled) {
  // Pulisci area status
  tft.fillRect(0, STATUS_Y_START, 240, STATUS_Y_END - STATUS_Y_START, ST77XX_BLACK);
  
  tft.setTextSize(3);
  tft.setCursor(20, STATUS_Y_START + 5);
  
  if (bleEnabled) {
    tft.setTextColor(ST77XX_GREEN);
    tft.print("BLE: ON");
  } else {
    tft.setTextColor(ST77XX_WHITE);
    tft.print("BLE: OFF");
  }
}

void DisplayManager::drawConnectionStatus(bool bleEnabled, bool deviceConnected) {
  // Pulisci area connessione
  tft.fillRect(0, CONNECTION_Y_START, 240, CONNECTION_Y_END - CONNECTION_Y_START, ST77XX_BLACK);
  
  tft.setTextSize(2);
  tft.setCursor(20, CONNECTION_Y_START + 5);
  
  if (deviceConnected) {
    // Stato: CONNESSO
    tft.setTextColor(ST77XX_GREEN);
    tft.print("CONNESSO");
    
    // Animazione heartbeat
    unsigned long currentTime = millis();
    if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
      heartbeatState = !heartbeatState;
      lastHeartbeat = currentTime;
      
      // Cerchio pulsante
      if (heartbeatState) {
        tft.fillCircle(200, CONNECTION_Y_START + 10, 8, ST77XX_GREEN);
      } else {
        tft.fillCircle(200, CONNECTION_Y_START + 10, 8, ST77XX_BLACK);
        tft.drawCircle(200, CONNECTION_Y_START + 10, 8, ST77XX_GREEN);
      }
    }
  } else if (bleEnabled) {
    // Stato: In attesa
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("In attesa");
    
    // Animazione ricerca (punti)
    unsigned long currentTime = millis();
    if (currentTime - lastSearchDot >= SEARCH_DOT_INTERVAL) {
      searchDotCount = (searchDotCount + 1) % 4;
      lastSearchDot = currentTime;
      
      // Pulisci area punti
      tft.fillRect(150, CONNECTION_Y_START + 5, 50, 20, ST77XX_BLACK);
      tft.setTextColor(ST77XX_YELLOW);
      tft.setCursor(150, CONNECTION_Y_START + 5);
      for (int i = 0; i < searchDotCount; i++) {
        tft.print(".");
      }
    }
  } else {
    // Stato: Inattivo
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Inattivo");
  }
}

void DisplayManager::drawCountdown(bool bleEnabled, unsigned long timeToSleep) {
  // Pulisci area countdown
  tft.fillRect(0, COUNTDOWN_Y_START, 240, COUNTDOWN_Y_END - COUNTDOWN_Y_START, ST77XX_BLACK);
  
  // Mostra countdown solo se BLE è disabilitato e siamo negli ultimi 10 secondi
  if (!bleEnabled && timeToSleep <= COUNTDOWN_THRESHOLD) {
    // Testo countdown
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(10, COUNTDOWN_Y_START + 2);
    tft.printf("Auto sleep: %lus", timeToSleep / 1000);
    
    // Barra progresso
    int barWidth = 220;
    int barHeight = 8;
    int progress = map(timeToSleep, 0, COUNTDOWN_THRESHOLD, 0, barWidth);
    if (progress < 0) progress = 0;
    if (progress > barWidth) progress = barWidth;
    
    // Background barra
    tft.fillRect(10, COUNTDOWN_Y_START + 12, barWidth, barHeight, ST77XX_DARKGREY);
    // Progress barra
    tft.fillRect(10, COUNTDOWN_Y_START + 12, progress, barHeight, ST77XX_YELLOW);
  }
}

void DisplayManager::drawFooter() {
  // Footer fisso
  tft.fillRect(0, FOOTER_Y_START, 240, FOOTER_Y_END - FOOTER_Y_START, ST77XX_BLACK);
  
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, FOOTER_Y_START + 2);
  tft.print("B1:BLE  B2:SLEEP");
}

void DisplayManager::update(bool bleEnabled, bool deviceConnected, unsigned long timeToSleep) {
  unsigned long currentTime = millis();
  
  // Aggiorna UI solo ogni UI_UPDATE_INTERVAL ms
  if (currentTime - lastUIUpdate < UI_UPDATE_INTERVAL && !uiNeedsRedraw) {
    return;
  }
  
  lastUIUpdate = currentTime;
  
  // Redraw completo solo se necessario
  if (uiNeedsRedraw) {
    tft.fillScreen(ST77XX_BLACK);
    drawHeader();
    drawFooter();
    uiNeedsRedraw = false;
  }
  
  // Aggiorna solo le parti dinamiche
  drawBLEStatus(bleEnabled);
  drawConnectionStatus(bleEnabled, deviceConnected);
  drawCountdown(bleEnabled, timeToSleep);
}

void DisplayManager::requestRedraw() {
  uiNeedsRedraw = true;
}

#endif // DISPLAY_MANAGER_H

