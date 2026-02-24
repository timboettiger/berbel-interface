#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
VERSION_FILE="$ROOT_DIR/version.txt"
VERSION_H="$ROOT_DIR/esphome/version.h"

BASE_VERSION="$(cat "$VERSION_FILE" | tr -d '[:space:]')"
MAJOR="$(echo "$BASE_VERSION" | cut -d. -f1)"
MINOR="$(echo "$BASE_VERSION" | cut -d. -f2)"

COMMIT_COUNT="$(git -C "$ROOT_DIR" rev-list --count HEAD 2>/dev/null || echo 0)"
PATCH="$COMMIT_COUNT"

COMMIT_HASH="$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo 000000)"

FULL_VERSION="${MAJOR}.${MINOR}.${PATCH}"
BUILD_DATE="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

cat > "$VERSION_H" << EOF
#pragma once
#define BERBEL_VERSION_MAJOR ${MAJOR}
#define BERBEL_VERSION_MINOR ${MINOR}
#define BERBEL_VERSION_PATCH ${PATCH}
#define BERBEL_VERSION "${FULL_VERSION}"
#define BERBEL_COMMIT_HASH "${COMMIT_HASH}"
#define BERBEL_BUILD_DATE "${BUILD_DATE}"
EOF

echo "$FULL_VERSION+$COMMIT_HASH"
