#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
pandoc relatorio.md -f markdown -t html5 -s --css style.css --metadata pagetitle="NexaSense — Avaliacao de Seguranca IoT" -o relatorio.html
weasyprint relatorio.html relatorio.pdf
echo "OK: $(pwd)/relatorio.pdf"
