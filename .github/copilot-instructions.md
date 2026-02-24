# Berbel ESP32 – Copilot Instructions

## Projektübersicht

Dieses Repository implementiert ein ESPHome-Projekt für einen ESP32, der eine **Berbel Dunstabzugshaube** (Skyline Edge Base) via **Bluetooth Low Energy (BLE)** steuert. Die Firmware stellt eine HTTP-API und ein Web-Interface bereit.

## Architektur

```
esphome/
  berbel_esp32.yaml          ← Basis-Konfiguration (board-unabhängig)
  boards/
    esp32dev.yaml            ← ESP32 DevKit v1
    esp32s3.yaml             ← ESP32-S3 DevKitC-1
    esp32c3.yaml             ← ESP32-C3 DevKitM-1
  components/
    berbel/
      __init__.py            ← ESPHome-Komponenten-Definition (Python)
      berbel_component.h     ← C++ Header
      berbel_component.cpp   ← C++ Implementierung (NimBLE + AsyncWebServer)
  secrets.yaml.example       ← WLAN-Zugangsdaten-Vorlage
  version.h                  ← Auto-generiert von scripts/update_version.sh

scripts/
  update_version.sh          ← Versionierung (version.txt + git count → version.h)

Makefile                     ← Build-Automatisierung
setup.sh                     ← Installations-Skript (macOS + Linux)
version.txt                  ← Basis-Version (MAJOR.MINOR)
.github/workflows/build.yml  ← CI/CD (baut alle Boards, erstellt Release)
```

## BLE-Protokoll (Berbel Skyline Edge Base)

### GATT-Charakteristiken

| UUID | Richtung | Beschreibung |
|------|----------|--------------|
| `F006F005-5745-4053-8043-62657262656C` | Lesen | Status (Fan, Licht, Nachlauf) |
| `F006F006-5745-4053-8043-62657262656C` | Lesen/Schreiben | Helligkeit + Steuerbefehle |
| `F006F007-5745-4053-8043-62657262656C` | Lesen/Schreiben | Farbtemperatur |

### Status-Parsing (Charakteristik F006F005)

| Byte | Wert | Bedeutung |
|------|------|-----------|
| 0 | `0x10` | Lüfter Stufe 1 |
| 1 | `0x01` | Lüfter Stufe 2 |
| 1 | `0x10` | Lüfter Stufe 3 |
| 2 | `& 0x10` | Licht Oben AN |
| 4 | `& 0x10` | Licht Unten AN |
| 5 | `& 0x90` | Nachlauf aktiv |

### Helligkeits-Parsing (Charakteristik F006F006)

| Byte | Bedeutung |
|------|-----------|
| 4 | Helligkeit Unten (0–255) |
| 5 | Helligkeit Oben (0–255) |

### Farb-Parsing (Charakteristik F006F007)

| Byte | Bedeutung |
|------|-----------|
| 6 | Farbtemperatur Unten (0=warm/2700K, 255=kalt/6500K) |
| 7 | Farbtemperatur Oben |

### Befehls-Format (31 Bytes, schreiben auf F006F006)

```
Lüfter:
  AUS:    01 58 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ...
  Stufe1: 01 58 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 00 ...
  Stufe2: 01 58 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 00 ...
  Stufe3: 01 58 00 00 00 00 00 00 00 00 00 00 00 00 00 00 03 00 ...

Licht (Helligkeit):
  Format: 01 63 00 00 BB TT 00 ... (BB=Unten 0–255, TT=Oben 0–255)
  Beide AN: 01 63 00 00 FF FF 00 ...
  Beide AUS: 01 63 00 00 00 00 00 ...
```

## HTTP API

Alle Endpunkte sind auf Port 80 verfügbar.

| Methode | Pfad | Body (JSON) | Beschreibung |
|---------|------|-------------|--------------|
| GET | `/` | – | Web-Interface |
| GET | `/api/state` | – | Aktueller Gerätestatus |
| POST | `/api/fan` | `{"level": 0-3}` | Lüfterstufe setzen |
| POST | `/api/light/top` | `{"on": bool, "brightness": 0-100, "color_temp": 2700-6500}` | Licht Oben steuern |
| POST | `/api/light/bottom` | `{"on": bool, "brightness": 0-100, "color_temp": 2700-6500}` | Licht Unten steuern |
| POST | `/api/scan` | – | BLE-Scan starten |
| GET | `/api/scan/results` | – | Scan-Ergebnisse abrufen |
| POST | `/api/pair` | `{"mac": "AA:BB:CC:DD:EE:FF"}` | Mit Gerät koppeln |
| POST | `/api/disconnect` | – | BLE-Verbindung trennen |

### Beispiel: Status abrufen

```bash
curl http://berbel-esp32.local/api/state
```

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

## Koppeln / Ersteinrichtung

1. ESP32 startet einen WiFi-Accesspoint **"Berbel-Setup"** (Passwort: `berbel1234`), wenn kein WLAN konfiguriert ist
2. Im Browser `http://192.168.4.1` aufrufen
3. WLAN einrichten
4. Danach `http://berbel-esp32.local` aufrufen
5. **"BLE-Scan starten"** drücken
6. Das Berbel-Gerät in der Liste auswählen und **"Koppeln"** drücken
7. Die MAC-Adresse wird dauerhaft im NVS gespeichert

## Versionsschema

`MAJOR.MINOR.PATCH+HASH`

- `MAJOR.MINOR` wird manuell in `version.txt` gepflegt
- `PATCH` = Anzahl der Git-Commits (`git rev-list --count HEAD`)
- `HASH` = Kurzer Git-Commit-Hash
- Generiert in `esphome/version.h` durch `scripts/update_version.sh`

## Build-System

```bash
make install          # Abhängigkeiten installieren
make build-esp32dev   # Firmware für ESP32-DEV kompilieren
make build-all        # Alle Boards bauen
make flash-esp32dev   # Flashen via USB
make clean            # Build-Artefakte löschen
```

## Unterstützte ESP32-Boards

| Board-ID | Chip | Flash | Hinweis |
|----------|------|-------|---------|
| `esp32dev` | ESP32 | 4 MB | Standard DevKit v1 |
| `esp32s3` | ESP32-S3 | 16 MB | DevKitC-1, mehr RAM |
| `esp32c3` | ESP32-C3 | 4 MB | Kleines Board, BLE 5.0 |

## Abhängigkeiten (C++)

- **NimBLE-Arduino** – BLE-Client-Stack
- **ESP Async WebServer** – Asynchroner HTTP-Server
- **ArduinoJson** – JSON-Verarbeitung
- **ESPHome** – Framework (WiFi, OTA, Logging, Preferences)

## Code-Konventionen

- **Kein Quellcode-Kommentare** (Anforderung)
- C++ Standard: C++17
- Namespace: `esphome::berbel`
- Logging via `ESP_LOGI(TAG, ...)` usw.
- Persistenz via `ESPPreferences` (NVS)
- BLE-State-Machine: IDLE → SCANNING / CONNECTING → CONNECTED → IDLE

## Wichtige Dateien bei Änderungen aktualisieren

- Neue API-Endpunkte: `berbel_component.cpp` → `setup_http()` + Web-Interface HTML
- Neues Board: `esphome/boards/<name>.yaml` + `Makefile` BOARDS-Variable + `build.yml` matrix
- Neues BLE-Feature: `berbel_component.h` (Deklaration) + `berbel_component.cpp` (Implementierung)
- Version bump: `version.txt` bearbeiten (MAJOR.MINOR), PATCH wird automatisch berechnet
