#!/usr/bin/env bash
set -euo pipefail

PYTHON_MIN_VERSION="3.10"
ESPHOME_MIN_VERSION="2024.6.0"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
success() { echo -e "${GREEN}[OK]${NC}   $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERR]${NC}  $*"; exit 1; }

detect_os() {
  if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "macos"
  elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "linux"
  else
    error "Nicht unterstütztes Betriebssystem: $OSTYPE"
  fi
}

version_ge() {
  python3 -c "import sys; v=tuple(int(x) for x in '$1'.split('.')); r=tuple(int(x) for x in '$2'.split('.')); sys.exit(0 if v >= r else 1)"
}

check_python() {
  if ! command -v python3 &>/dev/null; then
    return 1
  fi
  local ver
  ver=$(python3 -c "import platform; print(platform.python_version())")
  if version_ge "$ver" "$PYTHON_MIN_VERSION" 2>/dev/null; then
    return 0
  fi
  warn "Python $ver gefunden, mindestens $PYTHON_MIN_VERSION benötigt"
  return 1
}

install_python_macos() {
  info "Installiere Python via Homebrew..."
  if ! command -v brew &>/dev/null; then
    info "Installiere Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  fi
  brew install python@3.12
  success "Python installiert"
}

install_python_linux() {
  info "Installiere Python..."
  if command -v apt-get &>/dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y python3 python3-pip python3-venv python3-dev
  elif command -v dnf &>/dev/null; then
    sudo dnf install -y python3 python3-pip
  elif command -v pacman &>/dev/null; then
    sudo pacman -Sy --noconfirm python python-pip
  else
    error "Kein unterstützter Paketmanager gefunden (apt/dnf/pacman)"
  fi
  success "Python installiert"
}

check_esphome() {
  if ! command -v esphome &>/dev/null; then
    return 1
  fi
  local ver
  ver=$(esphome version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 || echo "0.0.0")
  if version_ge "$ver" "$ESPHOME_MIN_VERSION" 2>/dev/null; then
    return 0
  fi
  warn "ESPHome $ver gefunden, mindestens $ESPHOME_MIN_VERSION benötigt"
  return 1
}

install_esphome() {
  info "Installiere ESPHome >= $ESPHOME_MIN_VERSION..."
  python3 -m pip install --upgrade "esphome>=${ESPHOME_MIN_VERSION}"
  success "ESPHome installiert"
}

check_git() {
  command -v git &>/dev/null
}

install_git_macos() {
  info "Installiere git via Homebrew..."
  brew install git
}

install_git_linux() {
  info "Installiere git..."
  if command -v apt-get &>/dev/null; then
    sudo apt-get install -y git
  elif command -v dnf &>/dev/null; then
    sudo dnf install -y git
  elif command -v pacman &>/dev/null; then
    sudo pacman -Sy --noconfirm git
  fi
}

check_make() {
  command -v make &>/dev/null
}

install_make_macos() {
  info "Installiere make via Homebrew..."
  brew install make
}

install_make_linux() {
  info "Installiere make..."
  if command -v apt-get &>/dev/null; then
    sudo apt-get install -y make build-essential
  elif command -v dnf &>/dev/null; then
    sudo dnf install -y make
  elif command -v pacman &>/dev/null; then
    sudo pacman -Sy --noconfirm make
  fi
}

setup_secrets() {
  local secrets_file
  secrets_file="$(dirname "$0")/esphome/secrets.yaml"
  if [[ ! -f "$secrets_file" ]]; then
    info "Erstelle secrets.yaml aus Vorlage..."
    cp "$(dirname "$0")/esphome/secrets.yaml.example" "$secrets_file"
    warn "Bitte esphome/secrets.yaml mit deinen WLAN-Daten ausfüllen!"
  else
    success "secrets.yaml vorhanden"
  fi
}

echo ""
echo "================================================"
echo "  Berbel ESP32 - Setup"
echo "================================================"
echo ""

OS=$(detect_os)
info "Betriebssystem: $OS"

if ! check_git; then
  warn "git nicht gefunden, installiere..."
  [[ "$OS" == "macos" ]] && install_git_macos || install_git_linux
fi
success "git: $(git --version)"

if ! check_make; then
  warn "make nicht gefunden, installiere..."
  [[ "$OS" == "macos" ]] && install_make_macos || install_make_linux
fi
success "make: $(make --version | head -1)"

if ! check_python; then
  warn "Python $PYTHON_MIN_VERSION+ nicht gefunden, installiere..."
  [[ "$OS" == "macos" ]] && install_python_macos || install_python_linux
fi
success "Python: $(python3 --version)"

if ! check_esphome; then
  warn "ESPHome nicht gefunden oder zu alt, installiere..."
  install_esphome
fi
success "ESPHome: $(esphome version 2>/dev/null | head -1)"

setup_secrets

echo ""
echo "================================================"
echo "  Setup abgeschlossen!"
echo ""
echo "  Nächste Schritte:"
echo "  1. esphome/secrets.yaml mit WLAN-Daten befüllen"
echo "  2. make build-esp32dev   (Firmware kompilieren)"
echo "  3. make flash-esp32dev   (Firmware flashen)"
echo "================================================"
echo ""
