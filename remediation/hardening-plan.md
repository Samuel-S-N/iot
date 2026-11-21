# Plano de Hardening — NexaSense

Proposta de correção dos achados F01–F06. **Nada aqui foi executado**: são medidas para a versão de produto, com os trechos de configuração do ESP-IDF e do broker. A matriz do fim liga cada medida ao achado e à referência.

## 1. Prioridades

| Prioridade | Quando | Achados | Por quê |
|---|---|---|---|
| P1 | Antes do piloto | F04, F03, F01 | Controle remoto e vazamento de credencial são exploráveis só com acesso à rede |
| P2 | Antes do lançamento | F05, F02 | Exigem posição de rede ou acesso físico, mas o impacto é total |
| P3 | Linha de produção | F06 + processo | Dependem de acesso físico e de mudança na fábrica |

## 2. Medidas por camada

Todas marcadas como **propostas, não executadas**.

### Comunicação (F03, F04)

- TLS no cliente: trocar `WiFiClient` por `WiFiClientSecure`, fixar a CA com `setCACert()` e usar a porta 8883.
- ACL por aparelho no broker. Exemplo de padrão no Mosquitto, que restringe cada aparelho ao seu próprio tópico:
  ```
  pattern readwrite nexahome/%c/#
  ```
  Assim o `client_id` de um aparelho não publica no tópico de outro.

### Identidade (F01)

- Credencial ou certificado X.509 único por aparelho, gerado na linha de produção.
- Guardar em partição NVS criptografada: `CONFIG_NVS_ENCRYPTION=y`.
- Nenhum segredo embutido em tempo de compilação.

### Comandos (F04)

- HMAC-SHA256 por aparelho em cada comando, com contador monotônico persistido em NVS para barrar replay.
- Faixa válida de `set_interval` (ex.: 5 a 3600 s), rejeitando valores fora do limite.

### Firmware e atualização (F05)

- `CONFIG_SECURE_BOOT=y` e `CONFIG_SECURE_BOOT_V2_ENABLED=y` (ESP32 ECO3 ou superior).
- Assinatura da imagem no CI, com a chave privada em HSM.
- OTA por HTTPS, com validação da CA.
- Anti-rollback e A/B com rollback:
  ```
  CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK=y
  CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
  ```

### Armazenamento (F01, F06)

- `CONFIG_SECURE_FLASH_ENC_ENABLED=y` com `CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y`.
- Secure Boot e Flash Encryption são complementares: um garante **autenticidade** (só roda firmware assinado), o outro garante **confidencialidade** (a flash não é legível em claro). Produto precisa dos dois.

### Interfaces físicas (F02, F06)

- JTAG desabilitado por eFuse em release (o Secure Boot/Flash Encryption já faz parte disso).
- UART download mode seguro: `CONFIG_SECURE_UART_ROM_DL_MODE=y`.
- Nível de log em release: `CONFIG_LOG_DEFAULT_LEVEL_WARN`.
- Console `config` removido da build de produção.

### Resiliência (relacionada ao caso real)

- Brown-out detector e watchdog ativos.
- Verificação redundante de decisões críticas, para reduzir o efeito de fault injection (ver CVE-2019-17391).
- Preferir silício ECO3 (ESP32-D0WD-V3) ou ESP32-S3, que trazem proteções de ROM contra o glitching do CVE-2019-17391.

## 3. Processo

- Provisionamento seguro na fábrica: gerar e gravar a chave única por aparelho em ambiente controlado.
- Gestão de chaves: chave de assinatura em HSM, rotação e revogação definidas.
- Política de vulnerabilidade e ponto de contato (ETSI EN 303 645 §5.2).
- Reteste após as correções, repetindo a avaliação antes do piloto.

## 4. Rastreabilidade

| Achado | Medida principal | Referência |
|---|---|---|
| F01 | Credencial única por aparelho + NVS criptografada | EMB3D TID-328; NIST 8259A (Device Identification); ETSI §5.1, §5.4 |
| F02 | Log mínimo + console removido/autenticado | EMB3D TID-115; NIST 8259A (Logical Access); ETSI §5.6 |
| F03 | MQTT sobre TLS 1.2+ | EMB3D TID-408; NIST 8259A (Data Protection); ETSI §5.5 |
| F04 | ACL + HMAC com contador + limite de parâmetros | EMB3D TID-406/TID-407; NIST 8259A (Logical Access); ETSI §5.5 |
| F05 | Secure Boot v2 + imagem assinada + HTTPS + anti-rollback | EMB3D TID-211/TID-215; NIST 8259A (Software Update); ETSI §5.3, §5.7 |
| F06 | Flash Encryption + JTAG por eFuse + download mode seguro | EMB3D TID-119/TID-115; NIST 8259A (Logical Access); ETSI §5.6 |

## Fontes

- Espressif. *ESP-IDF Security Guide / Secure Boot v2 / Flash Encryption*. https://docs.espressif.com/projects/esp-idf/en/stable/esp32/security/security.html
- MITRE. *EMB3D Threat Model*. https://emb3d.mitre.org/threats/
- NIST. *IR 8259A*. https://csrc.nist.gov/pubs/ir/8259/a/final
- ETSI. *EN 303 645*. https://www.etsi.org/deliver/etsi_en/303600_303699/303645/03.01.03_60/en_303645v030103p.pdf
- NVD. *CVE-2019-17391*. https://nvd.nist.gov/vuln/detail/CVE-2019-17391
