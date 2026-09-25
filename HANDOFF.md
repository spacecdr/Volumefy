# Continuità Volumefy

## Stato attuale · 25 settembre 2026

Il repository e il sito sono aggiornati alla versione locale **BLE + IR con RC6 Philips e OTA web**. `src/main.cpp` è identico al sorgente del progetto del dispositivo. La precedente documentazione solo BLE è superata e rimane nella cronologia Git.

Progetto del dispositivo: `/Users/iceman/Documents/PlatformIO/Projects/Volumefy`, rinominato su richiesta dell'autore da `BT and IR Trasmitter`. Tradurre anche i sottopercorsi storici. Workspace VS Code: `Volumefy.code-workspace`. Prima di riprendere leggere `/Users/iceman/Documents/PROGETTI.md`.

Copia pubblica locale: `/Users/iceman/Documents/GitHub/Volumefy`. Repository https://github.com/spacecdr/Volumefy; GitHub Pages pubblica `main:/docs` su https://spacecdr.github.io/Volumefy/. La preparazione dell'aggiornamento è stata fatta in `/private/tmp/volumefy-publish`.

## Riferimenti autorevoli

- Firmware locale e pubblico: encoder CLK2, DT3, SW4; IR5; LED8 LOW. Rotazione volume, breve mute al rilascio, lunga ≥2 s cambio esclusivo BLE/IR con riavvio.
- Sleep120s, wakeSW, rilascio richiesto prima di nuovi comandi. BLE nome Volumefy; IR con AP Volumefy-Setup, password predefinita 12345678, HTTP 192.168.4.1.
- 125 profili, 101 TV / 20 soundbar / 4 audio. README e CATALOG locali precedenti riportavano ancora 124: usare il conteggio dei sorgenti aggiornati.
- Philips HTL3140B e HTL2163B/12: RC6 Mode 0, address 0x10. Cattura in reference/.
- NVS: modalità, profilo, preferiti e velocità. OTA solo IR/AP; migrazione dal vecchio firmware BLE tramite USB.

## Sito e immagini

Testi italiani, pagina di presentazione, specifiche, cablaggio BLE+IR, guida al pannello, catalogo e OTA. `tools/render_panel.py` estrae il C++ reale per generare `docs/panel.html`; richiede Python 3 e compilatore C++17. L'adattatore usa dati dimostrativi e intercetta richieste e upload. Le schermate Chromium non costituiscono collaudo hardware.

Foto componente Joy-IT attribuita e caricata dal server originale. Nessuna foto originale del prototipo disponibile. Manopola CSS e schema SVG dichiarati come illustrazioni. Nessuna licenza scelta.

## Verifica e prossimi passi

Consultare TESTING.md per esiti reali. La toolchain PlatformIO locale GCC 8 è un eseguibile Intel che non parte su questo Mac ARM senza traduzione: non confondere questo errore con un errore C++ del progetto. La compilazione pubblica è verificata dal workflow Linux `.github/workflows/firmware.yml`. Nessun firmware è stato caricato sul dispositivo durante la pubblicazione. Restano da collaudare BLE, IR, modalità, memoria, sleep, wake e OTA sul dispositivo.
