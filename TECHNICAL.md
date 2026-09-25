# Volumefy · implementazione BLE + IR

Il riferimento è `src/main.cpp`, copiato dal progetto locale aggiornato. La vecchia documentazione solo BLE è superata; la cronologia Git conserva le versioni precedenti. Questa descrizione riguarda il comportamento del codice, non un collaudo fisico.

## Stati, ingressi e memoria

- `MODE_BLE`: Wi-Fi spento e GPIO5 basso; `BleKeyboard("Volumefy", "Custom", 100)` su NimBLE. Volume/mute inviati solo quando connesso. Associazione e riconnessione dipendono anche dall'host; nessuna priorità personalizzata fra host. Il valore batteria è statico.
- `MODE_IR`: BLE non inizializzato, `IRsend` su GPIO5, access point `Volumefy-Setup`, DNS captive portal e HTTP sulla porta 80. Nessuna rete domestica richiesta.
- Cambio modalità: pressione ≥2.000 ms, salvataggio in NVS e riavvio, con impulso LED di 80 ms. Le radio non vengono usate contemporaneamente.
- Encoder: polling del fronte di discesa CLK su GPIO2, DT su GPIO3 per il verso. `DT != CLK` invia volume giù, direzione invertita rispetto al vecchio codice. Non è implementato un filtro completo in quadratura.
- Pulsante: GPIO4 con pull-up, debounce 30 ms. Pressione breve invia mute al rilascio; la lunga non genera mute. All'avvio con SW basso, i comandi sono disarmati fino al rilascio.
- NVS, namespace `volumefy`: `mode`, `remote`, `favBits`, `irRepeat`. Default BLE, profilo 0 e ripetizione 2×. I primi sette indici del catalogo precedente sono conservati; 125 profili occupano 16 byte di bitset preferiti.

## Sleep e LED

Deep sleep dopo 120.000 ms senza attività. Rotazione, pressione e richieste web gestite aggiornano il timer. I filtri client-side non generano richieste e non tengono sveglio il dispositivo. Durante upload OTA o riavvio OTA pendente il timeout è sospeso. In IR vengono chiusi DNS e AP; wake su GPIO4 LOW e ripristino della modalità salvata. Non è dichiarato un consumo misurato.

| Stato | LED GPIO8, attivo LOW |
| --- | --- |
| BLE non connesso | Cambio di stato ogni 1 s, ciclo completo 2 s |
| BLE connesso | Impulso 30 ms ogni 15 s |
| IR/AP | Impulso 30 ms ogni 5 s |
| Deep sleep | LED controllabile spento |

I `delay(30)` degli impulsi e le trasmissioni IR sono bloccanti: verificare eventuali scatti persi nelle rotazioni rapide.

## Invio IR

| Famiglia | Implementazione |
| --- | --- |
| NEC / NEC extended | `encodeNEC` e `sendNEC` |
| Samsung32 | `encodeSAMSUNG` e `sendSAMSUNG` |
| RC5 | Toggle tra pressioni logiche successive |
| RC6 Mode 0 | 20 bit, toggle tra pressioni; stesso toggle nei repeat |
| Sony SIRC | 12 bit; almeno 3 frame per pressione |
| RCA | Invio generico a 24 bit e portante 58 kHz |

La velocità 1×–6× imposta `irVolumeRepeat`: per i comandi volume i repeat aggiuntivi sono `valore - 1`; Sony usa `2 + valore - 1` per conservare il minimo del protocollo. Mute non usa l'accelerazione. La portante resta quella del protocollo. Non è un moltiplicatore garantito del volume percepito: la risposta dipende dall'apparecchio.

125 profili: 101 TV, 20 soundbar, 4 audio. Il catalogo contiene alias e famiglie: leggere stato e nota, poi provare il dispositivo. Philips HTL3140B e HTL2163B/12 usano RC6 address `0x10`, comandi `0x10 / 0x11 / 0x0D`. La cattura di riferimento è in `reference/Philips_HTL2163_Soundbar.ir`.

## Web e OTA

| Metodo e percorso | Funzione |
| --- | --- |
| GET `/` | Pannello HTML, catalogo e filtri JavaScript |
| GET `/activate?id=N` | Salva profilo attivo |
| GET `/favorite?id=N` | Alterna stato preferito |
| GET `/irspeed?value=N` | Limita e salva velocità a 1–6 |
| GET `/test?action=up`, `down` o `mute` | Invia comando del profilo attivo |
| POST `/update` | Caricamento multipart del firmware applicativo |

L'interfaccia ha filtri Tipo → Marca → Modello, ricerca nel sottoinsieme selezionato, stato e nota, preferiti, tre test, slider e OTA con progress bar. DNS e percorsi comuni captive portal riportano alla home.

L'OTA usa `Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)`, scrittura a blocchi e `Update.end(true)`. L'esito positivo restituisce HTTP 200 e programma il riavvio dopo 1.200 ms; errore HTTP 500, upload interrotto con `Update.abort()`. Non è implementata una verifica firmata dell'autore del firmware o un rollback applicativo automatico. Password AP predefinita pubblica, HTTP senza login aggiuntivo: l'accesso alla rete consente controllo e OTA. Configurazione OTA a due slot in `default.csv`.

La migrazione dal firmware solo BLE richiede caricamento via USB. Il pannello pubblico `docs/panel.html` è generato da `handleRoot()` usando un adattatore C++ host: stato di esempio Philips HTL3140B, due preferiti, velocità 2×. Filtri e slider sono esplorabili; richieste all'hardware e upload sono intercettati e non inviati. Non simula il backend né dimostra una prova hardware.

## Hardware e limiti

CLK2, DT3, SW4, comando IR5, LED8 attivo LOW; encoder alimentato a 3,3 V e massa comune. Verificare strapping GPIO2, pull-up e LED della variante SuperMini. Usare un modulo IR compatibile o uno stadio transistor/MOSFET con resistenza dimensionata per un LED nudo.

Il dispositivo invia comandi senza feedback sul volume e senza apprendimento IR. L'USB Serial/JTAG integrata del C3 non è USB HID ([Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html)). Corrente, autonomia, portata, precisione encoder e compatibilità effettiva richiedono prove sul dispositivo.
