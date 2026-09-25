# Verifiche

Stato: **firmware recuperato e compilazione verificata il 25 settembre 2026; nessun collaudo hardware eseguito durante l'integrazione**.

Verifica: `pio run -e esp32-c3-devkitm-1` da copia di lavoro del repository, senza artefatti di compilazione preesistenti. Espressif32 6.5.0, Arduino-ESP32 2.0.14, toolchain RISC-V 8.4.0+2021r2-patch5, NimBLE-Arduino 1.4.3, ESP32 BLE Keyboard commit `b7aaf9bb711a04216e4417f1e2a6b0ee0eaeaf66`. Il dettaglio delle dipendenze dirette è fissato in `platformio.ini`. I tool di piattaforma erano già installati: non è una verifica di installazione su macchina nuova.

Build riuscita: RAM 23.532 byte (7,2%), flash 511.734 byte (39,0%). Presente un warning nel core Arduino esterno (`esp32-hal-uart.c`, ritorno senza valore in `uartSetPins`); nessun errore di compilazione.

Quella che segue distingue la compilazione dalle prove ancora da eseguire sul dispositivo. I criteri seguono il firmware recuperato; le proposte della documentazione iniziale non sono considerate funzioni obbligatorie mancanti.

| Prova | Criterio di accettazione | Stato |
| --- | --- | --- |
| Compilazione | Dipendenze dirette fissate; build senza artefatti preesistenti | Superata |
| Encoder lento/veloce | Conteggio coerente per scatto nei due versi, nessuna inversione spuria | Da eseguire |
| Pulsante | Un solo mute per pressione, nessuna ripetizione se tenuto | Da eseguire |
| Pairing nuovo | Nome Volumefy; nessun codice; HID riconosciuto dall'host | Da eseguire |
| Volume/mute | Ricezione nativa dei report e risposta del sistema operativo | Da eseguire |
| LED discovery | Cambio di stato ogni 1 s, ciclo completo di 2 s | Da eseguire |
| LED collegato | Un impulso di 30 ms ogni 15 s | Da eseguire |
| Inattività | Sleep dopo 120 s senza rotazione o pressione, anche sotto USB | Da eseguire |
| Wake | SW risveglia; registrare eventuale mute alla prima pressione | Da eseguire |
| Riconnessione | Bond conservato; host precedente può riconnettersi | Da eseguire |
| Nuovo host | Dopo assenza del precedente, nuovo pairing possibile | Da eseguire |
| Reset e strapping | Avvio con encoder nelle diverse posizioni, pulsante premuto/rilasciato | Da eseguire |
| Consumo | Corrente dell'intera scheda misurata in discovery, connessione e sleep | Da eseguire |
| Sistemi operativi | macOS, Windows, Linux e dispositivi mobili verificati separatamente | Da eseguire |

Una simulazione di eventi non prova i rimbalzi del contatto fisico. Una connessione GATT non prova la ricezione dei comandi multimediali. Registrare separatamente test automatici, osservazioni hardware e comportamento dell'host.
