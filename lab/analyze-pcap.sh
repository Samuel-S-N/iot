#!/usr/bin/env bash
# Extrai do PCAP do Wokwi os campos relevantes (MQTT na porta 1884 + DNS), mascarados.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PCAP="$ROOT/network/evidence/wokwi.pcap"
OUT="$ROOT/network/evidence"
T=(tshark -r "$PCAP" -d tcp.port==1884,mqtt -o mqtt.show_msg_as_text:TRUE)

"${T[@]}" -Y dns -T fields -e frame.time_relative -e ip.src -e ip.dst -e dns.qry.name -e dns.a > "$OUT/dns.txt"
"${T[@]}" -Y mqtt -T fields -E header=y -E separator=' | ' \
  -e frame.time_relative -e ip.src -e ip.dst -e mqtt.msgtype \
  -e mqtt.clientid -e mqtt.username -e mqtt.passwd -e mqtt.topic -e mqtt.msg_text \
  | "$ROOT/lab/mask.sh" > "$OUT/mqtt-fields.txt"
"${T[@]}" -q -z conv,tcp > "$OUT/tcp-conversations.txt"
"${T[@]}" -Y 'tls' | wc -l | sed 's/^/pacotes TLS: /' > "$OUT/tls-count.txt"
head -5 "$OUT/mqtt-fields.txt"; cat "$OUT/tls-count.txt"
