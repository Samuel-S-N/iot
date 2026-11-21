#!/usr/bin/env bash
# Compila, consolida a imagem de flash (4 MB) e preserva o original com SHA-256.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
B="$ROOT/firmware-src/.pio/build/esp32dev"
ORIG="$ROOT/firmware/firmware_original.bin"
BOOT_APP0="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"

if [[ -e "$ORIG" ]]; then
  echo "ERRO: $ORIG já existe. O original não é sobrescrito." >&2
  echo "Para verificar: (cd firmware && sha256sum -c firmware_original.sha256)" >&2
  exit 1
fi

(cd "$ROOT/firmware-src" && pio run -e esp32dev)
mkdir -p "$ROOT/firmware/work"
(cd "$ROOT/firmware-src" && pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32 merge_bin \
  -o "$ORIG" --flash_mode dio --flash_size 4MB --fill-flash-size 4MB \
  0x1000 "$B/bootloader.bin" 0x8000 "$B/partitions.bin" 0xe000 "$BOOT_APP0" 0x10000 "$B/firmware.bin")
chmod 444 "$ORIG"

(cd "$ROOT/firmware" && sha256sum firmware_original.bin > firmware_original.sha256)
HASH=$(cut -d' ' -f1 "$ROOT/firmware/firmware_original.sha256")
SIZE=$(stat -c %s "$ORIG")
cat > "$ROOT/firmware/hashes.txt" <<EOT
Arquivo:  firmware_original.bin
Tamanho:  $SIZE bytes
Origem:   build local PlatformIO (espressif32@6.9.0, env esp32dev) consolidado com esptool merge_bin
          offsets 0x1000 bootloader | 0x8000 partições | 0xe000 boot_app0 | 0x10000 aplicação
Data:     $(date '+%Y-%m-%d %H:%M:%S %z')
SHA-256:  $HASH
EOT

cp "$ORIG" "$ROOT/firmware/work/firmware_analise.bin"
cp "$ORIG" "$ROOT/firmware/work/firmware_modificado.bin"
chmod 644 "$ROOT/firmware/work/"*.bin
cat "$ROOT/firmware/hashes.txt"
