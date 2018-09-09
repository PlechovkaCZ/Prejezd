/* Ovladani prejezdu 
 * 
 */
//Knihovny 
#include <EEPROM.h>

 
//Multiplexor
#define mux_number 1    //Počet obsluhovaných multiplexorů (lze měnit 1-4)
#define mux_inputs 16  // Počet vstupů na jednom multiplexoru !!!NEMĚNIT!!!
byte mux_addr_pin [] = {3, 4, 5, 6}; //piny D3, D4, D5, D6 - změna adresy 
byte mux_analog_pin [] = {14, 15, 16, 17}; //piny A0, A1, A2, A3 - čtení hodnoty ze senzoru

//Ovladač přejezdu 
byte ir_led = 12; //pin D12 - spíná IR led na senzoru
byte stat_pin = 2; //pin D2 Ovládání zapnutí/vypnutí přejezdu
byte red_led = 8; //pin D8, sepnutí červených LED
byte white_led = 9; //pin D9, sepnutí bílých LED

//eeprom
#define first_byte_addr 10 //Adresa prvního bytu s uloženými hodnotami v eeprom

//Proměnné pro práci přejezdu
#define info_interval 10000 //Jak čaato se má zobrazovat zpráva o stavu (aktivaci) přejezdu
bool obsazeno = false; //Pokud je přejezd aktivní - je na něm vlak, blikají červená světla, pak true
bool prejezd_status = false; //zapnutí/vypnutí přejezdu NEPOUŽITO
bool write_voltage = true; //Zda se má vypisovat napětí ze senzorů na sériový monitor
unsigned long odpocet1 = 0;

//Proměnné pro funkci příkazové řádky - terminál
#define max_buffik 40 //Maximální počet znaků v buffiku
String buffik = ""; //Zde se bude ukládat příkaz ze sériového monitoru


//Prototypy
void kontroluj_obsazeni (void); //Když je přejezd zapnutý, pravidelně kontroluje obsazení úseků
//void stop_prejezd(void); //Při vypnutí přejezdu okamžitě zastaví jeho funkci
void eeprom_zapis_napeti(int epr_addr, int data); //Zapíše spínací napětí do eeprom (adresa (první), napetí [mV])
int eeprom_precti_napeti(int epr_addr); //Přečte napětí z eeprom (adres(první))
void print_WC(void); //Vypíše na seriovou linku text
void zpracuj_buffik(void); //Zpracuje příkaz z buffiku (sériová linka)
vypis_oddelovac(byte, byte); //Vypíše řádku několika znaků, znaky: 1. argumentv - ASCII, počet: 2. argument

//Třídy
class sensor{
  private:
    int on_voltage = 2500; //Napětí, při kterém se sepne [mV]
    int eeprom_addr = 0; //Adresa napětí v eeprom
    bool mux_addr [4] = {1, 1, 1, 1}; //kombinace adres na multiplexoru (D3, D4, D5, D6)
    byte analog_pin = 0; //hodnota pinu, ze kterého se čte
    bool last_stat = false; //zda bylo detekována vozidlo při předchozím měření
    bool stat = false; //zda je detekováno vozidlo nebo ne

  public:
    sensor();
    sensor(int eepr, bool* mux_pins, byte readpin){
      eeprom_addr = eepr;
      for(int i; i < 4; i++){
        mux_addr[i] = mux_pins[i];  
      }
      analog_pin = readpin;
      /* DEBUG */
      Serial.println("Volán konstruktor sensor");
      Serial.print("eepr_addr: "); Serial.print(eepr); Serial.print("; analog_pin: "); Serial.print(readpin); Serial.print("; mux_addr_seq: ");
      for(int i; i < 4; i++){
        Serial.print(mux_addr[i]); 
      }
      Serial.println("");
    }
    
    bool get_last_stat(){ //Vrátí poslední stat senzoru
      return last_stat;
    }

    byte get_analog_pin(void){ //Vrátí pin, na kterém se má měřit napětí
      return analog_pin;
    }

    int print_ONvoltage(void){ //Vrátí hodnotu spínacího napětí
      return on_voltage;
    }
    
    int get_eeprom_addr (void); //Vrátí adresu prvního bytu napětí v eeprom
    void get_ONvoltage (void); //Přečte napětí z eeprom a změní ho
    void change_ONvoltage (int); //Změní spínací napětí na danou hodnotu (v mV)
    void set_mux (void); //Nastaví adresu mux podle nastavení adresy v proměnné mux_addr
};

int sensor::get_eeprom_addr(){
  return eeprom_addr;
}

void sensor::get_ONvoltage(){
  int vracene_napeti = eeprom_precti_napeti(eeprom_addr);
  if((vracene_napeti <=5000) &&(vracene_napeti > 0)){ //Pokud je hodnota v eeprom ve správné rozsahu, bude použita
    on_voltage = vracene_napeti;
  }
  /*DEBUG*/
  Serial.print("Spíací napětí: "); Serial.println(on_voltage); 
}

void sensor::change_ONvoltage(int nove){
  if((nove <=5000) &&(nove > 0)){
    eeprom_zapis_napeti(eeprom_addr, nove);
    on_voltage = nove;
  }
  else {
    Serial.println("CHYBA!!! Napeti ma rozsah 0 - 5000 mV");
  }
}

void sensor::set_mux(void){
  for(int i; i < 4; i++){
    digitalWrite(mux_addr_pin[i], mux_addr[i]);
  }
}

//Nastavení senzorů (pole)
  sensor *cidlo = (sensor*)malloc((mux_number*mux_inputs)*sizeof(sensor));

  
void setup() {
  Serial.begin(9600); //Příprava seériové linky
  buffik.reserve(max_buffik);
  Serial.println("Sprava zeleznicniho prejezdu");
  Serial.println("Nacitam data z interni eeprom");
  //Nastavení výstupů a vstupů
  pinMode(ir_led, OUTPUT);
  pinMode(red_led, OUTPUT);
  pinMode(white_led, OUTPUT);
  pinMode(stat_pin, INPUT);
  for(int i; i<4; i++){
    pinMode(mux_addr_pin[i], OUTPUT);
  }
  //Interupt pro spínání přejezdu
  attachInterrupt(digitalPinToInterrupt(stat_pin), kontroluj_obsazeni, RISING); //Zapnutí
  //attachInterrupt(digitalPinToInterrupt(stat_pin), stop_prejezd, FALLING); //Vypnutí
//  Tvorba objektů senzorů (cidel)
  for (int i; i < mux_number*mux_inputs; i++){ 
    byte analog_index = i/mux_inputs;
    byte zbytek_deleni = i % mux_inputs;
    bool adresa [4];
    adresa [0] = zbytek_deleni & 0x1;
    adresa [1] = zbytek_deleni & 0x2;
    adresa [2] = zbytek_deleni & 0x4;
    adresa [3] = zbytek_deleni & 0x8;
    cidlo[i] = sensor(first_byte_addr+(2*i), adresa, mux_analog_pin[analog_index]); 
  }
  //Čtení spínacího napětí z eeprom
  for (int i; i < mux_number*mux_inputs; i++){ 
    cidlo[i].get_ONvoltage();
  }
}

void loop() {
  kontroluj_obsazeni();
  if(millis() - odpocet1 >= info_interval){
    Serial.println("Prejezd je deaktivovany");
    odpocet1 = millis();
  }
 delay(100);
}
// FUNKCE
//------------------------------------------------------------------------------------------------------
//>>>>> Funkce pro deaktivaci přejezdu <<<<<
  /*  Zastavení dekodéru
   *    - Funkce pomocí goto vrátí program do hlavní smyčky
   */
void stop_prejezd(void){
  Serial.println("Deaktivuji prejezd");
  while(digitalRead(stat_pin) == LOW){
    Serial.println("Prejezd je deaktivovany");
    delay(10000);
  }
}
//------------------------------------------------------------------------------------------------------

//>>>>> Funkce pro aktivaci přejezdu <<<<<
  /*  Aktivní přejezd
   *    - Dokud je stat_pin aktivní (HIGH) projíždí všechny senzory (přepíná multiplexor)
   *    - Pokud je dvakrát naměřena stejná hodnota (dekodér je zastíněn/nezastíněn), přepne se celý stav přejezdu na obsazeno (bílá LED)/volno (červená LED)
   */
void kontroluj_obsazeni(void){
  int cidlo_index = 0;
  int merene_napeti = 0; //Napětí změřené s zepnutou IR LED
  int napeti_sum = 0; //Napětí bez IR LED (šum)
  int napeti = 0; //Napětí na senzoru po odečtení šumu
  while(digitalRead(stat_pin) == HIGH){ //Pokud je ovládací pin sepnutý 
    if(millis() - odpocet1 >= info_interval){
      Serial.println("Prejezd je deaktivovany");
      odpocet1 = millis();
    }
    if (mux_number*mux_inputs <= cidlo_index){
      cidlo_index = 0;
    }
    digitalWrite(ir_led, HIGH); //Zapne IR LED
    cidlo[cidlo_index].set_mux(); //Nastaví adresu na multiplxoru
    delay(5);
    merene_napeti = analogRead(cidlo[cidlo_index].get_analog_pin()) *5000.0/1023.0; //Odečte napětí
    digitalWrite(ir_led, LOW); //Vypne IR LED
    delay(5);
    napeti_sum = analogRead(cidlo[cidlo_index].get_analog_pin()) *5000.0/1023.0; //Odečte šumové napětí
    napeti = merene_napeti - napeti_sum; //Odečte šum
    if(write_voltage){
      Serial.print("Napeti na senzoru ");
      Serial.print(cidlo_index + 1);
      Serial.print(" je: ");
      Serial.print(napeti);
      Serial.println(" mV");
    }

   cidlo_index++; 
  }
}
//------------------------------------------------------------------------------------------------------

//>>>>> Funkce pro zápis hdnoty napětí [mV] do eeprom <<<<<
  /*  Zápis
   *    - Rozdělí integerovou hodnotu napětí do dvou byte hodnot
   *    - Každou zapíše (těsně za sebou)
   */
void eeprom_zapis_napeti(int epr_addr, int data){
  byte data1 = (int)(data);
  byte data2 = ((int)(data) >> 8); //Rozdělení adresy do dvou bajtů
    //Zápis po znaku
  delay(5);
  EEPROM.write (epr_addr, data1);
  delay(10);
  EEPROM.write ((epr_addr+1), data2);
  delay(5);
}
//------------------------------------------------------------------------------------------------------

//>>>>> Funkce pro přečtení napětí z eeprom <<<<<
  /*  Čtení napětí
   *    - Přečte napětí (rozděleno do dvou byte hodnot)
   *    - Složí a vrátí napětí v mV jako integer
   */
int eeprom_precti_napeti(int epr_addr){
  byte cteni_adresa[] = {0, 0};
  int cteni_addr = 0;
  delay(5);
  cteni_adresa[0] =  EEPROM.read(epr_addr);
  cteni_adresa[1] =  EEPROM.read(epr_addr+1);
  cteni_addr = (cteni_adresa[0] + (cteni_adresa[1] << 8));
  return cteni_addr;
}
//------------------------------------------------------------------------------------------------------
//>>>>> Funkce pro zpracovani příkazů <<<<<
  /*  Zpracování příkazů
   *    - Pokud je na sériové lince co číst, uloží se to do stringu 'buffik'
   *    - Pokud je zaznamenána prázdná řádka, buffik se vyhodnotí
   *    - Pokud je zadán delší řetězec než rezervovaný počet znaků, vypíše se chyba
   */
void serialEvent(){
  char novy_char = "";
  while(Serial.available()){
    if(buffik.length() >= max_buffik){
      print_WC();
    }
    novy_char = (char)Serial.read();
    buffik += novy_char;
    if(novy_char == '\n'){
      void zpracuj_buffik();
    }
  }
}
//------------------------------------------------------------------------------------------------------
//>>>>> Funkce pro vypsání špatného příkazu <<<<<
  /*  Vypíše chybové hlášení
   *    - Vypíše na sériovou linku zadaný text
   */
void print_WC(){
  Serial.println("Neplatny prikaz !");
  buffik = "";
}
//------------------------------------------------------------------------------------------------------
//>>>>> Funkce pro zpracování příkazů z buffiku <<<<<
  /*  Vypíše chybové hlášení
   *    - Načte buffik a podle toho vykoná příkaz
   *    - 'help': Vypíše seznam příkazů a jejich popis
   *    - 'show': Vypíše nastavení spínacích napětí (ukládají e do eeprom, defaultně 2500 mV)
   *    - 'set X ONvoltage Y': změní spínací napětí senzoru X na hodnotu Y [mV], ukládá se do eeprom
   *    - 'monitor on': zapne vypisopvání naměřených hodnot ze senzoru na sériový monitor
   *    - 'monitor off': vypne vypisopvání naměřených hodnot ze senzoru na sériový monitor
   */
void zpracuj_buffik(){
  byte buffik_index = 0;
  //--- Příkaz HELP ---
    if(buffik[0] == 'h' && buffik[1] == 'e' && buffik[2] == 'l' && buffik [3] == 'p' && buffik [4] == 10){
      Serial.println("Tady bude HELP");
      buffik = "";
      buffik_index = 0;
    //--- Příkaz SHOW ---
    else if(buffik[0] == 's' && buffik[1] == 'h' && buffik[2] == 'o' && buffik [3] == 'w' && buffik [4] == 10){
      Serial.println("vykonavam show");
      vypis_oddelovac(75,30);
      //Serial.println("\n Cislo senzoru   spinaci napeti");
      for(int i; i <  mux_number*mux_inputs; i++){
        Serial.print("Senzor ");
        Serial.print(i+1);
        Serial.print(": ");
        Serial.print(cidlo[i].print_ONvoltage());
        Serial.println(" mV");     
      }
      buffik = "";
      buffik_index = 0;
    }
    else{
      print_WC();
    }
}
//------------------------------------------------------------------------------------------------------
//>>>>> Funkce na výpis oddělovače <<<<<
  /*  Princip
   *    - podle indexu (1. argument) vypíše počet oddělovačů (2. argument)
   *    - index je hodnota v ASCII tabulce
  */

void vypis_oddelovac(byte index, byte pocet){
  for(int i = 0; i < pocet; i++){
    Serial.print((char)index);
  }
}
//------------------------------------------------------------------------------------------------------

