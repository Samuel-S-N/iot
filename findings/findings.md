# Achados e Riscos — NexaSense v1

Seis achados (F01–F06), classificados por uma matriz de risco 3×3. Cada achado tem a cadeia Evidência → Fragilidade → Ameaça → Impacto → Probabilidade → Risco → Mitigação, com as referências conferidas.

## 1. Matriz de risco

Risco = Probabilidade × Impacto.

| Prob. \\ Impacto | Baixo | Médio | Alto |
|---|---|---|---|
| **Alta** | Médio | Alto | Alto |
| **Média** | Baixo | Médio | Alto |
| **Baixa** | Baixo | Baixo | Médio |

## 2. Tabela resumida

| ID | Achado | Evidência | Impacto | Prob. | Risco | Recomendação |
|---|---|---|---|---|---|---|
| F01 | Credenciais e token fixos e compartilhados | `firmware/evidence/strings-interesting.txt`, `secret-offsets.txt` | Alto | Média | **Alto** | Credencial única por aparelho, provisionada na fábrica e guardada em NVS criptografada |
| F02 | UART verbosa + console sem autenticação | `hardware/evidence/boot-log.txt`, `uart-boot.png` | Médio | Média | **Médio** | Log mínimo em release; console removido ou autenticado |
| F03 | MQTT sem TLS | `network/evidence/mqtt-fields.txt`, `wireshark-connect.png` | Alto | Alta | **Alto** | MQTT sobre TLS 1.2+ (8883) com validação de CA |
| F04 | Comando remoto sem autenticação | `network/evidence/demo-f04.txt`, `demo-f04-serial.png` | Alto | Alta | **Alto** | ACL por aparelho no broker + comando assinado com contador/nonce + limite de parâmetros |
| F05 | OTA por HTTP sem assinatura | `firmware/evidence/secret-offsets.txt`, `firmware-src/src/main.cpp` (`runOta`) | Alto | Média | **Alto** | Secure Boot v2 + imagem assinada + HTTPS + anti-rollback |
| F06 | JTAG e UART download mode em produção | `hardware/interface-map.md`, `firmware/evidence/image_info.txt` | Alto | Baixa | **Médio** | Flash Encryption (release), desabilitar JTAG por eFuse, download mode seguro |

## 3. Detalhamento

### F01 — Credenciais e token fixos e compartilhados

- **Evidência:** SSID, usuário e senha MQTT, token de API e URL de OTA aparecem como strings na flash (`firmware/evidence/strings-interesting.txt`, offsets em `secret-offsets.txt`) e no log de boot. A credencial se confirma em uso no pacote CONNECT (`network/evidence/mqtt-fields.txt`).
- **Fragilidade:** valores embutidos em tempo de compilação. A mesma imagem vai para toda a frota, então a credencial é igual em todos os aparelhos.
- **Ameaça:** extração do firmware por um aparelho (UART, dump físico ou OTA capturada) expõe o segredo de todos.
- **Impacto:** Alto. Comprometer um aparelho dá acesso à frota inteira.
- **Probabilidade:** Média. Exige obter uma imagem, mas qualquer unidade serve.
- **Risco:** Alto.
- **Mitigação:** credencial ou certificado X.509 único por aparelho, gerado na fábrica e guardado em NVS criptografada.
- **Referências:** EMB3D TID-328 (Hardcoded Credentials, CWE-798); NIST IR 8259A (Device Identification, Data Protection); ETSI EN 303 645 §5.1 (sem senhas universais padrão), §5.4 (proteção de parâmetros sensíveis).

### F02 — UART verbosa e console sem autenticação

- **Evidência:** `hardware/evidence/boot-log.txt` e `uart-boot.png`. O boot despeja a configuração (`CONFIG DUMP`), e o comando `config`, sem senha, repete o despejo.
- **Fragilidade:** `debugMode` ligado por padrão e console de manutenção aberto.
- **Ameaça:** atacante com acesso físico lê segredos pela serial e, pelo download mode da ROM, extrai a flash.
- **Impacto:** Médio. Exposição de segredos e configuração; habilita F01 e F06.
- **Probabilidade:** Média. Precisa de acesso físico à UART.
- **Risco:** Médio.
- **Mitigação:** nível de log mínimo em release, console removido ou atrás de autenticação.
- **Referências:** EMB3D TID-115 (Firmware/Data Extraction via Hardware Interface); NIST IR 8259A (Logical Access to Interfaces); ETSI EN 303 645 §5.6 (minimizar superfície de ataque).

### F03 — MQTT sem TLS

- **Evidência:** `network/evidence/mqtt-fields.txt` (usuário e senha em claro no CONNECT), `tls-count.txt` com `pacotes TLS: 0`, e `wireshark-connect.png`/`wireshark-publish.png`.
- **Fragilidade:** `WiFiClient` puro na porta 1884, sem criptografia.
- **Ameaça:** quem observa a rede lê credencial, token e telemetria, e pode interceptar ou alterar o tráfego.
- **Impacto:** Alto. Perda de confidencialidade da credencial e dos dados.
- **Probabilidade:** Alta. Basta estar no caminho da rede; sem nenhuma barreira criptográfica.
- **Risco:** Alto.
- **Mitigação:** MQTT sobre TLS 1.2+ (porta 8883) com validação da CA do servidor.
- **Referências:** EMB3D TID-408 (Unencrypted Sensitive Data Communication, CWE-319); NIST IR 8259A (Data Protection); ETSI EN 303 645 §5.5 (comunicação segura).

### F04 — Comando remoto sem autenticação

- **Evidência:** `network/evidence/demo-f04.txt` e `demo-f04-serial.png`. Um comando forjado em `nexahome/nx-4b9e21/cmd`, publicado só com a credencial da frota, mudou o intervalo de telemetria e ligou o modo debug remotamente.
- **Fragilidade:** o callback aceita qualquer mensagem no tópico, sem verificar origem, assinatura ou contador.
- **Ameaça:** spoofing (qualquer cliente publica comando aceito) e replay (uma captura do comando pode ser reenviada).
- **Impacto:** Alto. Controle remoto do comportamento do aparelho; `set_interval:1` serve de vetor de negação de serviço e `ota:` aponta para F05.
- **Probabilidade:** Alta. Demonstrado em laboratório com a credencial extraída.
- **Risco:** Alto.
- **Mitigação:** ACL por aparelho no broker, comando assinado com HMAC + contador monotônico (anti-replay) e faixa válida de parâmetros.
- **Referências:** EMB3D TID-406 (Unauthorized Messages or Connections, ATT&CK T0855) e TID-407 (Missing Message Replay Protection); NIST IR 8259A (Logical Access to Interfaces); ETSI EN 303 645 §5.5.

### F05 — OTA por HTTP sem assinatura

- **Evidência:** URL `http://updates.nexahome.lab/...` nas strings (`secret-offsets.txt`) e a função `runOta` em `firmware-src/src/main.cpp`, que chama `httpUpdate.update` sobre HTTP. O cabeçalho da aplicação não tem bloco de assinatura (`firmware/evidence/image_info.txt`, `Secure version: 0`).
- **Fragilidade:** atualização por HTTP puro, sem verificar assinatura nem versão.
- **Ameaça:** um atacante na rede (ou via F04, com `ota:`) entrega um firmware malicioso, que o aparelho instala e executa.
- **Impacto:** Alto. Execução de código persistente; comprometimento total.
- **Probabilidade:** Média. Exige posição de rede ou o canal de comando.
- **Risco:** Alto.
- **Mitigação:** Secure Boot v2, imagem assinada, OTA por HTTPS, anti-rollback e esquema A/B com rollback.
- **Referências:** EMB3D TID-211 (Unauthenticated Firmware Installation) e TID-215 (Unencrypted SW/FW Updates); NIST IR 8259A (Software Update); ETSI EN 303 645 §5.3 (atualização de software), §5.7 (integridade de software).

### F06 — JTAG e UART download mode em produção

- **Evidência:** `hardware/interface-map.md` (pinos JTAG no header, download mode via GPIO0) e `firmware/evidence/image_info.txt` (sem assinatura, sem indício de Flash Encryption).
- **Fragilidade:** interfaces de depuração ativas e flash sem criptografia num aparelho de campo.
- **Ameaça:** atacante com acesso físico usa JTAG (OpenOCD) para ler e escrever memória, ou entra no download mode e faz `esptool read_flash`.
- **Impacto:** Alto. Leitura e escrita arbitrária da flash e da memória.
- **Probabilidade:** Baixa. Exige acesso físico prolongado e ferramentas.
- **Risco:** Médio.
- **Mitigação:** Flash Encryption em release, JTAG desabilitado por eFuse, UART download mode seguro.
- **Referências:** EMB3D TID-119 (Latent Hardware Debug Port) e TID-115; NIST IR 8259A (Logical Access to Interfaces); ETSI EN 303 645 §5.6.

## 4. Cadeias de ataque

Os achados se combinam:

- **Comprometer a frota:** F02 (ler segredo pela UART) ou F06 (dump da flash) → F01 (credencial igual em todos) → F03/F04 (usar a credencial para ler dados e enviar comando a qualquer aparelho).
- **Firmware malicioso remoto:** F03 (observar a rede) + F04 (`ota:` forjado) → F05 (OTA sem assinatura instala o firmware do atacante).

## Fontes

- MITRE. *EMB3D Threat Model*. https://emb3d.mitre.org/threats/
- NIST. *IR 8259A — IoT Device Cybersecurity Capability Core Baseline*. https://csrc.nist.gov/pubs/ir/8259/a/final
- ETSI. *EN 303 645 — Cyber Security for Consumer IoT*. https://www.etsi.org/deliver/etsi_en/303600_303699/303645/03.01.03_60/en_303645v030103p.pdf
