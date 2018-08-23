/* Ovladani prejezdu 
 * 
 */
//Knihovny 
#include <EEPROM.h>

 
//Multiplexor
#define mux_number 2    //Počet obsluhovaných multiplexorů (lze měnit 1-4)
int mux_addr_pin [] = {3, 4, 5, 6} //piny D3, D4, D5, D6 - změna adresy 
int mux_analog_pin [] = {14, 15, 16, 17} //piny A0, A1, A2, A3 - čtení hodnoty ze senzoru

//Ovladač přejezdu 
byte ir_led = 12; //pin D12 - spíná IR led na senzoru
byte stat_pin = 2; //pin D2 Ovládání zapnutí/vypnutí přejezdu
byte red_led = 8; //pin D8, sepnutí červených LED
byte white_led = 9; //pin D9, sepnutí bílých LED

//eeprom
#define first_byte_addr 10 //Adresa prvního bytu s uloženými hodnotami v eeprom
 
bool obsazeno = false; //Pokud je přejezd aktivní - je na něm vlak, blikají červená světla, pak true
bool prejezd_status = false; //zapnutí/vypnutí přejezdu


//Funkce
void kontroluj_obsazeni (void);

//Třídy
class sensor{
  private:
    double on_voltage = 2,5; //Napětí, při kterém se sepne
    int eeprom_addr = 0; //Adresa napětí v eeprom
    byte mux_addr[] = {0,0,0,0} //kombinace adres na multiplexoru (D3, D4, D5, D6)
    byte analog_pin = 0; //hodnota pinu, ze kterého se čte
    bool stat = false; //zda je detekováno vozidlo nbo ne

  public:
    sensor();
    sensor(int eepr, byte* mux_pins, byte readpin){
      eeprom_addr = eepr;
      for(int i, i < 4, i++){
        mux_addr[i] = mux_pins[i];  
      }
      analog_pin = readpin;
    }
}
void setup() {
  Serial.begin(9600); //Příprava seériové linky
  Serial.println("Sprava zeleznicniho prejezdu");
  Serial.println("Nacitam data z interni eeprom");

}

void loop() {
  // put your main code here, to run repeatedly:

}
