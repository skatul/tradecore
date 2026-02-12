#!/usr/bin/env bash
# Run the tradecore server
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY="$SCRIPT_DIR/../build/tradecore"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found. Build first: cmake --build build"
    exit 1
fi

exec "$BINARY" "${1:-tcp://*:5555}"
