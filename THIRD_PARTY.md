# Fonti e immagini

- Foto KY-040: [Joy-IT, COM-KY040RE](https://www.joy-it.net/en/products/COM-KY040RE), [immagine originale](https://www.joy-it.net/files/files/Produkte/COM-KY040RE/KY-040-2.png). Il sito la carica dal server Joy-IT; nessuna copia è inclusa nel repository. Diritti del rispettivo titolare. Mostra un componente di riferimento, non il prototipo dell'autore, e non certifica la variante KY-040 posseduta. Le specifiche del prodotto Joy-IT non sono automaticamente trasferibili al modulo del progetto.
- `docs/images/wiring.svg`: schema funzionale originale basato sul cablaggio fornito dall'autore. Posizioni e dimensioni non riproducono il pinout fisico delle schede.
- Manopola nella pagina iniziale: illustrazione CSS, non foto o rendering di un prodotto realizzato.
- [ESP32-C3 datasheet](https://documentation.espressif.com/esp32-c3_datasheet_en.html): capacità del chip e strapping.
- [USB Serial/JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html): limiti della periferica USB.
- [Sleep modes](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/system/sleep_modes.html): sonno, radio e sorgenti di risveglio.

## Dipendenze firmware

Le librerie sono scaricate da PlatformIO, non copiate nel repository:

- [ESP32 BLE Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard/tree/b7aaf9bb711a04216e4417f1e2a6b0ee0eaeaf66), backend BLE HID.
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino/tree/1.4.3), stack BLE.
- [Espressif32 per PlatformIO](https://github.com/platformio/platform-espressif32/tree/v6.5.0), piattaforma di compilazione Arduino-ESP32.

Per licenze e attribuzioni delle dipendenze consultare i rispettivi repository.
Non sono inclusi log, backup flash o dati di associazione. Nessuna licenza di riutilizzo è stata scelta per i materiali originali.

## Aggiornamento BLE + IR

- [IRremoteESP8266 2.9.0](https://github.com/crankyoldgit/IRremoteESP8266/tree/v2.9.0): codifica e invio IR, dipendenza gestita da PlatformIO.
- `reference/Philips_HTL2163_Soundbar.ir`: cattura già presente nel progetto locale fornito dall'autore; usata dai profili Philips RC6. Non viene attribuita una licenza aggiuntiva alla cattura.
- `docs/images/panel-desktop.png` e `panel-ota.png`: schermate del pannello HTML generato dal sorgente `handleRoot()` attraverso `tools/render_panel.py`, renderizzate in Chromium con stato dimostrativo (Philips HTL3140B, due preferiti, velocità 2×). Non sono fotografie né prove di connessione a un ESP32.
- `docs/panel.html`: stessa interfaccia con nota di anteprima e blocco delle azioni verso il dispositivo. Non carica firmware e non invia comandi IR.
- `docs/images/wiring.svg`: schema originale aggiornato con GPIO5, stadio di pilotaggio e uscita IR. È funzionale, non un circuito dimensionato o un pinout fisico.
- La fotografia KY-040 resta caricata dal server Joy-IT con attribuzione, come nella pubblicazione precedente. Non sono state create foto sintetiche del prototipo.
