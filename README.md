# Berbel ESP32

Ein **ESPHome**-basiertes Firmware-Projekt für den ESP32, das eine **Berbel Dunstabzugshaube** (Skyline Edge Base) über **Bluetooth Low Energy (BLE)** steuert. Der ESP32 stellt eine vollständige **HTTP-API** sowie ein eingebettetes **Web-Interface** für Einrichtung und Steuerung bereit.

## Funktionen

- **Lüftersteuerung**: Stufe 0–3 über HTTP-API und Web-Interface
- **Lichtsteuerung**: Oben und Unten separat – Ein/Aus, Helligkeit (0–100 %), Farbtemperatur (2700–6500 K)
- **Nachlauf-Monitoring**: Statusanzeige im Web-Interface
- **BLE-Pairing**: Einfaches Koppeln über Web-Interface (Scan → Auswahl → Koppeln)
- **WiFi-Ersteinrichtung**: Captive-Portal-Accesspoint bei fehlendem WLAN
- **Persistente Einstellungen**: Geräte-MAC dauerhaft im NVS des ESP32 gespeichert
- **OTA-Updates**: Firmware-Updates über WLAN
- **Multi-Board-Support**: ESP32-DEV, ESP32-S3, ESP32-C3

---

## Inhalt

- [Voraussetzungen](#voraussetzungen)
- [Installation](#installation)
- [Konfiguration](#konfiguration)
- [Bauen und Flashen](#bauen-und-flashen)
- [Ersteinrichtung](#ersteinrichtung)
- [HTTP-API](#http-api)
- [Web-Interface](#web-interface)
- [BLE-Protokoll](#ble-protokoll)
- [Versionsschema](#versionsschema)
- [GitHub Actions / CI](#github-actions--ci)
- [Projektstruktur](#projektstruktur)
- [Troubleshooting](#troubleshooting)

---

## Voraussetzungen

| Anforderung | Minimalversion |
|-------------|----------------|
| Python | 3.10 |
| ESPHome | 2024.6.0 |
| Git | beliebig |
| Make | beliebig |

### Unterstützte ESP32-Boards

| Board | Chip | Empfohlen |
|-------|------|-----------|
| `esp32dev` | ESP32 (original) | ✅ Standard-Board |
| `esp32s3` | ESP32-S3 | ✅ Mehr RAM/Flash |
| `esp32c3` | ESP32-C3 | ✅ Kompaktes Board |

---

## Installation

### Automatisch (empfohlen)

```bash
git clone https://github.com/timboettiger/berbel-interface.git
cd berbel-interface
bash setup.sh
```

Das Skript erkennt das Betriebssystem (macOS / Linux), prüft alle Abhängigkeiten und installiert fehlende Komponenten automatisch.

### Manuell

```bash
# Python 3.10+
python3 --version

# ESPHome installieren
pip install "esphome>=2024.6.0"
```

---

## Konfiguration

### secrets.yaml

Kopiere die Vorlage und fülle deine WLAN-Daten ein:

```bash
cp esphome/secrets.yaml.example esphome/secrets.yaml
```

Inhalt anpassen:

```yaml
wifi_ssid: "DeinWLAN"
wifi_password: "DeinPasswort"
api_password: "SicheresPasswort"
ota_password: "SicheresOTAPasswort"
```

### Optionale Vorkonfiguration (MAC-Adresse)

Wenn die BLE-MAC-Adresse der Dunstabzugshaube bereits bekannt ist, kann sie direkt in `esphome/berbel_esp32.yaml` angegeben werden:

```yaml
substitutions:
  device_mac: "AA:BB:CC:DD:EE:FF"
```

Andernfalls kann das Koppeln nach dem ersten Start über das Web-Interface durchgeführt werden.

---

## Bauen und Flashen

```bash
# Abhängigkeiten prüfen
make check-deps

# Firmware für ESP32-DEV kompilieren
make build-esp32dev

# Firmware für ESP32-S3 kompilieren
make build-esp32s3

# Alle Boards auf einmal bauen
make build-all

# Firmware auf angeschlossenen ESP32 flashen
make flash-esp32dev

# Seriellen Monitor öffnen
make logs-esp32dev

# OTA-Update (ESP32 muss im WLAN sein)
make ota-esp32dev

# Build-Artefakte löschen
make clean
```

---

## Ersteinrichtung

### Schritt 1: WiFi einrichten

Beim ersten Start (oder wenn das gespeicherte WLAN nicht verfügbar ist) öffnet der ESP32 automatisch einen WLAN-Accesspoint:

- **SSID**: `Berbel-Setup`
- **Passwort**: `berbel1234`

Mit diesem Netzwerk verbinden, dann im Browser `http://192.168.4.1` aufrufen und das Heim-WLAN eintragen.

### Schritt 2: Berbel-Dunstabzugshaube koppeln

Nach erfolgreichem WLAN-Login das Web-Interface öffnen:

```
http://berbel-esp32.local
```

oder mit der IP-Adresse:

```
http://<IP-ADRESSE>
```

1. Im Abschnitt **"Gerät koppeln"** auf **"BLE-Scan starten"** klicken
2. Warten bis die Dunstabzugshaube in der Liste erscheint (ca. 8 Sekunden)
3. **"Koppeln"** neben dem Gerät drücken
4. Der ESP32 verbindet sich und speichert die MAC-Adresse dauerhaft

---

## HTTP-API

Alle Endpunkte auf Port 80 (kein Authentifizierungstoken erforderlich im lokalen Netz).

### Status abrufen

```bash
GET /api/state
```

Antwort:
```json
{
  "connected": true,
  "mac": "AA:BB:CC:DD:EE:FF",
  "fan_level": 2,
  "fan_postrun": false,
  "light_top_on": true,
  "light_bottom_on": false,
  "light_top_brightness": 75,
  "light_bottom_brightness": 0,
  "light_top_color_kelvin": 4000,
  "light_bottom_color_kelvin": 2700
}
```

### Lüfter steuern

```bash
POST /api/fan
Content-Type: application/json

{"level": 2}
```

| Wert | Bedeutung |
|------|-----------|
| `0` | Aus |
| `1` | Stufe 1 (leise) |
| `2` | Stufe 2 (mittel) |
| `3` | Stufe 3 (stark) |

```bash
# Beispiele mit curl
curl -X POST http://berbel-esp32.local/api/fan -H "Content-Type: application/json" -d '{"level":2}'
curl -X POST http://berbel-esp32.local/api/fan -H "Content-Type: application/json" -d '{"level":0}'
```

### Licht steuern

```bash
POST /api/light/top       # Licht Oben
POST /api/light/bottom    # Licht Unten
Content-Type: application/json
```

Parameter (alle optional, können kombiniert werden):

| Parameter | Typ | Bereich | Beschreibung |
|-----------|-----|---------|--------------|
| `on` | bool | `true`/`false` | Ein- oder ausschalten |
| `brightness` | int | 0–100 | Helligkeit in Prozent |
| `color_temp` | int | 2700–6500 | Farbtemperatur in Kelvin |

```bash
# Licht Oben einschalten mit 75% Helligkeit und warmweißem Licht
curl -X POST http://berbel-esp32.local/api/light/top \
  -H "Content-Type: application/json" \
  -d '{"on": true, "brightness": 75, "color_temp": 2700}'

# Licht Unten ausschalten
curl -X POST http://berbel-esp32.local/api/light/bottom \
  -H "Content-Type: application/json" \
  -d '{"on": false}'

# Nur Helligkeit ändern
curl -X POST http://berbel-esp32.local/api/light/top \
  -H "Content-Type: application/json" \
  -d '{"brightness": 50}'
```

### BLE-Scan und Pairing

```bash
# Scan starten
POST /api/scan

# Scan-Ergebnisse abrufen (polling)
GET /api/scan/results
```

Antwort Scan-Ergebnisse:
```json
{
  "scanning": false,
  "devices": [
    {"address": "AA:BB:CC:DD:EE:FF", "name": "Berbel SEB", "rssi": -65}
  ]
}
```

```bash
# Gerät koppeln
POST /api/pair
Content-Type: application/json

{"mac": "AA:BB:CC:DD:EE:FF"}
```

```bash
# Verbindung trennen
POST /api/disconnect
```

---

## Web-Interface

Das Web-Interface ist unter `http://berbel-esp32.local/` erreichbar und bietet:

- **Verbindungsstatus** mit MAC-Adresse
- **Lüftersteuerung** (Stufe 0–3, Schaltflächen)
- **Nachlauf-Status** (Anzeige)
- **Licht Oben**: Ein/Aus, Helligkeit-Slider, Farbtemperatur-Slider
- **Licht Unten**: Ein/Aus, Helligkeit-Slider, Farbtemperatur-Slider
- **Gerät koppeln**: BLE-Scan, Geräteliste, Koppeln-Schaltfläche
- **Automatisches Polling** alle 10 Sekunden

---

## BLE-Protokoll

### GATT-Charakteristiken

| UUID | Richtung | Beschreibung |
|------|----------|--------------|
| `F006F005-5745-4053-8043-62657262656C` | Lesen | Fan/Licht/Nachlauf-Status |
| `F006F006-5745-4053-8043-62657262656C` | Lesen/Schreiben | Helligkeit + Steuerbefehle |
| `F006F007-5745-4053-8043-62657262656C` | Lesen/Schreiben | Farbtemperatur |

### Status-Byte-Layout

```
Byte 0:  0x10 → Lüfter Stufe 1
Byte 1:  0x01 → Lüfter Stufe 2  |  0x10 → Lüfter Stufe 3
Byte 2:  & 0x10 → Licht Oben AN
Byte 4:  & 0x10 → Licht Unten AN
Byte 5:  & 0x90 → Nachlauf aktiv
```

### Helligkeits-Byte-Layout (Charakteristik F006F006)

```
Byte 4: Helligkeit Unten (0–255)
Byte 5: Helligkeit Oben  (0–255)
```

### Farb-Byte-Layout (Charakteristik F006F007)

```
Byte 6: Farbtemperatur Unten (0=2700K warm, 255=6500K kalt)
Byte 7: Farbtemperatur Oben
```

### Befehls-Format (31 Bytes)

```
Lüfter:
  01 58 00 00 00 00 00 00 00 00 00 00 00 00 00 00 LL 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  LL: 00=Aus, 01=Stufe1, 02=Stufe2, 03=Stufe3

Licht (Helligkeit):
  01 63 00 00 BB TT 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  BB: Helligkeit Unten (0–255)
  TT: Helligkeit Oben  (0–255)
```

---

## Versionsschema

Format: `MAJOR.MINOR.PATCH+HASH`

- `MAJOR.MINOR`: Manuell in `version.txt` gepflegt
- `PATCH`: Automatisch aus `git rev-list --count HEAD`
- `HASH`: Kurzer Git-Commit-Hash (`git rev-parse --short HEAD`)

Generiert durch `scripts/update_version.sh` → `esphome/version.h`

```bash
bash scripts/update_version.sh
# Ausgabe: 0.0.42+a1b2c3d
```

---

## GitHub Actions / CI

### Workflow: `build.yml`

Trigger: Push auf `main` oder `master`

1. **Version berechnen** (job: `version`)
2. **Firmware für alle Boards bauen** (job: `build`, Matrix: esp32dev, esp32s3, esp32c3)
3. **GitHub Release erstellen** (job: `release`) mit allen `.bin`-Dateien als Anhänge

### Release-Artefakte

| Datei | Board |
|-------|-------|
| `berbel-esp32-esp32dev-firmware.bin` | ESP32 DevKit v1 |
| `berbel-esp32-esp32s3-firmware.bin` | ESP32-S3 DevKitC-1 |
| `berbel-esp32-esp32c3-firmware.bin` | ESP32-C3 DevKitM-1 |

---

## Projektstruktur

```
berbel-interface/
├── esphome/
│   ├── berbel_esp32.yaml          ← Basis-Konfiguration
│   ├── version.h                  ← Auto-generiert (nicht commiten)
│   ├── secrets.yaml               ← WLAN-Daten (nicht commiten)
│   ├── secrets.yaml.example       ← Vorlage
│   ├── boards/
│   │   ├── esp32dev.yaml
│   │   ├── esp32s3.yaml
│   │   └── esp32c3.yaml
│   └── components/
│       └── berbel/
│           ├── __init__.py        ← ESPHome-Komponenten-Definition
│           ├── berbel_component.h
│           └── berbel_component.cpp
├── scripts/
│   └── update_version.sh
├── Makefile
├── setup.sh
├── version.txt                    ← Basis-Version (MAJOR.MINOR)
├── README.md
├── .github/
│   ├── workflows/
│   │   └── build.yml
│   └── copilot-instructions.md
└── custom_components/             ← Ursprüngliche Home Assistant Integration
    └── berbel/                    ← (für Referenz behalten)
```

---

## Troubleshooting

### ESP32 findet Berbel-Gerät nicht

- Dunstabzugshaube einschalten und sicherstellen, dass sie im BLE-Modus ist
- Keine andere App ist aktiv mit der Haube verbunden (BLE erlaubt nur eine Verbindung gleichzeitig)
- Abstand < 5 Meter beim Scan
- Scan erneut starten

### Verbindung bricht ab

- Normales Verhalten: Der ESP32 verbindet sich für jeden Befehl kurz, liest den Status und trennt sich wieder
- Dauerverbindung ist nicht notwendig – Status wird alle 18 Sekunden aktualisiert

### Web-Interface nicht erreichbar

```bash
# ESPHome-Logs prüfen
make logs-esp32dev
```

oder direkt:

```bash
cd esphome && esphome logs boards/esp32dev.yaml
```

### Serielle Ausgabe (Debug)

```bash
make logs-esp32dev
# oder
cd esphome && esphome logs boards/esp32dev.yaml
```

Log-Level in `berbel_esp32.yaml` anpassen:

```yaml
logger:
  level: DEBUG  # INFO / DEBUG / VERBOSE
```

### Firmware zurücksetzen

1. Neueste Firmware von [Releases](https://github.com/timboettiger/berbel-interface/releases) herunterladen
2. Flashen:
   ```bash
   esptool.py --chip esp32 write_flash 0x0 berbel-esp32-esp32dev-firmware.bin
   ```

---

## Lizenz

MIT License – siehe [LICENSE](LICENSE)

## Hinweis

Dieses Projekt ist kein offizielles Produkt der berbel Ablufttechnik GmbH. Es handelt sich um ein unabhängiges Community-Projekt. Nutzung auf eigenes Risiko.
