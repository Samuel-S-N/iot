# Análise do Firmware — NexaSense v1

## 1. Identificação e preservação

| Campo | Valor |
|---|---|
| Arquivo | `firmware_original.bin` (somente leitura, modo 444) |
| Tamanho | 4.194.304 bytes (imagem completa da flash de 4 MB) |
| Origem | Build PlatformIO (espressif32@6.9.0, arduino-esp32 / ESP-IDF v4.4.7), consolidado com `esptool merge_bin` |
| Data | 2026-11-20 21:36:55 -0300 |
| SHA-256 | `39c484e40072e36392ee1c602aa686c028466266dd241b6f955e5e4eb03b3de4` |

Fluxo seguido:

```
firmware_original.bin ──► SHA-256 registrado (hashes.txt / firmware_original.sha256)
        │
        └──► cópia de trabalho firmware/work/firmware_analise.bin (cmp = idêntica)
                     │
                     └──► análise (binwalk, strings, partições, image_info)
```

Toda execução de `lab/analyze-firmware.sh` confere o hash antes de começar. O `build-firmware.sh` se recusa a sobrescrever o original.

## 2. Formato e estrutura

`file` retorna só `data`, e o `binwalk` (`evidence/binwalk.txt`) não encontra nenhum sistema de arquivos. É o esperado: o ESP32 não roda Linux, então não há SquashFS ou JFFS2. A imagem é bootloader + tabela de partições + aplicação monolítica. O binwalk acha caminhos de build, constantes SHA-256 e a tabela Base64, mas não acha estrutura de container.

Mapa da flash (offsets do `merge_bin` + `evidence/partitions.txt`):

| Offset | Tamanho | Conteúdo | Observação |
|---|---|---|---|
| 0x1000 | — | Bootloader de 2º estágio | Sem assinatura (Secure Boot desligado) |
| 0x8000 | 3 KB | Tabela de partições | Legível em texto claro |
| 0x9000 | 20 KB | `nvs` | Parâmetros e calibração. **Não criptografada** |
| 0xE000 | 8 KB | `otadata` | Indica qual app inicia |
| 0x10000 | 1280 KB | `app0` (ota_0) | Aplicação NexaSense em execução |
| 0x150000 | 1280 KB | `app1` (ota_1) | Slot B da atualização (esquema A/B) |
| 0x290000 | 1408 KB | `spiffs` | Sistema de arquivos de dados (vazio) |
| 0x3F0000 | 64 KB | `coredump` | Core dump em caso de pane: pode conter RAM com segredos |

O par `app0`/`app1` + `otadata` é o particionamento A/B da Unidade 2.4. A estrutura para atualização com rollback existe, mas sem verificação de autenticidade (ver F05).

## 3. Cabeçalho da aplicação (`evidence/image_info.txt`)

| Campo | Valor | Leitura de segurança |
|---|---|---|
| Entry point | 0x40082C90 | IRAM. Ponto de partida para engenharia reversa no Ghidra (Xtensa, little-endian) |
| Segmentos | 5 (DROM, DRAM, IROM, DRAM, IRAM) | Strings ficam em DROM (0x3F400020), mapeado da flash |
| Checksum / Validation hash | válidos | Só **integridade** (detecta corrupção). Não prova autenticidade |
| Bloco de assinatura | ausente | Qualquer imagem com hash correto é aceita |
| `Secure version` | 0 | Anti-rollback não configurado |
| ESP-IDF | v4.4.7-dirty | Versão identificável: permite cruzar com CVEs do framework |

## 4. Bibliotecas identificadas

Caminhos de build no binário (`evidence/binwalk.txt`) entregam a pilha usada:

| Biblioteca | Evidência | Relevância |
|---|---|---|
| WiFi (arduino-esp32) | `.../libraries/WiFi/src/WiFiClient.cpp` | `WiFiClient` = TCP sem TLS |
| HTTPUpdate / HTTPClient / Update | `.../HTTPUpdate.cpp`, `Updater.cpp`, string `ESP32-http-Update` | Mecanismo de OTA presente |
| PubSubClient | formato do tópico + strings MQTT | Cliente MQTT 3.1.1 |
| DHT | leitura no GPIO15 | Sensor ambiental |

Os caminhos também expõem o usuário e o diretório da máquina de build (`/home/<usuário>/.platformio/...`). É um vazamento menor de informação do ambiente de desenvolvimento.

## 5. Informações sensíveis encontradas

Offsets na imagem de flash (`evidence/secret-offsets.txt`, valores mascarados):

| Offset | String | Significado | Achado |
|---|---|---|---|
| 0x011338 | `CONFIG DUMP` | Rotina que imprime toda a configuração na UART | F02 |
| 0x01137A | `Wokwi-GUEST` | SSID da rede | F01 |
| 0x0113A9 | `****` | Senha do broker MQTT | F01 |
| 0x0113B6 | `test.mosquitto.org` | Endpoint MQTT (porta 1884, sem TLS) | F03 |
| 0x0113ED | `NXH-LAB-****` | Token de API do dispositivo | F01 |
| 0x011424 | `http://updates.nexahome.lab/...` | URL de atualização em **HTTP** | F05 |

Outras strings relevantes (`evidence/strings-interesting.txt`):

| String | Leitura |
|---|---|
| `[DEBUG] mqtt=%s:%d user=%s pass=%s` | Formato que imprime a senha no log (F02) |
| `[DEBUG] api_token=%s` | Formato que imprime o token no log (F02) |
| `{"device_id":"%s",...,"token":"%s"}` | O token vai junto na telemetria (F03) |
| `nexahome/` + `nx-4b9e21` | Estrutura de tópicos previsível (F04) |
| `debug_on`, `debug_off` | Comandos remotos aceitos (F04) |
| `1.0.3-lab` | Versão do firmware |

## 6. Hipótese × confirmação

Achar `pass=` ou um token no binário é só hipótese (Unidade 2.3). Para virar achado, falta provar que o segredo é **usado**, é **compartilhado** e **protege acesso**:

| Hipótese | Como foi confirmada |
|---|---|
| Senha MQTT é usada | Aparece no pacote CONNECT capturado (`network/evidence/mqtt-fields.txt`) |
| Credencial é da frota, não do aparelho | Constante de compilação: a mesma imagem vai para todas as unidades |
| Credencial dá acesso ao controle | Usada na demonstração F04 para publicar comandos aceitos (`network/evidence/demo-f04.txt`) |

## 7. Limitações

- A aquisição foi **lógica**: imagem do build, equivalente a um `esptool read_flash`. Não houve leitura física de chip SPI.
- Sem descompilação no Ghidra (não exigida). O entry point e o mapa de segmentos acima já permitem carregar a imagem como Xtensa LE.
- A partição `nvs` está vazia na imagem de fábrica. Em um aparelho em campo, ela guardaria credenciais de Wi-Fi do cliente em texto claro (sem `CONFIG_NVS_ENCRYPTION`).
