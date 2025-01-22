# ProjektR
Sve potrebno za izvrsavanje ProjektR-a

Projekt R vizualizira sinkronizacijske probleme koristeći ESP32 NodeMCU modul, FreeRTOS i Espressif IDF okruženje. 
Cilj projekta je demonstrirati rješenja problema "Sleeping Barber" i "Dining Philosophers" uz korištenje različitih funkcionalnosti FreeRTOS-a.


Lijevi gumb: Omogućuje navigaciju kroz glavni izbornik na LCD zaslonu. Korisnik može pregledati dostupne programe i odabrati željeni problem.
Desni gumb: Pokreće odabrani program. Tijekom rada programa moguće je pratiti stanje sustava na LCD zaslonu.
Ponovni pritisak na desni gumb: Prekida trenutni program i vraća korisnika u glavni izbornik.
Interakcija putem gumba implementirana je korištenjem Interrupt Service Routine (ISR) funkcionalnosti, omogućujući brzo i pouzdano prepoznavanje korisničkih akcija.


Sleeping Barber Problem
Riješen korištenjem FreeRTOS Queue-a.
Queue se koristi za sinkronizaciju između klijenata (koji dolaze u frizerski salon) i frizera (koji pruža uslugu). Klijenti se dodaju u red čekanja, dok frizer obrađuje jednog po jednog klijenta.
Vizualizacija:
LED diode prikazuju trenutno stanje:
Crvena LED: Frizer je zauzet.
Zelena LED: Klijent je uslužen.
Plava LED: Čekaonica je puna.
Stanje se ispisuje i na LCD zaslonu.

Dining Philosophers Problem
Riješen korištenjem kombinacije FreeRTOS Mutex-a i Semaphore-a.
Mutex-i se koriste za upravljanje vilicama (resursima) između filozofa, dok semafori kontroliraju pristup kritičnim dijelovima koda kako bi se spriječili zastoji i međusobna blokada.
Vizualizacija:
LED diode prikazuju aktivnosti filozofa:
Uključene filozofi jedu
Isključene filozofi razmisljaju
LCD zaslon prikazuje trenutna stanja svakog filozofa.

Stanje sustava i ključne poruke prikazuju se na 16x1 LCD zaslonu.
Implementacija ispisa koristi FreeRTOS task notifications, čime se osigurava pravovremeno ažuriranje informacija bez dodatnog opterećenja glavnih zadataka.

Gumbi na sklopu povezani su s GPIO pinovima ESP32 modula i koriste Interrupt Service Routine (ISR) za brzo prepoznavanje korisničkih akcija.
ISR omogućuje učinkovitu obradu prekida bez narušavanja rada ostalih zadataka u sustavu.
Tehnički detalji
Hardver:
ESP32 NodeMCU modul: Dvojezgreni procesor, 512 kB SRAM-a i WiFi/Bluetooth povezivost.
LCD zaslon (16x1): Za prikaz stanja sustava i poruka.
LED diode: Vizualna indikacija stanja sustava.
Gumbi: Za navigaciju i interakciju s izbornikom.
Otpornici i breadboard: Za integraciju svih komponenti.
Softver:
Razvojna okolina: Espressif IDF.
Operativni sustav: FreeRTOS.
Funkcionalnosti: Taskovi, Queue, Mutex-i, Semaphore-i, task notifications i ISR.


CircuitDiagram.jpg
![Alt text](CircuitDiagram.jpg?raw=true "Circuit Diagram")
