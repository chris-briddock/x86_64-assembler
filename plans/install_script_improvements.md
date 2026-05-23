# Install Script Improvements

This document lists possible enhancements for [`install.sh`](../install.sh) in order of priority and complexity.

## High Priority

### 1. DESTDIR Support

Support `DESTDIR` for staged installs (required for package managers and distro builds).

```bash
DESTDIR=""
# ...
cp "$TARGET" "${DESTDIR}${BINDIR}/x86_64-asm"
```

Usage: `DESTDIR=/tmp/pkg ./install.sh --prefix=/usr`

### 2. Parallel Build

Use `make -j$(nproc)` by default, with an override flag.

```bash
JOBS="$(nproc 2>/dev/null || echo 4)"
make -j"$JOBS"
```

Add `--jobs=N` flag.

### 3. Dependency Checks

Verify build dependencies exist before starting:

- `make`
- `gcc` or `clang`
- Standard C library headers

Fail early with a helpful message if anything is missing.

### 4. Dry-Run Mode

Add `--dry-run` to print every action without executing it.

```bash
cp() { echo "  [dry-run] cp $*"; }
# ... override commands in dry-run mode
```

### 5. Backup on Overwrite

If a binary already exists at the target path, back it up before overwriting.

```bash
if [ -f "${BINDIR}/x86_64-asm" ]; then
    mv "${BINDIR}/x86_64-asm" "${BINDIR}/x86_64-asm.bak.$(date +%s)"
fi
```

## Medium Priority

### 6. Strip Binaries

Add `--strip` to run `strip` on the installed binary, reducing size.

```bash
if [ "$STRIP" -eq 1 ]; then
    strip "${BINDIR}/x86_64-asm"
fi
```

### 7. Ownership / Permissions Flags

Allow specifying owner and group for installed files:

```bash
--owner=root --group=root
```

Use `chown` when running as root or with `sudo`.

### 8. Verbose / Quiet Modes

Add `--verbose` and `--quiet` flags to control output level.

### 9. Check for Existing Installation

Warn or abort if the same version (or any version) is already installed.

```bash
if command -v x86_64-asm >/dev/null 2>&1; then
    echo "Warning: x86_64-asm already found at $(command -v x86_64-asm)"
fi
```

### 10. Install Completion Scripts

If bash/zsh/fish completions are added to the project, install them to the appropriate system directories.

```
/usr/share/bash-completion/completions/x86_64-asm
/usr/share/zsh/site-functions/_x86_64-asm
```

## Lower Priority

### 11. Man Page Installation

If a man page is written, install it:

```bash
mkdir -p "${PREFIX}/share/man/man1"
cp docs/x86_64-asm.1 "${PREFIX}/share/man/man1/"
gzip -f "${PREFIX}/share/man/man1/x86_64-asm.1"
```

### 12. Library Installation

If the project ever builds a static or shared library, install it to `${PREFIX}/lib` and run `ldconfig`.

### 13. pkg-config File

Generate and install a `.pc` file if the project exposes a library:

```
${PREFIX}/lib/pkgconfig/x86_64-asm.pc
```

### 14. Configuration File Support

Allow a `.installrc` or `install.conf` to set default flags per user or per machine.

### 15. Cross-Compilation Support

Respect environment variables like `CC`, `CFLAGS`, `LDFLAGS`, `AR`, and `RANLIB`.

```bash
CC="${CC:-gcc}"
```

### 16. Checksum Verification

Generate and optionally verify checksums of installed files:

```bash
sha256sum "${BINDIR}/x86_64-asm" > "${BINDIR}/x86_64-asm.sha256"
```

### 17. Version Flag

Add `--version` to print the project version and exit.

### 18. Install Only (Skip Build)

Add `--no-build` to skip `make` and install an already-built binary.

## Suggested Implementation Order

1. DESTDIR support
2. Parallel build (`-j`)
3. Dependency checks
4. Dry-run mode
5. Backup on overwrite
6. Strip option
7. Verbose / quiet modes
8. Ownership flags
9. Existing-installation warning
10. Completion / man page support (when those assets exist)
