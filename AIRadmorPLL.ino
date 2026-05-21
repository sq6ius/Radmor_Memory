/*
 * si5351_radmor_controller.ino - Optimized example of using Si5351Arduino library
 *
 * Based on original work by Jason Milldrum <milldrumgmail.com>
 * and adapted for Radmor channel control with EEPROM and improved frequency calculations.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <si5351.h>
#include <Wire.h>
#include <EEPROM.h> // Dołącz bibliotekę EEPROM

// --- Definicje Pinów ---
const int PTT_PIN = 2;
const int CH_PINS[] = {3, 4, 5, 6, 7, 8, 9, 10}; // Piny dla kanałów CH1 do CH8
const int NUM_CHANNELS = sizeof(CH_PINS) / sizeof(CH_PINS[0]);

// --- Definicje EEPROM ---
const int EEPROM_INIT_FLAG_ADDR = 0; // Adres dla flagi inicjalizacji EEPROM
const int EEPROM_CHANNEL_FREQ_START_ADDR = 1; // Początkowy adres dla częstotliwości kanałów
const int EEPROM_CHANNEL_COUNT = 7; // Liczba kanałów, które chcemy przechowywać w EEPROM

// --- Definicje Częstotliwości ---
const float RX_IF_OFFSET_KHZ = 10700.0; // Częstotliwość pośrednia RX w kHz (10.7 MHz)
const float TX_OFFSET_KHZ = 600.0;     // Offset TX w kHz (np. +600 kHz dla przemienników VHF)

// --- Obiekty Bibliotek ---
Si5351 si5351;

// --- Zmienne Globalne ---
volatile float currentChannelFreq_kHz = 145350.0; // Aktualna częstotliwość kanału w kHz
int selectedChannel = 0; // Aktualnie wybrany kanał (0 = brak wyboru lub domyślny)

// --- Prototypy Funkcji ---
void initializeEEPROM();
void printChannelFrequencies();
int readSelectedChannel();
uint64_t getRxFreqHz(float channelFreq_kHz);
uint64_t getTxFreqHz(float channelFreq_kHz);
void startTransmitting();
void startReceiving();
void handleSerialInput();

// --- Funkcja Setup ---
void setup() {
  // Konfiguracja pinów
  pinMode(PTT_PIN, INPUT_PULLUP); // PTT pin jako wejście z podciąganiem
  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(CH_PINS[i], INPUT_PULLUP); // Piny kanałów jako wejścia z podciąganiem
  }

  // Inicjalizacja komunikacji szeregowej
  Serial.begin(9600);
  Serial.println("\n--- Si5351 Radmor Controller ---");

  // Inicjalizacja EEPROM
  EEPROM.begin(512); // Inicjalizacja EEPROM (rozmiar może być większy w zależności od mikrokontrolera)
  initializeEEPROM();

  // Inicjalizacja Si5351
  bool i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if (!i2c_found) {
    Serial.println("Błąd: Urządzenie Si5351 nie znaleziono na magistrali I2C!");
    while (true); // Zatrzymanie programu w przypadku błędu
  }

  // Ustawienie początkowych częstotliwości (można dostosować)
  // Te częstotliwości mogą być używane do testowania lub jako domyślne dla innych celów
  si5351.set_freq(134650000ULL, SI5351_CLK0); // Przykład: 134.65 MHz na CLK0
  si5351.set_freq(16150000ULL, SI5351_CLK1);  // Przykład: 16.15 MHz na CLK1
  si5351.update_status();
  delay(500);

  Serial.println("\nProgramowanie Kanalow Radmorka:");
  Serial.println("W celu zaprogramowania wybierz nr kanalu (1-7) i wcisnij Enter.");
  Serial.println("Nastepnie wprowadz czestotliwosc w MHz (np. 145.350) i wcisnij Enter.");
  printChannelFrequencies(); // Wyświetl aktualne częstotliwości kanałów
}

// --- Funkcja Loop ---
void loop() {
  // 1. Odczyt wybranego kanału
  int newSelectedChannel = readSelectedChannel();
  if (newSelectedChannel != selectedChannel) {
    selectedChannel = newSelectedChannel;
    if (selectedChannel > 0 && selectedChannel <= EEPROM_CHANNEL_COUNT) {
      // Odczytaj częstotliwość z EEPROM dla wybranego kanału
      EEPROM.get(EEPROM_CHANNEL_FREQ_START_ADDR + (selectedChannel - 1) * sizeof(float), currentChannelFreq_kHz);
      Serial.print("Wybrano kanal: CH");
      Serial.print(selectedChannel);
      Serial.print(", Czestotliwosc: ");
      Serial.print(currentChannelFreq_kHz, 3); // Wyświetl z 3 miejscami po przecinku
      Serial.println(" MHz");
    } else {
      // Domyślna częstotliwość, jeśli żaden kanał nie jest wybrany lub poza zakresem EEPROM
      currentChannelFreq_kHz = 145350.0; // Domyślna częstotliwość
      Serial.println("Brak wybranego kanalu lub kanal poza zakresem EEPROM. Uzywam domyslnej czestotliwosci.");
    }
  }

  // 2. Odczyt stanu PTT i przełączanie TX/RX
  if (digitalRead(PTT_PIN) == LOW) { // PTT aktywny (przycisk PTT podłączony do GND)
    startTransmitting();
  } else { // PTT nieaktywny
    startReceiving();
  }

  // 3. Obsługa wejścia szeregowego (programowanie kanałów)
  handleSerialInput();

  delay(100); // Krótkie opóźnienie, aby uniknąć nadmiernego obciążenia procesora
}

// --- Implementacje Funkcji ---

/**
 * @brief Inicjalizuje EEPROM z domyślnymi wartościami, jeśli nie został wcześniej zainicjalizowany.
 */
void initializeEEPROM() {
  byte initFlag = EEPROM.read(EEPROM_INIT_FLAG_ADDR);
  if (initFlag != 0xDE) { // Sprawdź "magiczną liczbę"
    Serial.println("Inicjalizacja EEPROM domyslnymi wartosciami...");
    float defaultChannelFrequencies_MHz[] = {
      145.350, 145.450, 145.550, 145.600, 144.500, 145.7875, 145.500
    };

    for (int i = 0; i < EEPROM_CHANNEL_COUNT; i++) {
      EEPROM.put(EEPROM_CHANNEL_FREQ_START_ADDR + i * sizeof(float), defaultChannelFrequencies_MHz[i]);
    }
    EEPROM.write(EEPROM_INIT_FLAG_ADDR, 0xDE); // Zapisz flagę inicjalizacji
    EEPROM.commit(); // Zapisz zmiany w EEPROM (ważne dla ESP32/ESP8266)
    Serial.println("EEPROM zainicjalizowany.");
  }
}

/**
 * @brief Wyświetla aktualne częstotliwości wszystkich kanałów z EEPROM.
 */
void printChannelFrequencies() {
  float freq_MHz;
  for (int i = 0; i < EEPROM_CHANNEL_COUNT; i++) {
    EEPROM.get(EEPROM_CHANNEL_FREQ_START_ADDR + i * sizeof(float), freq_MHz);
    Serial.print("CH");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(freq_MHz, 4); // Wyświetl z 4 miejscami po przecinku
    Serial.println(" MHz");
  }
}

/**
 * @brief Odczytuje, który pin kanału jest aktywny (LOW).
 * @return Numer aktywnego kanału (1-8) lub 0, jeśli żaden nie jest aktywny.
 */
int readSelectedChannel() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (digitalRead(CH_PINS[i]) == LOW) {
      return i + 1; // Zwróć numer kanału (1-8)
    }
  }
  return 0; // Żaden kanał nie jest wybrany
}

/**
 * @brief Oblicza częstotliwość RX dla Si5351 w Hz.
 * @param channelFreq_MHz Częstotliwość kanału w MHz.
 * @return Częstotliwość RX w Hz (uint64_t).
 */
uint64_t getRxFreqHz(float channelFreq_MHz) {
  // Częstotliwość kanału (MHz) - IF (MHz) = Częstotliwość Si5351 (MHz)
  // Przekształć na Hz: (channelFreq_MHz * 1000000) - (RX_IF_OFFSET_KHZ * 1000)
  return (uint64_t)((channelFreq_MHz * 1000.0) - RX_IF_OFFSET_KHZ) * 1000ULL;
}

/**
 * @brief Oblicza częstotliwość TX dla Si5351 w Hz.
 * @param channelFreq_MHz Częstotliwość kanału w MHz.
 * @return Częstotliwość TX w Hz (uint64_t).
 */
uint64_t getTxFreqHz(float channelFreq_MHz) {
  // Częstotliwość kanału (MHz) + TX Offset (MHz) = Częstotliwość Si5351 (MHz)
  // Przekształć na Hz: (channelFreq_MHz * 1000000) + (TX_OFFSET_KHZ * 1000)
  return (uint64_t)((channelFreq_MHz * 1000.0) + TX_OFFSET_KHZ) * 1000ULL;
}

/**
 * @brief Konfiguruje Si5351 do trybu nadawania.
 */
void startTransmitting() {
  static bool wasReceiving = true; // Statyczna zmienna do śledzenia poprzedniego stanu
  if (wasReceiving) {
    uint64_t txFreq = getTxFreqHz(currentChannelFreq_kHz / 1000.0); // Konwertuj kHz na MHz dla funkcji
    si5351.set_freq(txFreq, SI5351_CLK1); // Ustaw częstotliwość TX na CLK1
    si5351.output_enable(SI5351_CLK0, 0); // Wyłącz CLK0 (RX)
    si5351.output_enable(SI5351_CLK1, 1); // Włącz CLK1 (TX)
    Serial.print("TRANSMITTING na ");
    Serial.print(txFreq / 1000000.0, 3);
    Serial.println(" MHz");
    wasReceiving = false;
  }
}

/**
 * @brief Konfiguruje Si5351 do trybu odbioru.
 */
void startReceiving() {
  static bool wasTransmitting = true; // Statyczna zmienna do śledzenia poprzedniego stanu
  if (wasTransmitting) {
    uint64_t rxFreq = getRxFreqHz(currentChannelFreq_kHz / 1000.0); // Konwertuj kHz na MHz dla funkcji
    si5351.set_freq(rxFreq, SI5351_CLK0); // Ustaw częstotliwość RX na CLK0
    si5351.output_enable(SI5351_CLK1, 0); // Wyłącz CLK1 (TX)
    si5351.output_enable(SI5351_CLK0, 1); // Włącz CLK0 (RX)
    Serial.print("RECEIVING na ");
    Serial.print(rxFreq / 1000000.0, 3);
    Serial.println(" MHz");
    wasTransmitting = false;
  }
}

/**
 * @brief Obsługuje wejście z portu szeregowego do programowania częstotliwości kanałów.
 */
void handleSerialInput() {
  if (Serial.available() > 0) {
    int channelToProgram = Serial.parseInt(); // Odczytaj numer kanału
    if (channelToProgram >= 1 && channelToProgram <= EEPROM_CHANNEL_COUNT) {
      Serial.print("Edycja kanalu CH");
      Serial.println(channelToProgram);
      Serial.print("Wprowadz czestotliwosc w MHz (np. 145.350): ");

      // Czekaj na wprowadzenie częstotliwości
      while (Serial.available() == 0) {
        // Czekaj
      }
      float newFreq_MHz = Serial.parseFloat(); // Odczytaj częstotliwość
      Serial.println(newFreq_MHz, 3);

      if (newFreq_MHz > 0) { // Sprawdź, czy częstotliwość jest prawidłowa
        EEPROM.put(EEPROM_CHANNEL_FREQ_START_ADDR + (channelToProgram - 1) * sizeof(float), newFreq_MHz);
        EEPROM.commit(); // Zapisz zmiany w EEPROM
        Serial.println("Czestotliwosc zapisana pomyslnie!");
        printChannelFrequencies(); // Wyświetl zaktualizowane częstotliwości
      } else {
        Serial.println("Nieprawidlowa czestotliwosc. Zapis nie powiodl sie.");
      }
    } else if (channelToProgram != 0) { // Ignoruj 0, jeśli parseInt nie znalazł liczby
      Serial.println("Nieprawidlowy numer kanalu. Wybierz 1-7.");
    }
    // Wyczyść bufor szeregowy po przetworzeniu
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
}
