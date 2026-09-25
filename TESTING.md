# Verifiche · BLE + IR e OTA

Aggiornamento del 25 settembre 2026. Il sorgente applicativo pubblico è identico a quello del progetto locale aggiornato. Nessun upload o collaudo fisico è stato eseguito durante questa pubblicazione.

## Compilazione

Configurazione: Espressif32 6.5.0, Arduino-ESP32 2.0.14, NimBLE-Arduino 1.4.3, IRremoteESP8266 2.9.0, ESP32 BLE Keyboard commit `b7aaf9bb711a04216e4417f1e2a6b0ee0eaeaf66`, tabella OTA `default.csv`.

La compilazione locale è stata bloccata prima della compilazione C++ dalla toolchain Intel GCC 8 (`Bad CPU type in executable`) sul Mac ARM. La precedente build solo BLE non valida il firmware aggiornato. Per verificare il codice aggiornato è stato aggiunto il workflow Linux [Firmware](https://github.com/spacecdr/Volumefy/actions/workflows/firmware.yml), che esegue `pio run -e esp32-c3-devkitm-1`.

**Build Linux superata** il 25 settembre 2026 sul commit `2c09415`: [esecuzione 36189901076](https://github.com/spacecdr/Volumefy/actions/runs/36189901076). RAM **46.028 byte / 327.680 (14,0%)**, flash **1.015.998 byte / 1.310.720 (77,5%)**, entro lo slot OTA. Un warning nel core Arduino esterno `esp32-hal-uart.c` (ritorno senza valore); nessun errore. Nessun upload hardware.

## Sito e anteprima

Il generatore `python3 tools/render_panel.py` compila un adattatore C++17 host ed esegue il vero `handleRoot()` con stato dimostrativo. Questo verifica il rendering HTML, non la compilazione per ESP32 o il backend.

Verifiche browser: filtri tipo/marca, ricerca OLED, ricerca senza risultati, modelli Philips, slider a 6×, intercettazione attivazione/test/salvataggio/OTA senza richieste hardware, immagini locali, ancore, assenza di overflow a 1440/390/320 pixel e assenza di errori JavaScript. Le schermate sono renderizzate con Chrome, senza dispositivo connesso.

Catalogo ricontato dal sorgente: **125 = 101 TV + 20 soundbar + 4 audio**. Conteggio e indici allineati in CATALOG.md, README e sito.

## Collaudo hardware da eseguire

| Prova | Criterio |
| --- | --- |
| Encoder | Verificare verso, rotazione lenta/rapida, rimbalzi e scatti persi durante invio IR o impulsi LED |
| Breve/lunga | Un mute al rilascio breve; ≥2 s commuta e riavvia senza mute aggiuntivo |
| BLE | Pairing, report volume/mute e riconnessione verificati su ogni host di interesse |
| Esclusione modalità | BLE con Wi-Fi spento; IR/AP senza BLE |
| AP e pannello | Accesso diretto, captive portal, selezione profilo, test, preferiti e velocità |
| IR | VOL+/VOL−/Mute sui dispositivi reali, con stato del catalogo annotato |
| Philips RC6 | HTL3140B/HTL2163B: comandi e toggle fra pressioni, repeat coerenti |
| Velocità | Risposta 1×–6× e singolo mute; minimo SIRC preservato |
| NVS | Modalità, profilo, preferiti e velocità recuperati dopo riavvio, spegnimento e sleep |
| Sleep/wake | Timeout120s, SW risveglia, rilascio prima di nuovi comandi |
| LED | BLE ricerca1s, connesso15s, IR5s, impulsi30ms dove previsti |
| OTA | Firmware compatibile installato, esito e riavvio; timeout sospeso; errore/abort gestiti |
| Avvio | Reset e accensione in diverse posizioni encoder; verifica strapping GPIO2 |
| Elettrica | Pull-up3,3V, stadio IR, corrente scheda e portata misurati |

Lo stato “Verificato” nel catalogo riguarda le fonti/codici secondo il firmware e non sostituisce queste prove.
