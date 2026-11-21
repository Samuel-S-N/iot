#!/usr/bin/env bash
# Triagem do firmware sobre a CÓPIA DE TRABALHO, depois de conferir a integridade do original.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMG="$ROOT/firmware/work/firmware_analise.bin"
OUT="$ROOT/firmware/evidence"
GEN="$HOME/.platformio/packages/framework-arduinoespressif32/tools/gen_esp32part.py"
mkdir -p "$OUT"

(cd "$ROOT/firmware" && sha256sum -c firmware_original.sha256)
cmp "$ROOT/firmware/firmware_original.bin" "$IMG" && echo "cópia de trabalho idêntica ao original"

file "$IMG" > "$OUT/file.txt"
binwalk "$IMG" > "$OUT/binwalk.txt" || true

dd if="$IMG" of="$ROOT/firmware/work/partitions.bin" bs=1 skip=$((0x8000)) count=3072 status=none
python3 "$GEN" "$ROOT/firmware/work/partitions.bin" > "$OUT/partitions.txt"

dd if="$IMG" of="$ROOT/firmware/work/app.bin" bs=4096 skip=16 status=none
(cd "$ROOT/firmware-src" && pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32 image_info --version 2 "$ROOT/firmware/work/app.bin") > "$OUT/image_info.txt" 2>&1 || true

strings -n 6 "$IMG" \
  | grep -iE 'pass|user|token|secret|mqtt|http|ota|debug|ssid|nexa|nx-|mosquitto|fw=|1\.0\.3' \
  | sort -u | "$ROOT/lab/mask.sh" > "$OUT/strings-interesting.txt"

# Offsets (na imagem de flash) dos parâmetros sensíveis embutidos — valor mascarado
: > "$OUT/secret-offsets.txt"
for s in Wokwi-GUEST test.mosquitto.org readwrite NXH-LAB updates.nexahome.lab "CONFIG DUMP"; do
  grep -boa -- "$s" "$IMG" | while IFS=: read -r off val; do
    printf '0x%06X  %s\n' "$off" "$val"
  done
done | "$ROOT/lab/mask.sh" >> "$OUT/secret-offsets.txt"

wc -l "$OUT"/*.txt
