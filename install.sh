#!/bin/sh
# install.sh — Build and install x86_64-assembler
#
# Usage: ./install.sh [OPTIONS]
#   --prefix=DIR    Installation prefix (default: /usr/local)
#   --bindir=DIR    Binary directory (default: PREFIX/bin)
#   --includedir=DIR Header directory (default: PREFIX/include)
#   --uninstall     Remove installed files instead of installing
#   --help          Show this help message

set -e

PREFIX="/usr/local"
BINDIR=""
INCLUDEDIR=""
UNINSTALL=0

for arg in "$@"; do
    case "$arg" in
        --prefix=*)
            PREFIX="${arg#*=}"
            ;;
        --bindir=*)
            BINDIR="${arg#*=}"
            ;;
        --includedir=*)
            INCLUDEDIR="${arg#*=}"
            ;;
        --uninstall)
            UNINSTALL=1
            ;;
        --help)
            sed -n '2,9p' "$0"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            echo "Run with --help for usage." >&2
            exit 1
            ;;
    esac
done

if [ -z "$BINDIR" ]; then
    BINDIR="${PREFIX}/bin"
fi

if [ -z "$INCLUDEDIR" ]; then
    INCLUDEDIR="${PREFIX}/include"
fi

TARGET="bin/x86_64-asm"
HEADER_DIR="include/x86_64_asm"

if [ "$UNINSTALL" -eq 1 ]; then
    echo "Uninstalling x86_64-assembler..."

    if [ -f "${BINDIR}/x86_64-asm" ]; then
        echo "  Removing ${BINDIR}/x86_64-asm"
        rm -f "${BINDIR}/x86_64-asm"
    fi

    if [ -d "${INCLUDEDIR}/x86_64_asm" ]; then
        echo "  Removing ${INCLUDEDIR}/x86_64_asm"
        rm -rf "${INCLUDEDIR}/x86_64_asm"
    fi

    echo "Uninstall complete."
    exit 0
fi

echo "Building x86_64-assembler..."
make clean >/dev/null 2>&1 || true
make

if [ ! -f "$TARGET" ]; then
    echo "Error: Build failed — $TARGET not found." >&2
    exit 1
fi

echo "Installing to prefix: $PREFIX"
echo "  Binary:  $BINDIR"
echo "  Headers: $INCLUDEDIR"

mkdir -p "$BINDIR"
mkdir -p "$INCLUDEDIR"

echo "  Installing x86_64-asm -> ${BINDIR}/x86_64-asm"
cp "$TARGET" "${BINDIR}/x86_64-asm"
chmod 755 "${BINDIR}/x86_64-asm"

if [ -d "$HEADER_DIR" ]; then
    echo "  Installing headers -> ${INCLUDEDIR}/x86_64_asm"
    cp -r "$HEADER_DIR" "$INCLUDEDIR/"
    chmod -R 644 "${INCLUDEDIR}/x86_64_asm"/*
fi

echo "Install complete."
echo ""
echo "Usage: x86_64-asm <input.asm> -o <output>"
