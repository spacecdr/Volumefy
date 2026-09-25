# Verifiche

Stato iniziale: **nessun firmware compilato o collaudato per questa pubblicazione**. Quella che segue è una lista di prove da eseguire, non un registro di test superati.

| Prova | Criterio di accettazione | Stato |
| --- | --- | --- |
| Compilazione riproducibile | Toolchain e dipendenze fissate; build da checkout pulito | Da eseguire |
| Encoder lento/veloce | Conteggio coerente per scatto nei due versi, nessuna inversione spuria | Da eseguire |
| Pulsante | Un solo mute per pressione, nessuna ripetizione se tenuto | Da eseguire |
| Pairing nuovo | Nome Volumefy; nessun codice; HID riconosciuto dall'host | Da eseguire |
| Volume/mute | Ricezione nativa dei report e risposta del sistema operativo | Da eseguire |
| LED discovery | Un impulso ogni 1 s | Da eseguire |
| LED collegato | Un impulso ogni 15 s | Da eseguire |
| Inattività | Sleep dopo 120 s senza rotazione o pressione, anche sotto USB | Da eseguire |
| Wake | SW risveglia; proposta: prima pressione non invia mute | Da eseguire |
| Riconnessione | Bond conservato; host precedente può riconnettersi | Da eseguire |
| Nuovo host | Dopo assenza del precedente, nuovo pairing possibile | Da eseguire |
| Reset e strapping | Avvio con encoder nelle diverse posizioni, pulsante premuto/rilasciato | Da eseguire |
| Consumo | Corrente dell'intera scheda misurata in discovery, connessione e sleep | Da eseguire |
| Sistemi operativi | macOS, Windows, Linux e dispositivi mobili verificati separatamente | Da eseguire |

Una simulazione di eventi non prova i rimbalzi del contatto fisico. Una connessione GATT non prova la ricezione dei comandi multimediali. Registrare separatamente test automatici, osservazioni hardware e comportamento dell'host.
