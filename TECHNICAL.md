# Descrizione tecnica

## Stato e obiettivo

Questo documento distingue i requisiti del progetto dalle scelte proposte per il futuro firmware. Nessuna delle procedure qui descritte è ancora verificata sul prototipo. Non è disponibile una build da installare.

Volumefy invierà comandi multimediali al sistema operativo: non trasmetterà audio e non sarà una cassa Bluetooth. L'architettura proposta usa ESP32-C3 come periferica BLE HID e il computer/telefono come central.

## Acquisizione encoder

CLK e DT formano una coppia di segnali in quadratura. Una macchina a stati dovrà validare le transizioni dei due bit, accumularle fino a uno scatto completo e scartare i salti non validi. Questo limita gli effetti dei rimbalzi meccanici. Il numero di transizioni per scatto e il verso vanno verificati sul KY-040 montato.

Il pulsante SW richiede debounce separato e un evento singolo per pressione: tenerlo premuto non deve alternare continuamente il mute. La proposta prevede di consumare la prima pressione dopo lo sleep per il solo risveglio, attendendo il rilascio prima di accettare un nuovo mute.

Rotazione valida e pressione valida devono aggiornare il timer di inattività. Lampeggi, pacchetti BLE e diagnostica non devono mantenerlo sveglio. I tempi devono essere gestiti senza attese bloccanti.

## Report Bluetooth

Il servizio HID over GATT dovrà esporre un report Consumer Control con gli usi Volume Increment, Volume Decrement e Mute. Ogni evento richiede pressione e successivo rilascio del comando, per evitare tasti bloccati. I report devono essere inviati solo dopo la preparazione del collegamento e delle sottoscrizioni dell'host. Gli eventi generati senza collegamento non vanno accumulati per poi produrre una raffica alla riconnessione.

Il nome annunciato è `Volumefy`. È previsto pairing Just Works: bonding persistente, nessuna capacità di input/output per un codice, nessuna richiesta di PIN da parte del firmware. Il sistema operativo può comunque mostrare il proprio dialogo di associazione. Just Works non offre protezione autenticata contro un intermediario durante il primo pairing.

## Riconnessione e visibilità

La proposta iniziale usa advertising connettibile aperto: un host già associato può riconnettersi con le chiavi salvate, e un nuovo host può fare pairing quando non c'è una connessione attiva. Una connessione alla volta.

Questa scelta permette il recupero del vecchio host e l'associazione con uno nuovo, ma **non garantisce priorità esclusiva all'ultimo host** se più dispositivi tentano insieme. Un'eventuale finestra iniziale riservata all'ultimo host, con successiva apertura, è una decisione da implementare e collaudare. Un peripheral BLE non può stabilire da solo se l'ultimo host è fuori portata: l'assenza di un tentativo di connessione non ne prova la distanza.

Bond e identità devono sopravvivere a reset e deep sleep. La gestione di chiavi scadute, numero massimo di host memorizzati e cancellazione delle associazioni resta da definire nel firmware; non cancellare indiscriminatamente i bond a ogni disconnessione.

## Stati e tempi richiesti

| Stato | Radio | LED | Uscita |
| --- | --- | --- | --- |
| Disponibile | Advertising connettibile | Un impulso ogni 1 s | Connessione oppure 120 s di inattività |
| Connesso | BLE attivo | Un impulso ogni 15 s | Disconnessione oppure 120 s di inattività |
| Sleep | Spenta | LED controllabile spento | Pulsante SW |

La durata dell'impulso luminoso non è stata specificata: 100 ms è una proposta da validare visivamente. Il periodo indica la distanza fra gli inizi dei lampeggi.

Per il risparmio è proposto il deep sleep con wake su livello basso di GPIO4. Prima di dormire occorre attendere il rilascio del tasto, disattivare correttamente BLE e configurare il pull-up che mantenga SW stabile anche durante lo sleep. Al risveglio vengono ricreati servizio HID e advertising mantenendo l'identità. Il timer riparte al risveglio, così l'utente ha tempo per riconnettersi.

Il deep sleep interrompe il collegamento BLE e può far scomparire la porta USB. Il consumo dell'intera scheda dipende anche da regolatore, LED di alimentazione e pull-up dell'encoder; non è equivalente al valore minimo del solo chip. Non sono dichiarate autonomia o correnti misurate. [Sleep ESP32-C3](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/system/sleep_modes.html).

## Vincoli elettrici e hardware

Il chip ESP32-C3 dispone di CPU RISC-V fino a 160 MHz e Bluetooth LE; non offre Bluetooth Classic. Capacità flash, dimensioni, LED e assorbimento della SuperMini vanno verificati sulla variante acquistata. [Datasheet Espressif](https://documentation.espressif.com/esp32-c3_datasheet_en.html).

GPIO2 partecipa allo strapping: verificare l'avvio con CLK sia alto sia basso, senza dedurre che basti evitare la rotazione durante l'accensione. SW su GPIO4 è la sorgente di wake proposta; verificarne il funzionamento e il bias elettrico durante lo sleep. Alimentazione KY-040 a 3,3 V e massa comune, senza pull-up dei segnali verso 5 V.

## Estensione USB

La periferica USB Serial/JTAG integrata ha funzioni fisse e non espone report HID personalizzati. Un eventuale ponte software dovrà definire protocollo seriale, riconoscimento del dispositivo, gestione delle riconnessioni e arbitraggio USB/BLE per evitare doppi eventi. Non è parte della versione documentale pubblicata. [USB ESP32-C3](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html).
