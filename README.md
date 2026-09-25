# Volumefy

Una manopola per il volume. Un click per il silenzio.

Progetto di controller Bluetooth con **ESP32-C3 SuperMini** e **KY-040**, pensato per comandare volume e mute del dispositivo collegato tramite BLE HID.

**Stato: firmware disponibile e compilazione verificata; collaudo hardware da eseguire.** Il codice recuperato dal progetto locale PlatformIO è in `src/main.cpp`. Documentazione, cablaggio e sito sono riuniti nello stesso repository. Il firmware locale è il riferimento per il comportamento attuale; le proposte della prima pubblicazione sono conservate come possibili evoluzioni in [TECHNICAL.md](TECHNICAL.md).

[Sito del progetto](https://spacecdr.github.io/Volumefy/) · [Descrizione tecnica](TECHNICAL.md) · [Piano di verifica](TESTING.md) · [Fonti e immagini](THIRD_PARTY.md)

![Schema funzionale dei collegamenti Volumefy](docs/images/wiring.svg)

## Funzionamento del firmware

| Azione o stato | Comportamento nel codice / verifica |
| --- | --- |
| Ruota la manopola | Aumenta o diminuisce il volume |
| Premi la manopola | Mute / unmute |
| Associazione Bluetooth | Nome **Volumefy**; pairing gestito dalle librerie, da collaudare |
| Disponibile per la connessione | LED cambia stato ogni secondo (ciclo completo di 2 s) |
| Connesso | Impulso LED di 30 ms ogni 15 secondi |
| 120 secondi senza uso | Sleep con radio spenta |
| Premi durante lo sleep | Risveglio e nuova disponibilità Bluetooth |
| Dispositivo precedente assente | Associazione gestita dalle librerie; da verificare con più host |

La riconnessione BLE è avviata dal computer o telefono: la conservazione delle chiavi e la disponibilità sono affidate alle librerie BLE, ma non può obbligare l'host a riconnettersi. La compatibilità effettiva con ciascun sistema operativo deve essere verificata.

## Hardware e cablaggio

| Componente | Quantità | Ruolo |
| --- | --- | --- |
| ESP32-C3 SuperMini | 1 | Microcontrollore e radio Bluetooth LE |
| KY-040 con pulsante | 1 | Encoder incrementale e mute |
| Cavetti | 5 | Alimentazione e segnali |
| Cavo USB-C | 1 | Alimentazione e programmazione |

| Pin KY-040 | Pin ESP32-C3 |
| --- | --- |
| CLK | GPIO 2 |
| DT | GPIO 3 |
| SW | GPIO 4 |
| + | 3,3 V |
| GND | GND |

Questo è il cablaggio comunicato per il prototipo. I segnali devono rimanere a 3,3 V. Verificare le resistenze di pull-up del modulo concreto, incluso SW: le varianti KY-040 non sono tutte identiche.

GPIO2 è un pin di strapping. Una resistenza di pull-up non impedisce al contatto dell'encoder di portarlo basso: l'avvio va provato nelle diverse posizioni della manopola, inclusi reset e risveglio. Non considerare l'avvio garantito dal solo cablaggio. Il pin e la polarità del LED controllabile della SuperMini devono essere verificati sull'esemplare; il LED di alimentazione potrebbe non essere controllabile dal firmware.

## Uso via USB

La porta USB integrata dell'ESP32-C3 è **Serial/JTAG a funzione fissa**, non USB HID programmabile. Il solo cavo non consente quindi il funzionamento diretto come tastiera multimediale USB. [Documentazione Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html).

Una possibile estensione è un ponte seriale sul computer che riceva gli eventi e regoli il volume, con implementazione specifica per sistema operativo. Questo ponte non è incluso. Per HID USB nativo insieme a BLE occorrerebbe invece rivedere l'hardware, per esempio usando un ESP32-S3.

## Foto e stato dei lavori

Il sito include una foto del KY-040 fornita da Joy-IT, caricata dalla fonte originale e attribuita. È un'immagine di riferimento del componente, **non una foto del prototipo Volumefy**. Schema e rappresentazione della manopola sono illustrazioni. Non sono disponibili foto originali del montaggio in questa pubblicazione.

Prossimi passi: verificare encoder e LED, provare pairing e riconnessione, misurare sleep e risveglio. Il [piano di verifica](TESTING.md) descrive le prove necessarie prima di dichiarare una release funzionante.

## Compilazione e caricamento

Apri la cartella del repository con PlatformIO oppure, con PlatformIO Core installato, esegui:

```sh
pio run -e esp32-c3-devkitm-1
pio run -e esp32-c3-devkitm-1 -t upload
```

Il primo comando compila; il secondo carica il firmware sulla scheda collegata. Se la porta non viene individuata automaticamente, aggiungi `--upload-port PORTA` al comando di caricamento. L'upload e il funzionamento sul prototipo non sono stati verificati durante questa integrazione.

`platformio.ini` conserva il profilo `esp32-c3-devkitm-1` usato per la SuperMini nel progetto locale e fissa le versioni usate per la verifica: Espressif32 6.5.0, NimBLE-Arduino 1.4.3 e ESP32 BLE Keyboard al commit `b7aaf9bb711a04216e4417f1e2a6b0ee0eaeaf66`. `USE_NIMBLE` è definito nei flag per tutti i sorgenti.

## Cosa fa il codice attuale

- Legge CLK su GPIO2 e DT su GPIO3; a ogni fronte di discesa di CLK invia volume su o giù se BLE è connesso. Non usa una macchina a stati antirimbalzo.
- Legge SW su GPIO4 e invia mute alla pressione, con un intervallo minimo di 200 ms tra gli eventi accettati.
- Usa il LED su GPIO8, attivo basso: scollegato cambia stato ogni secondo (ciclo acceso/spento di 2 s); collegato genera un impulso di 30 ms ogni 15 s.
- Entra in deep sleep dopo 120 s senza eventi accettati di rotazione o pressione e configura il risveglio quando SW è basso. Il rilascio prima dello sleep e il consumo della prima pressione al risveglio non sono gestiti esplicitamente.
- Annuncia `Volumefy`; associazione e riconnessione sono affidate alle librerie BLE. Il valore batteria esposto è fisso a 100, non una misura.

Il comportamento è ricavato dai sorgenti e non sostituisce il collaudo. Il lampeggio connesso usa `delay(30)`, quindi la lettura dell'encoder si interrompe brevemente durante l'impulso.

## Contenuto

- `src/main.cpp`: firmware recuperato dal progetto locale.
- `platformio.ini`: configurazione e dipendenze di compilazione.
- `TECHNICAL.md`: implementazione attuale, vincoli e possibili evoluzioni.
- `TESTING.md`: criteri di accettazione e stato delle verifiche.
- `docs/`: sito statico GitHub Pages e schema SVG.
- `THIRD_PARTY.md`: provenienza immagini e riferimenti.

Non è ancora scelta una licenza di riutilizzo per i materiali originali. La pubblicazione non attribuisce una licenza alle immagini di terzi.
