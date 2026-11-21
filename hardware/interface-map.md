# Mapa de Interfaces — NexaSense v1

O protótipo roda num ESP32-DevKitC (módulo ESP32-WROOM-32) simulado no Wokwi. As interfaces abaixo vêm do datasheet do módulo e do esquema do DevKitC. A análise é de simulação: não houve medição com multímetro, osciloscópio ou analisador lógico.

## 1. Interfaces do ESP32-DevKitC

| Interface | Pinos | Função legítima | Exposição no protótipo | O que permite | Risco em produção |
|---|---|---|---|---|---|
| UART0 | TX0=GPIO1, RX0=GPIO3 (ponte USB-serial CP2102) | Log e gravação em fábrica | Log de boot verboso + console `config` sem senha | Ler segredos no boot (F02); com GPIO0 em LOW no reset, entrar no ROM download mode | Alto: dump da flash sem Flash Encryption |
| JTAG | TDI=GPIO12, TCK=GPIO13, TMS=GPIO14, TDO=GPIO15 | Depuração on-chip | Pinos no header do DevKit, sem eFuse de bloqueio | Depurar com OpenOCD, ler/escrever memória, contornar checagens | Alto: controle total do SoC |
| Boot / strapping | GPIO0 (botão BOOT), EN (botão RESET) | Selecionar modo de boot | Botões expostos | Forçar o download mode da ROM | Médio: habilita o ataque pela UART |
| SPI Flash | GPIO6–11, internos ao módulo | Armazenar firmware e dados | Sob a blindagem metálica do WROOM-32 | Leitura física do chip (precisa remover a blindagem) | Médio: exige acesso físico e retrabalho |
| Sensor DHT22 | Dados no GPIO15 (1-wire) | Ler temperatura e umidade | Pino no header | Injetar leituras falsas com acesso físico | Baixo: afeta só a telemetria |
| Alimentação | USB 5 V → regulador LDO 3,3 V | Energia do SoC e do sensor | Conector USB | Ponto para glitching de tensão | Médio: habilita fault injection |

## 2. Conflito do GPIO15

O GPIO15 acumula três papéis: dados do DHT22, TDO do JTAG e pino de strapping da ROM. No protótipo ele está ligado ao sensor. Num produto, usar esse pino para o sensor conflita com o uso do JTAG e pode alterar o comportamento de boot. É um sinal de projeto apressado, típico de protótipo que vira produto sem revisão.

## 3. Análise contextual (UART)

A interface UART0 permanece acessível, fornece o log completo de boot e um console de manutenção, e não possui autenticação nem filtro de dados sensíveis. Considerando um atacante com acesso físico ao aparelho, isso pode permitir ler as credenciais e o token no boot, acionar o comando `config` para repetir o despejo, e, segurando o BOOT no reset, colocar o chip em download mode para extrair a flash inteira. O impacto é a exposição de toda a configuração de fábrica, que é a mesma em toda a frota (ver F01).

## 4. Acessível não é o mesmo que vulnerável

A UART é legítima: a fábrica precisa dela para gravar e testar. Ela só vira achado por três fatos somados:

| Fato | Evidência |
|---|---|
| O log imprime dados sensíveis | `hardware/evidence/boot-log.txt`, `uart-boot.png` |
| O console não pede autenticação | `hardware/evidence/boot-log.txt` (comando `config`) |
| O download mode não tem Flash Encryption | `firmware/evidence/image_info.txt` (sem bloco de assinatura, `Secure version: 0`) |

Interface aberta por si só é superfície. O risco aparece quando a interface entrega segredo, não barra o operador e não protege o conteúdo da flash.

## 5. Limitações

- Simulação. Os pinos vêm do datasheet do ESP32-WROOM-32 e do esquema do DevKitC, não de medição.
- A lógica é 3,3 V. Num teste físico, um adaptador USB-TTL de 5 V danifica o chip.
- A aquisição da flash foi lógica (imagem do build). Não houve leitura do chip SPI.

## Fontes

- Espressif. *ESP32-WROOM-32 Datasheet*. https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf
- Espressif. *ESP32-DevKitC V4 — Getting Started Guide*. https://docs.espressif.com/projects/esp-idf/en/stable/esp32/hw-reference/esp32/get-started-devkitc.html
