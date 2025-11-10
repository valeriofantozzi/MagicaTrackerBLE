/*
 * DisplayManager.h
 * Gestione display TFT e UI per MagicaTrackerBLE
 * UI/UX Design ottimizzato con elementi grafici moderni
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
#define UI_UPDATE_INTERVAL 50       // Aggiorna UI ogni 50ms (più fluido)
#define COUNTDOWN_THRESHOLD 10000   // Mostra countdown negli ultimi 10 secondi
#define HEARTBEAT_INTERVAL 400      // Intervallo heartbeat (ms) - più veloce
#define SEARCH_DOT_INTERVAL 200     // Intervallo animazione ricerca (ms) - più fluido
#define PULSE_ANIMATION_INTERVAL 100 // Intervallo animazione pulse (ms)

// ========== COLORI PERSONALIZZATI (RGB565) ==========
#define COLOR_BG_DARK      0x0000    // Nero puro
#define COLOR_BG_CARD      0x1082    // Grigio scuro per card
#define COLOR_ACCENT_BLUE  0x051F    // Blu scuro
#define COLOR_ACCENT_GREEN 0x07E0    // Verde brillante
#define COLOR_ACCENT_YELLOW 0xFFE0   // Giallo
#define COLOR_ACCENT_ORANGE 0xFD20  // Arancione
#define COLOR_TEXT_PRIMARY 0xFFFF    // Bianco
#define COLOR_TEXT_SECONDARY 0x8410  // Grigio chiaro
#define COLOR_BORDER       0x4208    // Grigio medio per bordi
#define COLOR_SUCCESS      0x07E0    // Verde successo
#define COLOR_WARNING      0xFFE0    // Giallo warning
#define COLOR_INACTIVE     0x4208    // Grigio inattivo

// Colore grigio scuro (se non definito nella libreria)
#ifndef ST77XX_DARKGREY
#define ST77XX_DARKGREY 0x4208
#endif

// ========== LAYOUT OTTIMIZZATO (135x240 pixel) ==========
#define DISPLAY_WIDTH 135
#define DISPLAY_HEIGHT 240
#define HEADER_HEIGHT 38
#define CARD_SPACING 3
#define CARD_PADDING 4
#define CARD_MARGIN 3
#define CARD_BLE_Y 42
#define CARD_BLE_HEIGHT 36
#define CARD_CONN_Y 82
#define CARD_CONN_HEIGHT 36
#define COUNTDOWN_Y 122
#define COUNTDOWN_HEIGHT 18
#define FOOTER_Y (DISPLAY_HEIGHT - 20)  // In fondo allo schermo (240 - 20 = 220)
#define FOOTER_HEIGHT 20

// ========== DIMENSIONI ELEMENTI ==========
#define ICON_SIZE 20
#define STATUS_INDICATOR_SIZE 6

// ========== CLASSE DISPLAY MANAGER ==========
class DisplayManager {
private:
  Adafruit_ST7789 tft;
  unsigned long lastUIUpdate;
  bool uiNeedsRedraw;
  
  // Variabili per tracking stato (ottimizzazione refresh)
  bool lastBleEnabled;
  bool lastDeviceConnected;
  unsigned long lastTimeToSleep;
  bool countdownWasVisible;
  unsigned long lastCountdownUpdate;
  
  // Variabili per animazioni
  unsigned long lastHeartbeat;
  bool heartbeatState;
  unsigned long lastSearchDot;
  int searchDotCount;
  unsigned long lastPulseAnimation;
  int pulseRadius;
  bool pulseExpanding;
  
  // Funzioni private per disegno componenti grafici
  void drawHeader();
  void drawCard(int x, int y, int w, int h, uint16_t bgColor = COLOR_BG_CARD);
  void drawBLEStatus(bool bleEnabled);
  void drawConnectionStatus(bool bleEnabled, bool deviceConnected);
  void drawCountdown(bool bleEnabled, unsigned long timeToSleep);
  void drawFooter();
  
  // Funzioni helper per refresh parziali (ottimizzazione animazioni)
  void updateHeartbeatText();
  void updateSearchDots();
  void updateCountdownText(unsigned long timeToSleep);
  void updateCountdownProgress(unsigned long timeToSleep);
  
  // Funzioni grafiche avanzate
  void drawBluetoothIcon(int x, int y, uint16_t color, bool filled = true);
  void drawConnectionIcon(int x, int y, uint16_t color, bool connected = false);
  void drawSleepIcon(int x, int y, uint16_t color);
  void drawButtonIcon(int x, int y, char label, uint16_t color);
  void drawStatusIndicator(int x, int y, uint16_t color, bool pulse = false);
  void drawProgressBar(int x, int y, int w, int h, float progress, uint16_t color);
  void drawPulseAnimation(int x, int y, int baseRadius, uint16_t color);
  
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
    lastBleEnabled(false),
    lastDeviceConnected(false),
    lastTimeToSleep(0),
    countdownWasVisible(false),
    lastCountdownUpdate(0),
    lastHeartbeat(0),
    heartbeatState(false),
    lastSearchDot(0),
    searchDotCount(0),
    lastPulseAnimation(0),
    pulseRadius(4),
    pulseExpanding(true) {
}

void DisplayManager::init() {
  // Inizializza backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // Inizializza display
  tft.init(135, 240);
  tft.setRotation(2);  // Portrait invertito (180 gradi)
  tft.fillScreen(COLOR_BG_DARK);
  
  // Reset variabili animazioni e stato
  uiNeedsRedraw = true;
  lastUIUpdate = 0;
  lastBleEnabled = false;
  lastDeviceConnected = false;
  lastTimeToSleep = 0;
  countdownWasVisible = false;
  lastCountdownUpdate = 0;
  heartbeatState = false;
  searchDotCount = 0;
  pulseRadius = 4;
  pulseExpanding = true;
  
  // Disegna UI iniziale
  drawHeader();
  drawFooter();
}

void DisplayManager::turnOff() {
  // Spegni backlight
  digitalWrite(TFT_BL, LOW);
  // Pulisci schermo
  tft.fillScreen(COLOR_BG_DARK);
}

void DisplayManager::turnOn() {
  // Accendi backlight
  digitalWrite(TFT_BL, HIGH);
  // Richiedi redraw completo
  uiNeedsRedraw = true;
}

// ========== FUNZIONI HELPER GRAFICHE ==========

void DisplayManager::drawCard(int x, int y, int w, int h, uint16_t bgColor) {
  // Disegna card con bordo arrotondato (simulato con rettangolo + bordi)
  tft.fillRect(x, y, w, h, bgColor);
  // Bordo superiore e inferiore
  tft.drawLine(x, y, x + w - 1, y, COLOR_BORDER);
  tft.drawLine(x, y + h - 1, x + w - 1, y + h - 1, COLOR_BORDER);
}

void DisplayManager::drawBluetoothIcon(int x, int y, uint16_t color, bool filled) {
  // Icona Bluetooth standard (ottimizzata per ICON_SIZE 20)
  // Simbolo Bluetooth: due triangoli che formano una "B" stilizzata
  int centerX = x + ICON_SIZE / 2;  // x + 10
  int centerY = y + ICON_SIZE / 2;  // y + 10
  
  if (filled) {
    // Disegna simbolo Bluetooth riempito
    // Triangolo superiore sinistro (punta verso il basso)
    tft.fillTriangle(centerX - 2, y + 3, centerX - 6, y + 7, centerX, y + 7, color);
    // Triangolo superiore destro (punta verso il basso)
    tft.fillTriangle(centerX + 2, y + 3, centerX + 6, y + 7, centerX, y + 7, color);
    // Barra verticale centrale
    tft.fillRect(centerX - 1, y + 7, 2, 6, color);
    // Triangolo inferiore sinistro (punta verso l'alto)
    tft.fillTriangle(centerX - 2, y + 17, centerX - 6, y + 13, centerX, y + 13, color);
    // Triangolo inferiore destro (punta verso l'alto)
    tft.fillTriangle(centerX + 2, y + 17, centerX + 6, y + 13, centerX, y + 13, color);
  } else {
    // Disegna simbolo Bluetooth outline (più sottile e preciso)
    // Triangolo superiore sinistro
    tft.drawTriangle(centerX - 2, y + 3, centerX - 6, y + 7, centerX, y + 7, color);
    // Triangolo superiore destro
    tft.drawTriangle(centerX + 2, y + 3, centerX + 6, y + 7, centerX, y + 7, color);
    // Barra verticale centrale (linee invece di rettangolo per essere più sottile)
    tft.drawLine(centerX, y + 7, centerX, y + 13, color);
    // Triangolo inferiore sinistro
    tft.drawTriangle(centerX - 2, y + 17, centerX - 6, y + 13, centerX, y + 13, color);
    // Triangolo inferiore destro
    tft.drawTriangle(centerX + 2, y + 17, centerX + 6, y + 13, centerX, y + 13, color);
  }
}

void DisplayManager::drawConnectionIcon(int x, int y, uint16_t color, bool connected) {
  int centerX = x + ICON_SIZE / 2;
  int centerY = y + ICON_SIZE / 2;
  
  if (connected) {
    // Icona connesso: due cerchi collegati (ridotta per ICON_SIZE 20)
    tft.fillCircle(centerX - 5, centerY, 3, color);
    tft.fillCircle(centerX + 5, centerY, 3, color);
    tft.drawLine(centerX - 2, centerY, centerX + 2, centerY, color);
  } else {
    // Icona disconnesso: cerchio con X (ridotta)
    tft.drawCircle(centerX, centerY, 5, color);
    tft.drawLine(centerX - 3, centerY - 3, centerX + 3, centerY + 3, color);
    tft.drawLine(centerX - 3, centerY + 3, centerX + 3, centerY - 3, color);
  }
}

void DisplayManager::drawSleepIcon(int x, int y, uint16_t color) {
  // Icona sleep: luna crescente (adattabile a diverse dimensioni)
  int iconSize = 16; // Dimensione ridotta per countdown
  int centerX = x + iconSize / 2;
  int centerY = y + iconSize / 2;
  int radius = iconSize / 2 - 1;
  tft.fillCircle(centerX, centerY, radius, color);
  tft.fillCircle(centerX - 2, centerY, radius, COLOR_BG_DARK);
}

void DisplayManager::drawButtonIcon(int x, int y, char label, uint16_t color) {
  // Disegna icona pulsante con label (ridotta per spazio)
  tft.drawRect(x, y, 18, 14, color);
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(x + 5, y + 3);
  tft.print(label);
}

void DisplayManager::drawStatusIndicator(int x, int y, uint16_t color, bool pulse) {
  if (pulse) {
    // Indicatore pulsante
    unsigned long currentTime = millis();
    if (currentTime - lastPulseAnimation >= PULSE_ANIMATION_INTERVAL) {
      if (pulseExpanding) {
        pulseRadius++;
        if (pulseRadius >= STATUS_INDICATOR_SIZE + 3) pulseExpanding = false;
      } else {
        pulseRadius--;
        if (pulseRadius <= STATUS_INDICATOR_SIZE) pulseExpanding = true;
      }
      lastPulseAnimation = currentTime;
    }
    tft.fillCircle(x, y, pulseRadius, color);
    tft.drawCircle(x, y, STATUS_INDICATOR_SIZE, COLOR_BG_DARK);
  } else {
    tft.fillCircle(x, y, STATUS_INDICATOR_SIZE, color);
  }
}

void DisplayManager::drawProgressBar(int x, int y, int w, int h, float progress, uint16_t color) {
  // Clamp progress tra 0 e 1
  if (progress < 0.0) progress = 0.0;
  if (progress > 1.0) progress = 1.0;
  
  // Background
  tft.fillRect(x, y, w, h, COLOR_BG_CARD);
  tft.drawRect(x, y, w, h, COLOR_BORDER);
  
  // Progress
  int progressWidth = (int)(w * progress);
  if (progressWidth > 0) {
    tft.fillRect(x + 1, y + 1, progressWidth - 2, h - 2, color);
  }
}

void DisplayManager::drawPulseAnimation(int x, int y, int baseRadius, uint16_t color) {
  // Animazione pulse con cerchi concentrici
  int alpha = 128; // Opacità ridotta per effetto fade
  for (int r = baseRadius; r < baseRadius + 8; r += 2) {
    uint16_t fadeColor = (color & 0xF800) >> 8 | (color & 0x07E0) | (color & 0x001F);
    tft.drawCircle(x, y, r, fadeColor);
  }
}

// ========== FUNZIONI DISEGNO COMPONENTI ==========

void DisplayManager::drawHeader() {
  // Header moderno con gradiente e design migliorato
  tft.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, COLOR_ACCENT_BLUE);
  
  // Titolo principale (ridotto per spazio)
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(4, 6);
  tft.print("MAGICA");
  
  // Sottotitolo
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(4, 16);
  tft.print("CAR TRACKER");
  
  // Icona Bluetooth decorativa nell'header (a destra)
  drawBluetoothIcon(DISPLAY_WIDTH - ICON_SIZE - 4, 9, COLOR_TEXT_PRIMARY, false);
  
  // Linea separatrice sottile
  tft.drawLine(0, HEADER_HEIGHT - 1, DISPLAY_WIDTH, HEADER_HEIGHT - 1, COLOR_BORDER);
}

void DisplayManager::drawBLEStatus(bool bleEnabled) {
  // Calcola larghezza card (display width - margini)
  int cardWidth = DISPLAY_WIDTH - (CARD_MARGIN * 2);
  
  // Pulisci area card
  tft.fillRect(CARD_MARGIN, CARD_BLE_Y, cardWidth, CARD_BLE_HEIGHT, COLOR_BG_DARK);
  
  // Disegna card BLE
  drawCard(CARD_MARGIN, CARD_BLE_Y, cardWidth, CARD_BLE_HEIGHT);
  
  // Icona Bluetooth
  uint16_t iconColor = bleEnabled ? COLOR_SUCCESS : COLOR_INACTIVE;
  int iconX = CARD_MARGIN + CARD_PADDING;
  int iconY = CARD_BLE_Y + (CARD_BLE_HEIGHT - ICON_SIZE) / 2;
  drawBluetoothIcon(iconX, iconY, iconColor, bleEnabled);
  
  // Testo stato (ridotto per spazio)
  int textX = iconX + ICON_SIZE + 4;
  tft.setTextSize(1);
  tft.setTextColor(bleEnabled ? COLOR_SUCCESS : COLOR_TEXT_SECONDARY);
  tft.setCursor(textX, CARD_BLE_Y + 8);
  tft.print("BLE");
  
  // Stato ON/OFF
  tft.setTextSize(1);
  tft.setTextColor(bleEnabled ? COLOR_SUCCESS : COLOR_INACTIVE);
  tft.setCursor(textX, CARD_BLE_Y + 20);
  tft.print(bleEnabled ? "ON" : "OFF");
  
  // Indicatore stato a destra
  int indicatorX = DISPLAY_WIDTH - CARD_MARGIN - STATUS_INDICATOR_SIZE - 2;
  int indicatorY = CARD_BLE_Y + CARD_BLE_HEIGHT / 2;
  drawStatusIndicator(indicatorX, indicatorY, iconColor, bleEnabled);
}

void DisplayManager::drawConnectionStatus(bool bleEnabled, bool deviceConnected) {
  // Calcola larghezza card (display width - margini)
  int cardWidth = DISPLAY_WIDTH - (CARD_MARGIN * 2);
  
  // Pulisci area card
  tft.fillRect(CARD_MARGIN, CARD_CONN_Y, cardWidth, CARD_CONN_HEIGHT, COLOR_BG_DARK);
  
  // Disegna card connessione
  drawCard(CARD_MARGIN, CARD_CONN_Y, cardWidth, CARD_CONN_HEIGHT);
  
  // Icona connessione
  uint16_t iconColor;
  const char* statusText;
  const char* statusSubtext;
  
  if (deviceConnected) {
    // Stato: CONNESSO
    iconColor = COLOR_SUCCESS;
    statusText = "Connesso";
    statusSubtext = "Tracking";
  } else if (bleEnabled) {
    // Stato: In attesa
    iconColor = COLOR_WARNING;
    statusText = "In attesa";
    statusSubtext = "Ricerca";
  } else {
    // Stato: Inattivo
    iconColor = COLOR_INACTIVE;
    statusText = "Inattivo";
    statusSubtext = "BLE OFF";
  }
  
  // Disegna icona connessione
  int iconX = CARD_MARGIN + CARD_PADDING;
  int iconY = CARD_CONN_Y + (CARD_CONN_HEIGHT - ICON_SIZE) / 2;
  drawConnectionIcon(iconX, iconY, iconColor, deviceConnected);
  
  // Testo stato principale
  int textX = iconX + ICON_SIZE + 4;
  tft.setTextSize(1);
  tft.setTextColor(iconColor);
  tft.setCursor(textX, CARD_CONN_Y + 8);
  tft.print(statusText);
  
  // Sottotesto (verrà aggiornato dalle funzioni di animazione se necessario)
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(textX, CARD_CONN_Y + 20);
  
  if (deviceConnected) {
    // Mostra animazione heartbeat
    tft.print(statusSubtext);
    if (heartbeatState) {
      tft.print("  ");
    }
  } else if (bleEnabled) {
    // Mostra animazione punti ricerca
    tft.print(statusSubtext);
    for (int i = 0; i < searchDotCount; i++) {
      tft.print(".");
    }
    for (int i = searchDotCount; i < 3; i++) {
      tft.print(" ");
    }
  } else {
    tft.print(statusSubtext);
  }
  
  // Indicatore stato animato a destra
  int indicatorX = DISPLAY_WIDTH - CARD_MARGIN - STATUS_INDICATOR_SIZE - 2;
  int indicatorY = CARD_CONN_Y + CARD_CONN_HEIGHT / 2;
  drawStatusIndicator(indicatorX, indicatorY, iconColor, deviceConnected || (bleEnabled && !deviceConnected));
}

void DisplayManager::drawCountdown(bool bleEnabled, unsigned long timeToSleep) {
  bool countdownVisible = (!bleEnabled && timeToSleep <= COUNTDOWN_THRESHOLD && timeToSleep > 0);
  
  // Se il countdown è appena apparso o scomparso, ridisegna tutto
  if (countdownVisible != countdownWasVisible) {
    // Pulisci area countdown
    tft.fillRect(CARD_MARGIN, COUNTDOWN_Y, DISPLAY_WIDTH - (CARD_MARGIN * 2), COUNTDOWN_HEIGHT, COLOR_BG_DARK);
    
    if (countdownVisible) {
      // Icona sleep a sinistra (ridotta)
      int iconSize = 16;
      int iconX = CARD_MARGIN + 2;
      int iconY = COUNTDOWN_Y + 1;
      drawSleepIcon(iconX, iconY, COLOR_WARNING);
      
      // Testo countdown iniziale
      tft.setTextSize(1);
      tft.setTextColor(COLOR_WARNING);
      int textX = iconX + iconSize + 3;
      unsigned long secondsLeft = (timeToSleep + 999) / 1000;
      tft.setCursor(textX, COUNTDOWN_Y + 3);
      tft.printf("Sleep %lus", secondsLeft);
      
      // Barra progresso iniziale
      int barX = textX;
      int barY = COUNTDOWN_Y + 11;
      int barWidth = DISPLAY_WIDTH - barX - CARD_MARGIN - 2;
      int barHeight = 5;
      float progress = 1.0 - ((float)timeToSleep / (float)COUNTDOWN_THRESHOLD);
      if (progress < 0.0) progress = 0.0;
      if (progress > 1.0) progress = 1.0;
      uint16_t barColor = (timeToSleep < 3000) ? COLOR_ACCENT_ORANGE : COLOR_WARNING;
      drawProgressBar(barX, barY, barWidth, barHeight, progress, barColor);
    }
    
    countdownWasVisible = countdownVisible;
    lastCountdownUpdate = millis();
  }
}

void DisplayManager::drawFooter() {
  // Footer moderno con icone pulsanti
  tft.fillRect(0, FOOTER_Y, DISPLAY_WIDTH, FOOTER_HEIGHT, COLOR_BG_DARK);
  
  // Linea separatrice superiore
  tft.drawLine(0, FOOTER_Y, DISPLAY_WIDTH, FOOTER_Y, COLOR_BORDER);
  
  // Calcola posizioni centrate (ottimizzate per 135px)
  // NOTA: Posizioni invertite per compensare rotazione 180 gradi
  int buttonIconSize = 18;
  int buttonLabelWidth = 20; // Spazio per label
  int totalButtonWidth = buttonIconSize + buttonLabelWidth + 2;
  int buttonSpacing = (DISPLAY_WIDTH - (totalButtonWidth * 2)) / 3;
  // Invertiti per rotazione 180 gradi: button2 a sinistra, button1 a destra
  int button2X = buttonSpacing;  // SLEEP a sinistra
  int button1X = buttonSpacing + totalButtonWidth + buttonSpacing;  // BLE a destra
  
  // Icona e label BUTTON2 (SLEEP - a sinistra dopo rotazione)
  drawButtonIcon(button2X, FOOTER_Y + 3, '2', COLOR_TEXT_PRIMARY);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(button2X + buttonIconSize + 2, FOOTER_Y + 7);
  tft.print("SLP");
  
  // Icona e label BUTTON1 (BLE - a destra dopo rotazione)
  drawButtonIcon(button1X, FOOTER_Y + 3, '1', COLOR_TEXT_PRIMARY);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(button1X + buttonIconSize + 2, FOOTER_Y + 7);
  tft.print("BLE");
  
  // Separatore verticale centrato
  int separatorX = DISPLAY_WIDTH / 2;
  tft.drawLine(separatorX, FOOTER_Y + 1, separatorX, FOOTER_Y + FOOTER_HEIGHT - 1, COLOR_BORDER);
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
    tft.fillScreen(COLOR_BG_DARK);
    drawHeader();
    drawFooter();
    uiNeedsRedraw = false;
    // Reset stato per forzare ridisegno completo dopo redraw
    lastBleEnabled = !bleEnabled;  // Forza cambiamento
    lastDeviceConnected = !deviceConnected;  // Forza cambiamento
    lastTimeToSleep = timeToSleep + 1000;  // Forza cambiamento
    countdownWasVisible = false;  // Reset countdown
  }
  
  // ========== OTTIMIZZAZIONE REFRESH: Aggiorna solo se necessario ==========
  
  // Salva valori precedenti per confronti (prima di aggiornarli)
  bool bleChanged = (bleEnabled != lastBleEnabled);
  bool deviceChanged = (deviceConnected != lastDeviceConnected);
  
  // Refresh BLE Status solo se bleEnabled è cambiato
  if (bleChanged) {
    drawBLEStatus(bleEnabled);
    lastBleEnabled = bleEnabled;
  }
  
  // Refresh Connection Status solo se stato è cambiato OPPURE se ci sono animazioni attive
  bool connectionStateChanged = bleChanged || deviceChanged;
  bool needsAnimationUpdate = false;
  
  if (deviceConnected) {
    // Animazione heartbeat: aggiorna ogni HEARTBEAT_INTERVAL
    if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
      heartbeatState = !heartbeatState;
      lastHeartbeat = currentTime;
      needsAnimationUpdate = true;
    }
  } else if (bleEnabled) {
    // Animazione search dots: aggiorna ogni SEARCH_DOT_INTERVAL
    if (currentTime - lastSearchDot >= SEARCH_DOT_INTERVAL) {
      searchDotCount = (searchDotCount + 1) % 4;
      lastSearchDot = currentTime;
      needsAnimationUpdate = true;
    }
  }
  
  // Aggiorna connection status se stato cambiato OPPURE se animazione richiede update
  if (connectionStateChanged || needsAnimationUpdate) {
    if (connectionStateChanged) {
      // Ridisegna completo se stato cambiato
      drawConnectionStatus(bleEnabled, deviceConnected);
    } else {
      // Aggiorna solo animazione se solo quella è cambiata
      if (deviceConnected) {
        updateHeartbeatText();
      } else if (bleEnabled) {
        updateSearchDots();
      }
    }
    lastBleEnabled = bleEnabled;
    lastDeviceConnected = deviceConnected;
  }
  
  // Refresh Countdown solo se necessario
  bool countdownVisible = (!bleEnabled && timeToSleep <= COUNTDOWN_THRESHOLD && timeToSleep > 0);
  bool countdownStateChanged = (countdownVisible != countdownWasVisible);
  bool countdownTimeChanged = false;
  
  if (countdownVisible) {
    // Aggiorna countdown ogni secondo quando visibile
    unsigned long secondsLeft = (timeToSleep + 999) / 1000;
    unsigned long lastSecondsLeft = (lastTimeToSleep + 999) / 1000;
    countdownTimeChanged = (secondsLeft != lastSecondsLeft) || (currentTime - lastCountdownUpdate >= 1000);
  }
  
  if (countdownStateChanged || countdownTimeChanged) {
    if (countdownStateChanged) {
      // Ridisegna completo se countdown appare/scompare
      drawCountdown(bleEnabled, timeToSleep);
    } else {
      // Aggiorna solo testo e progress bar se solo tempo cambiato
      updateCountdownText(timeToSleep);
      updateCountdownProgress(timeToSleep);
      lastCountdownUpdate = currentTime;
    }
    lastTimeToSleep = timeToSleep;
  }
}

void DisplayManager::requestRedraw() {
  uiNeedsRedraw = true;
}

// ========== FUNZIONI HELPER PER REFRESH PARZIALI (OTTIMIZZAZIONE ANIMAZIONI) ==========

void DisplayManager::updateHeartbeatText() {
  // Aggiorna solo il testo heartbeat senza ridisegnare tutta la card
  int iconX = CARD_MARGIN + CARD_PADDING;
  int textX = iconX + ICON_SIZE + 4;
  
  // Pulisci solo area testo sottotesto (larghezza sufficiente per "Tracking" + spazi)
  tft.fillRect(textX, CARD_CONN_Y + 20, 60, 10, COLOR_BG_CARD);
  
  // Ridisegna sottotesto con animazione heartbeat
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(textX, CARD_CONN_Y + 20);
  tft.print("Tracking");
  if (heartbeatState) {
    tft.print("  ");
  }
}

void DisplayManager::updateSearchDots() {
  // Aggiorna solo i punti di ricerca senza ridisegnare tutta la card
  int iconX = CARD_MARGIN + CARD_PADDING;
  int textX = iconX + ICON_SIZE + 4;
  
  // Pulisci area sottotesto (incluso testo "Ricerca" + punti)
  // "Ricerca" è 7 caratteri, quindi circa 42 pixel + spazio per 3 punti
  tft.fillRect(textX, CARD_CONN_Y + 20, 60, 10, COLOR_BG_CARD);
  
  // Ridisegna testo completo con punti
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(textX, CARD_CONN_Y + 20);
  tft.print("Ricerca");
  for (int i = 0; i < searchDotCount; i++) {
    tft.print(".");
  }
  for (int i = searchDotCount; i < 3; i++) {
    tft.print(" ");
  }
}

void DisplayManager::updateCountdownText(unsigned long timeToSleep) {
  // Aggiorna solo il testo del countdown senza ridisegnare tutto
  int iconSize = 16;
  int iconX = CARD_MARGIN + 2;
  int textX = iconX + iconSize + 3;
  
  // Pulisci solo area testo countdown
  tft.fillRect(textX, COUNTDOWN_Y + 3, 50, 8, COLOR_BG_DARK);
  
  // Ridisegna testo countdown
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  unsigned long secondsLeft = (timeToSleep + 999) / 1000;
  tft.setCursor(textX, COUNTDOWN_Y + 3);
  tft.printf("Sleep %lus", secondsLeft);
}

void DisplayManager::updateCountdownProgress(unsigned long timeToSleep) {
  // Aggiorna solo la progress bar senza ridisegnare tutto
  int iconSize = 16;
  int iconX = CARD_MARGIN + 2;
  int textX = iconX + iconSize + 3;
  int barX = textX;
  int barY = COUNTDOWN_Y + 11;
  int barWidth = DISPLAY_WIDTH - barX - CARD_MARGIN - 2;
  int barHeight = 5;
  
  // Calcola progress
  float progress = 1.0 - ((float)timeToSleep / (float)COUNTDOWN_THRESHOLD);
  if (progress < 0.0) progress = 0.0;
  if (progress > 1.0) progress = 1.0;
  
  // Colore barra cambia in base al tempo rimanente
  uint16_t barColor = (timeToSleep < 3000) ? COLOR_ACCENT_ORANGE : COLOR_WARNING;
  
  // Ridisegna solo progress bar
  drawProgressBar(barX, barY, barWidth, barHeight, progress, barColor);
}

#endif // DISPLAY_MANAGER_H

