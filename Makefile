ESPHOME_DIR := esphome
BOARDS := esp32dev esp32s3 esp32c3
BUILD_DIR := build
SCRIPTS_DIR := scripts

PYTHON_MIN := 3.10
ESPHOME_MIN := 2024.6.0

.DEFAULT_GOAL := help

.PHONY: help check-deps install version build-all clean flash-% build-% compile-% logs-% ota-%

help:
	@echo ""
	@echo "Berbel ESP32 - Build System"
	@echo "==========================="
	@echo ""
	@echo "Verfügbare Targets:"
	@echo "  make check-deps         Abhängigkeiten prüfen"
	@echo "  make install            Abhängigkeiten installieren (setup.sh)"
	@echo "  make version            version.h aktualisieren"
	@echo "  make build-all          Firmware für alle Boards bauen"
	@echo "  make build-esp32dev     Firmware für ESP32-DEV bauen"
	@echo "  make build-esp32s3      Firmware für ESP32-S3 bauen"
	@echo "  make build-esp32c3      Firmware für ESP32-C3 bauen"
	@echo "  make flash-esp32dev     Firmware auf ESP32-DEV flashen"
	@echo "  make flash-esp32s3      Firmware auf ESP32-S3 flashen"
	@echo "  make flash-esp32c3      Firmware auf ESP32-C3 flashen"
	@echo "  make logs-esp32dev      Seriellen Monitor für ESP32-DEV"
	@echo "  make ota-esp32dev       OTA-Update für ESP32-DEV"
	@echo "  make clean              Build-Artefakte löschen"
	@echo ""

check-deps:
	@echo "Prüfe Abhängigkeiten..."
	@command -v python3 >/dev/null 2>&1 || (echo "FEHLER: python3 nicht gefunden. Führe 'make install' aus." && exit 1)
	@python3 -c "import sys; v=sys.version_info; ok=v>=(3,10); print(f'  Python: {v.major}.{v.minor}.{v.micro} ' + ('OK' if ok else 'ZU ALT')); sys.exit(0 if ok else 1)"
	@command -v esphome >/dev/null 2>&1 || (echo "FEHLER: esphome nicht gefunden. Führe 'make install' aus." && exit 1)
	@echo "  ESPHome: $$(esphome version 2>/dev/null | head -1) OK"
	@echo "Alle Abhängigkeiten erfüllt."

install:
	@bash setup.sh

version:
	@bash $(SCRIPTS_DIR)/update_version.sh
	@echo "version.h aktualisiert"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

define BUILD_BOARD
build-$(1): check-deps version $(BUILD_DIR)
	@echo "Baue Firmware für Board: $(1)..."
	@VERSION=$$(bash $(SCRIPTS_DIR)/update_version.sh | cut -d+ -f1); \
	  cd $(ESPHOME_DIR) && esphome -s firmware_version "$$VERSION" compile boards/$(1).yaml
	@mkdir -p $(BUILD_DIR)/$(1)
	@find $(ESPHOME_DIR)/.esphome/build -name "*.bin" -exec cp {} $(BUILD_DIR)/$(1)/ \; 2>/dev/null || true
	@echo "Firmware fertig: $(BUILD_DIR)/$(1)/"

flash-$(1): build-$(1)
	@echo "Flashe Firmware auf $(1)..."
	@VERSION=$$(bash $(SCRIPTS_DIR)/update_version.sh | cut -d+ -f1); \
	  cd $(ESPHOME_DIR) && esphome -s firmware_version "$$VERSION" upload boards/$(1).yaml

compile-$(1): check-deps version
	@VERSION=$$(bash $(SCRIPTS_DIR)/update_version.sh | cut -d+ -f1); \
	  cd $(ESPHOME_DIR) && esphome -s firmware_version "$$VERSION" compile boards/$(1).yaml

logs-$(1):
	@cd $(ESPHOME_DIR) && esphome logs boards/$(1).yaml

ota-$(1):
	@VERSION=$$(bash $(SCRIPTS_DIR)/update_version.sh | cut -d+ -f1); \
	  cd $(ESPHOME_DIR) && esphome -s firmware_version "$$VERSION" upload boards/$(1).yaml --device OTA
endef

$(foreach board,$(BOARDS),$(eval $(call BUILD_BOARD,$(board))))

build-all: $(foreach board,$(BOARDS),build-$(board))
	@echo ""
	@echo "Alle Firmwares gebaut:"
	@ls -la $(BUILD_DIR)/*/

clean:
	@echo "Lösche Build-Artefakte..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ESPHOME_DIR)/.esphome
	@echo "Bereinigt."
