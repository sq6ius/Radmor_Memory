/*
 * si5351_example.ino - Simple example of using Si5351Arduino library
 *
 * Copyright (C) 2015 - 2016 Jason Milldrum <milldrum@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "si5351.h"
#include "Wire.h"

Si5351 si5351;
const int PTTpin = 2;
const int CH1 = 3;
const int CH2 = 4;
const int CH3 = 5;
const int CH4 = 6;
const int CH5 = 7;
const int CH6 = 8;
const int CH7 = 9;
const int CH8 = 10;

volatile int long QRG = 145350;
//volatile int TXQRG = 0;

// variables will change:
int PTT = 0;
int CHANEL = 0;

int wybor = 0;
float wypisz = 0;

void setup()
{
  bool i2c_found;
  pinMode(PTTpin, INPUT);
  pinMode(CH1, INPUT_PULLUP);
  pinMode(CH2, INPUT_PULLUP);
  pinMode(CH3, INPUT_PULLUP);
  pinMode(CH4, INPUT_PULLUP);
  pinMode(CH5, INPUT_PULLUP);
  pinMode(CH6, INPUT_PULLUP);
  pinMode(CH7, INPUT_PULLUP);
  pinMode(CH8, INPUT_PULLUP);

  //float CH_1 = 145.3500;
  //float CH_2 = 144.8000;
  //float CH_3 = 144.5000;
  //float CH_4 = 145.7125;
  //float CH_5 = 145.0000;
  //float CH_6 = 145.7875;
  //float CH_7 = 145.6000;
  
  /*
   * eeprom_write_block(&CH_1,0,4);
  eeprom_write_block(&CH_2,4,4);
  eeprom_write_block(&CH_3,8,4);
  eeprom_write_block(&CH_4,12,4);
  eeprom_write_block(&CH_5,16,4);
  eeprom_write_block(&CH_6,20,4);
  eeprom_write_block(&CH_7,24,4);
  */
  
  // Start serial and initialize the Si5351
  Serial.begin(9600);


  Serial.println(" ##    #   ##   # # #   #   ##");
  Serial.println(" ##   ###  # #  # # #  # #  ##");
  Serial.println(" # #  # #  ##   #   #   #   # #");
  

  //Serial.println(" ##   #  ##  #   #  #  ## ");
  //Serial.println(" # # # # # # ## ## # # # #");
  //Serial.println(" ##  ### # # # # # # # ##");
 // Serial.println(" # # # # ##  #   #  #  # #");
  
  i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  //si5351.set_correction(132700, SI5351_PLL_INPUT_XO);
  //si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  if(!i2c_found)
  {
    Serial.println("Device not found on I2C bus!");
  }

  // Set CLK0 to output 134.650 MHz
  si5351.set_freq(1346500000ULL, SI5351_CLK0);

  // Set CLK1 to output 16.15 MHz
  si5351.set_ms_source(SI5351_CLK1, SI5351_PLLB);
  si5351.set_freq(1615000000ULL, SI5351_CLK1);
  si5351.update_status();
  delay(500);

  Serial.println("Programowanie Kanalow Radmorka, w celu zaprogramowania wybierz nr kanalu 1-7  ");
  Serial.println("wcisnij enter i gdy wyswietli sie nr kanalu wprowadz czestotliwosc np 145.350 ");

  Serial.print("CH1: ");
  eeprom_read_block(&wypisz,0,4);
  Serial.println(wypisz, 5);

  Serial.print("CH2: ");
  eeprom_read_block(&wypisz,4,4);
  Serial.println(wypisz, 5);
  
  Serial.print("CH3: ");
  eeprom_read_block(&wypisz,8,4);
  Serial.println(wypisz, 5);

  Serial.print("CH4: ");
  eeprom_read_block(&wypisz,12,4);
  Serial.println(wypisz, 5);

  Serial.print("CH5: ");
  eeprom_read_block(&wypisz,16,4);
  Serial.println(wypisz, 5);

  Serial.print("CH6: ");
  eeprom_read_block(&wypisz,20,4);
  Serial.println(wypisz, 5);

  Serial.print("CH7: ");
  eeprom_read_block(&wypisz,24,4);
  Serial.println(wypisz, 5);
  
}

  float RX_freq_read(float QRG){
  float RXQRG=(QRG-10700)*100000ULL;
  //Serial.print("RX READ = ");
  //Serial.println(RXQRG);
  return RXQRG;
  }

  float TX_freq_read(float QRG){
  float TXQRG=(QRG/9)*100000ULL;
  //Serial.print("TX READ = ");
  //Serial.println(TXQRG);
  return TXQRG;
  }
  

int memory_read(){};

void memory_write(){

    // check if data is available
    while (Serial.available()==0){}
    // read the incoming byte:
    wybor = Serial.parseInt();

    // prints the received data
    if (wybor != 0)Serial.print("Edycja kanału : ");
    if (wybor != 0)Serial.println(wybor);

    // prints the received data
    if (wybor != 0)Serial.print("Częstotliwośc kanału : ");
    // check if data is available
    while (Serial.available()==1){} 

    // read the incoming byte:
    wypisz = Serial.parseFloat();
      if (wybor != 0)Serial.println(wypisz, 5);

    if (wybor == 1)eeprom_write_block(&wypisz,0,4);
    if (wybor == 2)eeprom_write_block(&wypisz,4,4);
    if (wybor == 3)eeprom_write_block(&wypisz,8,4);
    if (wybor == 4)eeprom_write_block(&wypisz,12,4);
    if (wybor == 5)eeprom_write_block(&wypisz,16,4);
    if (wybor == 6)eeprom_write_block(&wypisz,20,4);
    if (wybor == 7)eeprom_write_block(&wypisz,24,4);
    if (wybor >= 8)Serial.print("DUPA");
    }

int CH_read(){
  if (digitalRead(CH1) == LOW) return 1;
  if (digitalRead(CH2) == LOW) return 2;
  if (digitalRead(CH3) == LOW) return 3;
  if (digitalRead(CH4) == LOW) return 4;
  if (digitalRead(CH5) == LOW) return 5;
  if (digitalRead(CH6) == LOW) return 6;
  if (digitalRead(CH7) == LOW) return 7;
  if (digitalRead(CH8) == LOW) return 8;
  else CHANEL = 0;
  
   return CHANEL;
  };


void transmitting(){
  si5351.set_freq(TX_freq_read(QRG), SI5351_CLK1);
  si5351.output_enable(SI5351_CLK1, 1);
  //Serial.println("TRANSMITTING NOW");
  }
void receivinging(){
  si5351.set_freq(RX_freq_read(QRG), SI5351_CLK0);
  //si5351.set_freq(13395000000ULL, SI5351_CLK0);
  si5351.output_enable(SI5351_CLK1, 0);
  Serial.println("RECEIVING NOW");
  Serial.print("CHANEL=");
  Serial.println(CH_read());
  };


void loop()
{

  if (CH_read() == 1) QRG = 145350;
  if (CH_read() == 2) QRG = 145450;
  if (CH_read() == 3) QRG = 145550;
  if (CH_read() == 4) QRG = 145600;
  if (CH_read() == 5) QRG = 144500;
  if (CH_read() == 6) QRG = 145787.5;
  if (CH_read() == 7) QRG = 145500;
  if (CH_read() == 8) QRG = 144800;

  PTT = digitalRead(PTTpin);
  

  if (PTT == HIGH) {
    
    transmitting();
   }
  else {
    
    receivinging();
  }
  
  delay(300);
  memory_write();
}
