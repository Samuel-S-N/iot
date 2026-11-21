# Roteiro do Vídeo — NexaSense (até 4 min)

Vídeo de apresentação da avaliação. Alvo: **4:00**. Gravar com os segredos borrados e subir no YouTube como **Não listado**.

## Roteiro

| Tempo | Tela | Fala (resumo) |
|---|---|---|
| 0:00–0:20 | README no editor | Quem sou, o trabalho e o objetivo: avaliar o protótipo NexaSense e mostrar o caminho ao produto seguro. |
| 0:20–0:45 | Wokwi + diagrama de arquitetura | O dispositivo: ESP32 + DHT22, publicando por MQTT. Mostrar o circuito e o diagrama. |
| 0:45–1:05 | Tabela da superfície de ataque (§4 do relatório) | As cinco superfícies: físico, firmware, atualização, rede e serviço. |
| 1:05–1:35 | Terminal: `sha256sum -c` + `strings-interesting.txt` | Firmware preservado por hash. As strings expõem credencial, token e URL de OTA. |
| 1:35–2:00 | Monitor serial do Wokwi | Hardware: boot despeja a config; comando `config` repete, sem senha. |
| 2:00–2:30 | Wireshark (CONNECT e PUBLISH) | Comunicação: credencial em claro, token na telemetria, zero TLS. |
| 2:30–3:15 | `demo-attack.sh` + monitor serial lado a lado | Demonstração F04 ao vivo: comando forjado acelera a telemetria e liga o debug. |
| 3:15–3:40 | Tabela F01–F06 (§10) | Os seis achados e o risco de cada um. |
| 3:40–4:00 | Tabela de hardening (§12) | O caminho: TLS, credencial por aparelho, comandos assinados, Secure Boot + Flash Encryption. |

## Checklist de gravação

- [ ] Gravador: OBS ou o gravador do sistema, 1080p.
- [ ] Senha MQTT e token **borrados** em toda a tela (serial, Wireshark, strings).
- [ ] Testar o áudio antes.
- [ ] Upload no YouTube como **Não listado**.
- [ ] Copiar o link para o README e para o relatório (§14), e regerar o PDF.
