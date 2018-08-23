# Prejezd

Popis:
 - ovládání jednoho páru výstražníků
 - výstup: spínání dvou pinů, připojení k dalšímu obvodu
 - možnost připojit až 4 16-bit multiplexory (adresy se mění současně)
 - nastavení za běhu (serial monitor)
 
 Porty:
 - adresy multiplexor/ů: D3, D4, D5, D6
 - odečítání hodnoty (podle množství multiplexorů): A0, A1, A2, A3
 - ovládání blikače: D8, D9
 - spínání infra LED: D12
 - ovládání přejezdu (zapnuto/vypnuto): D2
 
 Terminál:
 - help:				                    zobrazení nápovědy
 - show:			              	      aktuální konfigurace spínacích napětí (defaultně 2,5 V)
 - sen 'X' ONvoltage 'Y':	      změní spínací napětí senzoru X na hodnotu Y
 - monitor on:			               zapne vypisování napětí na sériový monitor
 - monitor off:			              vypne vypisování napětí na sériový monitor

 
