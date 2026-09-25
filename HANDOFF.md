# Continuità

Pubblicazione iniziale del 25 settembre 2026, richiesta nello stesso formato di AirMouse: repository pubblico e sito GitHub Pages.

Non sono stati trovati sorgenti Volumefy o riferimenti KY-040 nei progetti locali Arduino/GitHub. La conversazione definisce il cablaggio e i requisiti, senza prove di firmware realizzato. Per questo la pubblicazione è dichiarata specifica, senza codice inventato o risultati di collaudo attribuiti al dispositivo.

Nome: Volumefy. ESP32-C3 SuperMini, KY-040: CLK2, DT3, SW4, alimentazione3,3V. Volume/mute BLE, LED1s/15s, sleep120s, wakeSW, bonding senza PIN e possibilità di nuovo host. Il C3 non supporta HID sulla USB Serial/JTAG integrata; un ponte seriale sarebbe un'estensione separata.

Foto del componente caricata dal sito Joy-IT con attribuzione; nessuna foto originale del prototipo disponibile. Illustrazione CSS e schema SVG esplicitamente dichiarati. Nessuna licenza scelta, in continuità con AirMouse.

## Integrazione del progetto locale — 25 settembre 2026

Successivamente alla pubblicazione iniziale sono stati individuati i sorgenti nella cartella PlatformIO `Volumefy`, esterna ai percorsi esaminati in precedenza. `src/main.cpp` e `platformio.ini` sono ora riuniti alla documentazione e al sito del repository originale, preservandone la cronologia.

La logica applicativa recuperata è invariata; eliminata soltanto la definizione duplicata di `USE_NIMBLE` dal sorgente e fissate le dipendenze alle versioni presenti nell'ambiente locale. Build verificata; nessun upload o test hardware effettuato. README, descrizione tecnica e sito distinguono ora implementazione e requisiti. La precedente dichiarazione di assenza dei sorgenti descriveva la ricerca iniziale ed è superata da questo recupero.

Il firmware locale è la fonte autorevole per il comportamento corrente. La specifica precedente è stata redatta senza questi sorgenti: filtro encoder e variazioni LED/sleep sono soltanto proposte, non lavori richiesti. Prossimo lavoro: collaudare il firmware e valutare eventuali modifiche solo se necessarie. Non usare il piano TESTING.md come prova di test hardware superati.
