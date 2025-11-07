# LILYGO T-Display ESP32 - Documentazione Tecnica Completa

## 📋 Panoramica del Prodotto

Il **LILYGO T-Display** è una scheda di sviluppo ESP32 con display TFT integrato, progettata per applicazioni entry-level e progetti IoT portatili.

⚠️ **IMPORTANTE**: Esistono diverse versioni del T-Display (v1, v2, ecc.) con differenze hardware. Verifica la tua versione controllando il codice del progetto e gli schematici.

### Caratteristiche Principali

- **Microcontrollore**: ESP32 Xtensa dual-core LX6 microprocessor
- **Display**: 1.14" IPS ST7789V TFT LCD (135 x 240 pixel, 260 PPI)
- **Connettività**: Wi-Fi 802.11 b/g/n, Bluetooth V4.2+BLE
- **Memoria**: 4MB/16MB Flash, 520KB SRAM
- **Interfacce**: SPI, I2C, UART
- **Alimentazione**: 2.7V-4.2V (supporto batteria LiPo)
- **Dimensioni**: 51.52 × 25.04 × 8.54mm
- **Peso**: 7.81g

## 🔌 Specifiche Tecniche Dettagliate

### Microcontrollore ESP32

- **Chip**: ESP32 Xtensa dual-core LX6 microprocessor
- **Frequenza CPU**: 240MHz
- **SRAM**: 520KB
- **Flash**: QSPI 4MB (espandibile a 16MB)
- **Temperatura operativa**: -40°C ~ +85°C
- **Corrente di sonno**: ~120μA
- **Corrente operativa**: ~60mA

### Display

- **Tipo**: IPS ST7789V TFT LCD
- **Dimensione**: 1.14 pollici
- **Risoluzione**: 135 × 240 pixel
- **Densità pixel**: 260 PPI
- **Interfaccia**: 4-Wire SPI
- **Controller**: ST7789

### Connettività Wireless

#### Wi-Fi

- **Standard**: 802.11 b/g/n
- **Velocità massima**: 150Mbps (802.11n)
- **Potenza trasmissione**: 22dBm
- **Frequenza**: 2.4GHz ~ 2.5GHz (2400M~2483.5M)
- **Portata**: 300m
- **Sicurezza**: WPA/WPA2/WPA2-Enterprise/WPS
- **Modalità**: Station/SoftAP/SoftAP+Station/P2P

#### Bluetooth

- **Versione**: 4.2 BR/EDR e BLE
- **Sensibilità ricevitore**: -97dBm
- **Classe emettitore**: Class-1, Class-2, Class-3
- **Codec audio**: CVSD & SBC

### Alimentazione

- **Tensione operativa**: 2.7V - 4.2V
- **Connettore USB**: Type-C (5V/1A)
- **Batteria supportata**: 3.7V Lithium battery (LiPo/Li-Ion)
- **Connettore batteria**: JST 2Pin 1.25mm
- **Corrente di carica**: 500mA
- **Chip di carica**: TP4056

## 🔋 Gestione Batteria e Caricatore

### Chip di Carica TP4056

Il LILYGO T-Display utilizza il chip TP4056 per la gestione della carica della batteria LiPo.

#### Pin del TP4056

- **BAT**: Connesso al positivo della batteria (4.2V regolati)
- **GND**: Massa comune
- **VCC**: Alimentazione da USB (5V)
- **CHRG**: Uscita open-drain per stato carica (LOW = carica, HIGH-Z = non carica)
- **STDBY**: Uscita open-drain per standby (LOW = carica completa, HIGH-Z = in carica)
- **PROG**: Programmazione corrente carica e monitoraggio (RPROG = 2KΩ per 500mA)
- **TEMP**: Ingresso sensore temperatura (opzionale, può essere collegato a GND)

#### Stati di Carica

- **Carica in corso**: LED rosso acceso, CHRG = LOW
- **Carica completa**: LED blu/verde acceso, STDBY = LOW
- **Standby**: Entrambi i pin HIGH-Z

### Rilevamento Stato Batteria

#### Misurazione Tensione

- **Pin ADC**: GPIO 34
- **Circuito**: Divisore di tensione con rapporto 1:2 (2x 47KΩ)
- **Range ADC**: 0-3.3V (corrisponde a 0-7.26V batteria)
- **Precisione**: ±50mV (con calibrazione)

#### Lettura Tensione Batteria

```cpp
// Lettura tensione batteria
#define VBAT_PIN 34
#define VOLTAGE_DIVIDER_RATIO 2.0

float readBatteryVoltage() {
    int adcValue = analogRead(VBAT_PIN);
    // Conversione ADC -> tensione
    float adcVoltage = (adcValue / 4095.0) * 3.3;
    // Applicazione divisore di tensione
    float batteryVoltage = adcVoltage * VOLTAGE_DIVIDER_RATIO;
    return batteryVoltage;
}
```

#### Calcolo Percentuale Carica

La batteria LiPo ha una curva di scarica non lineare. Range tipico:

- **4.2V**: 100% (carica completa)
- **4.0V**: ~90%
- **3.7V**: ~50%
- **3.3V**: ~10%
- **2.7V**: 0% (scarica profonda - evitare!)

```cpp
int calculateBatteryPercentage(float voltage) {
    if (voltage >= 4.2) return 100;
    if (voltage <= 2.7) return 0;

    // Curva approssimata per LiPo 3.7V
    if (voltage >= 4.0) {
        return 90 + (voltage - 4.0) * (100 - 90) / (4.2 - 4.0);
    } else if (voltage >= 3.7) {
        return 50 + (voltage - 3.7) * (90 - 50) / (4.0 - 3.7);
    } else if (voltage >= 3.3) {
        return 10 + (voltage - 3.3) * (50 - 10) / (3.7 - 3.3);
    } else {
        return (voltage - 2.7) * 10 / (3.3 - 2.7);
    }
}
```

#### Libreria Pangodream per Calcolo Automatico

```cpp
#include <Pangodream_18650_CL.h>

#define ADC_PIN 34
#define CONV_FACTOR 1.7  // Fattore di conversione
#define READS 20         // Numero di letture per media

Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

void setup() {
    Serial.begin(115200);
}

void loop() {
    Serial.print("Tensione: ");
    Serial.print(BL.getBatteryVolts());
    Serial.println("V");

    Serial.print("Percentuale: ");
    Serial.print(BL.getBatteryChargeLevel());
    Serial.println("%");

    delay(1000);
}
```

### Rilevamento Stato Carica

Il TP4056 non ha pin direttamente accessibili sul T-Display, ma è possibile rilevare lo stato di carica indirettamente:

#### Metodo 1: Monitoraggio Tensione USB vs Batteria

```cpp
#define VBAT_PIN 34
#define USB_DETECT_PIN 14  // Se disponibile

bool isCharging() {
    float batteryVoltage = readBatteryVoltage();

    // Durante la carica, la tensione potrebbe essere diversa
    // Questo metodo non è affidabile al 100%
    // Meglio usare un pin dedicato del TP4056 se accessibile
}
```

#### Metodo 2: Modifica Hardware (Raccomandato)

Per un rilevamento affidabile, è necessario modificare il circuito aggiungendo:

- Connessione del pin CHRG del TP4056 a un GPIO libero
- Connessione del pin STDBY del TP4056 a un GPIO libero

```cpp
#define CHRG_PIN 12  // Pin collegato a CHRG del TP4056
#define STDBY_PIN 13 // Pin collegato a STDBY del TP4056

void setup() {
    pinMode(CHRG_PIN, INPUT_PULLUP);
    pinMode(STDBY_PIN, INPUT_PULLUP);
}

String getChargingStatus() {
    bool chrg = digitalRead(CHRG_PIN);
    bool stdby = digitalRead(STDBY_PIN);

    if (!chrg && stdby) return "CARICA IN CORSO";
    if (!stdby && chrg) return "CARICA COMPLETA";
    if (chrg && stdby) return "NON IN CARICA";
    return "STATO SCONOSCIUTO";
}
```

### Considerazioni Importanti

- **Misurazione accurata solo a batteria**: Quando collegato USB, la lettura ADC potrebbe mostrare la tensione di carica (5V) invece della batteria
- **Protezione batteria**: Non scaricare sotto 2.7V per evitare danni permanenti
- **Calibrazione**: Ogni divisore di tensione ha tolleranze - calibrare con multimetro
- **Corrente minima**: Il divisore consuma ~25μA continuamente
- **Deep Sleep**: Durante deep sleep, il consumo scende a ~220μA

## 📍 Pinout e Connessioni

### Pin GPIO Disponibili

- **Digitali**: GPIO 0, 4, 5, 18, 19, 21, 22, 34, 35
- **Analogici**: ADC_IN su GPIO 34
- **I2C**: SDA (GPIO 21), SCL (GPIO 22)
- **SPI**: MOSI (GPIO 19), SCLK (GPIO 18), CS (GPIO 5)

### Interfaccia Display TFT

```
TFT_MOSI: GPIO 19
TFT_SCLK: GPIO 18
TFT_CS:   GPIO 5
TFT_DC:   GPIO 16
TFT_RST:  GPIO 23 (alcune versioni) / N/A (altre versioni)
TFT_BL:   GPIO 4
```

**⚠️ Nota Versioni**: Il pin TFT_RST può variare tra versioni:

- **Versione con TFT_RST connesso**: GPIO 23
- **Versione senza TFT_RST**: N/A (non connesso)

### Pulsanti

- **BUTTON1**: GPIO 35
- **BUTTON2**: GPIO 0

### Alimentazione e Misc

- **ADC Power**: GPIO 14
- **Rilevamento batteria**: Supportato

### Pin Non Disponibili (Usati internamente)

- GPIO 1, 3 (UART)
- GPIO 6-11 (Flash)
- GPIO 12, 13, 15, 23, 25-27 (Display SPI/I2C)

## 🛠️ Programmazione e Sviluppo

### Piattaforme Supportate

- **Arduino IDE**
- **MicroPython**
- **ESP-IDF**
- **FreeRTOS**

### Librerie Richieste (Arduino)

- **TFT_eSPI**: Per il controllo del display
- **Adafruit_GFX**: Libreria grafica base
- **ESP32 Board Support**: Pacchetto ESP32 per Arduino

### Configurazione Arduino IDE

1. **Installare il supporto ESP32**:

   - Seguire la documentazione ufficiale: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html

2. **Impostazioni Board**:
   ```
   Board: ESP32 Dev Module
   Upload Speed: 921600
   CPU Frequency: 240MHz (WiFi/BT)
   Flash Frequency: 80MHz
   Flash Mode: QIO
   Flash Size: 4MB (32Mb)
   Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5 SPIFFS)
   Core Debug Level: None
   PSRAM: Disabled
   ```

### Configurazione TFT_eSPI

Per utilizzare correttamente la libreria TFT_eSPI, è necessario modificare il file `User_Setup_Select.h`.

**⚠️ IMPORTANTE**: La configurazione varia a seconda della versione del T-Display!

#### Per Versioni SENZA TFT_RST connesso (v1):

```cpp
// Abilitare la configurazione per LILYGO T-Display
#define USER_SETUP_LOADED
#define ILI9341_DRIVER  // Anche se è ST7789, questa configurazione funziona
#define TFT_WIDTH  135
#define TFT_HEIGHT 240

#define TFT_MISO -1
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC   16
#define TFT_RST  -1  // Non connesso
#define TFT_BL    4

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define SMOOTH_FONT
```

#### Per Versioni CON TFT_RST connesso (v2+):

```cpp
// Abilitare la configurazione per LILYGO T-Display
#define USER_SETUP_LOADED
#define ILI9341_DRIVER  // Anche se è ST7789, questa configurazione funziona
#define TFT_WIDTH  135
#define TFT_HEIGHT 240

#define TFT_MISO -1
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC   16
#define TFT_RST   23  // Connesso a GPIO 23
#define TFT_BL    4

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define SMOOTH_FONT
```

## 📚 Risorse e Documentazione

### Repository Ufficiali

- **GitHub Principale**: https://github.com/Xinyuan-LilyGO/TTGO-T-Display
- **GitHub T-Display S3**: https://github.com/Xinyuan-LilyGO/T-Display-S3

### Documentazione Ufficiale

- **Sito LILYGO**: https://lilygo.cc/products/t-display
- **Wiki LILYGO**: https://wiki.lilygo.cc/
- **Documentazione ESP32**: https://docs.espressif.com/projects/esp32/en/latest/

### Guide Community

- **Joen's Code Snippets**: https://sites.google.com/site/jmaathuis/arduino/lilygo-ttgo-t-display-esp32
- **HomeDing Library**: https://homeding.github.io/boards/esp32/ttgo-t-display.htm
- **DONE.LAND**: https://done.land/components/microcontroller/families/esp/esp32/developmentboards/esp32s/t-display/

## ⚠️ Limitazioni e Considerazioni

### Limitazioni Hardware

- **Pin GPIO limitati**: Solo 9 pin GPIO disponibili
- **Nessun PSRAM**: Non presente nella versione base
- **Display SPI condiviso**: Conflitti potenziali con altri dispositivi SPI
- **Alimentazione**: Sensibile alle variazioni di tensione

### Problemi Comuni

1. **Upload fallito**: Verificare la modalità boot (premere BOOT durante upload)
2. **Display nero**: Controllare connessione TFT_BL (GPIO 4)
3. **WiFi instabile**: Verificare alimentazione adeguata
4. **Pulsanti non funzionanti**: Controllare pull-up/down resistors

### Compatibilità

- **Arduino IDE**: Versione 1.8.10+
- **ESP32 Board Package**: Versione 1.0.6+
- **TFT_eSPI**: Versione 2.4.0+

## 🔧 Esempi di Codice

### Esempio Base Display

```cpp
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Hello T-Display!", 10, 10);
}

void loop() {
  // Codice principale
}
```

### Lettura Pulsanti

```cpp
#define BUTTON1_PIN 35
#define BUTTON2_PIN 0

void setup() {
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(BUTTON1_PIN) == LOW) {
    // Button 1 pressed
  }
  if (digitalRead(BUTTON2_PIN) == LOW) {
    // Button 2 pressed
  }
}
```

## 🛒 Acquisto e Supporto

- **Distributori**: LilyGO Official Store, AliExpress, Amazon
- **Prezzo**: ~$15-20 USD
- **Supporto**: Forum GitHub, Discord LilyGO
- **Certificazioni**: FCC/CE-RED/IC/TELEC/KCC/SRRC/NCC

## 📅 Versioni e Varianti

### Versioni del T-Display Base (ESP32)

- **T-Display v1**: Versione originale, TFT_RST non connesso (-1)
- **T-Display v2+**: Versioni successive, TFT_RST connesso a GPIO 23

### Come Identificare la Versione

Controlla il codice del tuo progetto:

- Se vedi `TFT_RST = 23` → Versione con RST connesso
- Se vedi `TFT_RST = -1` → Versione senza RST connesso

### Altre Varianti LILYGO

- **T-Display S3**: Versione con ESP32-S3 e display più grande
- **T-Display AMOLED**: Versione con display AMOLED
- **T-Display S3 Pro**: Versione avanzata con più funzionalità

---

_Documentazione compilata da analisi della documentazione ufficiale LILYGO e risorse community. Ultimo aggiornamento: Novembre 2025_
