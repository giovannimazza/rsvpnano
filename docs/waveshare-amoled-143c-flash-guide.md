# Waveshare AMOLED 1.43C — Guida al flash di libri e firmware

Questa guida spiega come caricare libri e aggiornare il firmware sul dispositivo
**Waveshare ESP32-S3-Touch-AMOLED-1.43C**, che non dispone di slot per scheda SD.
I libri vengono caricati direttamente nella memoria flash interna tramite LittleFS.

---

## Prerequisiti

- [PlatformIO](https://platformio.org/) installato (CLI o estensione VS Code)
- Cavo USB-C collegato al dispositivo
- PowerShell (Windows) — minimo versione 5.1
- Il repository clonato sul branch `release/amoled-143`

---

## Struttura della memoria flash

| Partizione | Tipo      | Offset     | Dimensione | Contenuto             |
|------------|-----------|------------|------------|-----------------------|
| `nvs`      | NVS       | `0x9000`   | 20 KB       | Impostazioni salvate  |
| `app0`     | Factory   | `0x10000`  | 3 MB        | Firmware              |
| `spiffs`   | LittleFS  | `0x310000` | ~4.875 MB   | Libri e file dati     |
| `coredump` | Coredump  | `0x7F0000` | 64 KB       | Diagnostica crash     |

---

## Come aggiungere libri

### Formato supportato

I libri devono essere in formato **`.rsvp`** (testo per RSVP reading).

> ⚠️ **Importante**: I nomi dei file **non devono contenere spazi**.
> Lo script rinomina automaticamente gli spazi in `_` prima del flash.

### 1. Copia i file nella cartella corretta

Posiziona i file `.rsvp` nella cartella:

```
data/books/books/
```

Esempio:

```
data/
└── books/
    └── books/
        ├── DungeonCrawlerCarl.rsvp
        ├── Il_mio_libro.rsvp
        └── ...
```

> I file in `data/books/articles/` appaiono invece nella sezione **Articles**.

---

## Flash del firmware e dei libri

### Script automatico (raccomandato)

Dalla root del repository, esegui:

```powershell
.\tools\flash_143c.ps1
```

Lo script esegue automaticamente questi passaggi in ordine:

1. **Rimozione file obsoleti** — elimina eventuali file generati da build precedenti
2. **Rinomina spazi** — sostituisce gli spazi nei nomi dei file con `_`
3. **Controllo spazio** — verifica che i libri entrino nella partizione LittleFS (~4.4 MB usabili)
4. **Flash filesystem** (`uploadfs`) — carica i libri in LittleFS a `0x310000`
5. **Flash firmware** (`upload`) — installa firmware e partition table aggiornata

#### Output atteso

```
==> Checking for spaces in filenames...
   No spaces found.

==> Checking available space...
   Partition : 5000 KB  (usable ~4500 KB after LittleFS overhead)
   Data size : 856 KB  (19% of usable space)
   Space OK.

==> Flashing filesystem (books)...
   [...]

==> Flashing firmware...
   [...]

Done! Books and firmware flashed successfully.
```

---

### Flash manuale (alternativo)

Se preferisci eseguire i comandi separatamente:

```powershell
# Solo filesystem (libri)
pio run -e waveshare_esp32s3_touch_amoled_143c -t uploadfs

# Solo firmware
pio run -e waveshare_esp32s3_touch_amoled_143c -t upload
```

> ⚠️ Esegui sempre **prima** `uploadfs` e **poi** `upload`, altrimenti la partition
> table aggiornata potrebbe non corrispondere all'immagine già caricata.

---

## Monitor seriale (debug)

Per verificare il corretto funzionamento dopo il flash:

```powershell
pio device monitor -e waveshare_esp32s3_touch_amoled_143c
```

### Log attesi all'avvio

```
[sd-check] internal flash mounted: used=XXX total=5111808 free=XXX bytes
[book-scan] found: /books/books/DungeonCrawlerCarl.rsvp size=876856
[book-scan] dir /books/books: 1 entries
[storage] Directory inventory: 3 files, 1 books, 0 cache probes in XX ms
```

### Errori comuni

| Messaggio                          | Causa                                         | Soluzione                                           |
|------------------------------------|-----------------------------------------------|-----------------------------------------------------|
| `internal flash mount failed`      | Partizione LittleFS non trovata               | Ri-esegui `flash_143c.ps1` con la versione corrente |
| `Not enough space for sidecar`     | File troppo grande per l'indicizzazione       | LittleFS ha ~4.4 MB usabili; rimuovi libri in eccesso |
| `data sidecar flush failed`        | Spazio esaurito durante l'indicizzazione      | Controlla spazio libero con il monitor seriale      |
| `Failed to load selected book`     | Indicizzazione fallita                        | Vedi log seriale per dettagli                       |

---

## Funzionalità disabilitate su questo dispositivo

Il 143C è configurato per uso **offline**. Le seguenti funzionalità non sono disponibili
(flag `RSVP_NO_WIFI=1`):

- Aggiornamenti OTA (Over-The-Air)
- Feed RSS
- Sincronizzazione con companion app
- Impostazioni Wi-Fi

---

## Limiti pratici

| Cosa                  | Limite                                      |
|-----------------------|---------------------------------------------|
| Spazio totale libri   | ~4.4 MB usabili (LittleFS overhead ~10%)    |
| Dimensione singolo libro | Nessun limite fisso; dipende dallo spazio libero. Per libri >1.5 MB verificare che ci sia spazio sufficiente per i file sidecar (~2× la dimensione del libro) |
| Nome file             | Niente spazi (sostituiti automaticamente con `_`) |
| Formato supportato    | `.rsvp`                                     |

---

## Struttura sidecar (generata automaticamente)

Quando apri un libro per la prima volta, il dispositivo crea in LittleFS dei file
di indice ausiliari:

```
/books/books/
├── MioLibro.rsvp          ← libro sorgente (caricato via uploadfs)
├── MioLibro.rsvp.rdat     ← dati parole (generato al primo avvio)
└── MioLibro.rsvp.ridx     ← indice parole (generato al primo avvio)
```

Questi file vengono creati automaticamente e non devono essere caricati manualmente.
