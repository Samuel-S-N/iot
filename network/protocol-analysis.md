# Análise de Comunicação — NexaSense v1

Base: `network/evidence/wokwi.pcap`, capturado no simulador, com os campos extraídos em `mqtt-fields.txt`, `dns.txt` e `tcp-conversations.txt`. Todos os segredos estão mascarados nas evidências.

## 1. Fluxo observado

```
DNS test.mosquitto.org → TCP 1884 → MQTT CONNECT → SUBSCRIBE .../cmd → PUBLISH .../telemetry (a cada 5 s)
```

| Passo | Pacote | Detalhe |
|---|---|---|
| Resolução de nome | DNS | `10.10.0.2` pergunta ao gateway `10.0.0.1`; resposta: `test.mosquitto.org = 54.36.178.49` |
| Conexão | TCP SYN | `10.10.0.2:62014 → 54.36.178.49:1884`; uma sessão, 149 pacotes em 211 s |
| Autenticação | MQTT CONNECT (tipo 1) | `client_id=nx-4b9e21`, `user=rw`, senha em claro no pacote |
| Inscrição | MQTT SUBSCRIBE (tipo 8) | tópico `nexahome/nx-4b9e21/cmd` |
| Telemetria | MQTT PUBLISH (tipo 3) | 43 publicações em `nexahome/nx-4b9e21/telemetry`, cada uma com o token |

## 2. Avaliação

| Item | Observado | Avaliação |
|---|---|---|
| Protocolo | MQTT 3.1.1 sobre TCP | Padrão de IoT, mas sem segurança por si só |
| Porta | 1884 | Porta de texto claro do broker de teste |
| Autenticação | usuário e senha compartilhados, em claro no CONNECT | Qualquer captura expõe a credencial |
| Criptografia | nenhuma — `pacotes TLS: 0` | Todo o tráfego é legível em trânsito |
| Identificadores | `client_id = device_id`, MAC no log, tópico previsível | Facilita enumerar e forjar |
| Dado trafegado | temperatura, umidade e **token** | O token não deveria ir na telemetria |
| Controle | `.../cmd` sem autenticação de origem nem integridade | Qualquer cliente publica comando aceito (F04) |

## 3. Identificar o protocolo não é o mesmo que uso seguro

Saber que é MQTT diz só *como* o dado trafega. A segurança depende de quatro camadas que aqui não existem:

| Camada | Função | Estado no protótipo |
|---|---|---|
| TLS | Confidencialidade e autenticação do servidor | Ausente |
| Credencial por dispositivo | Isolar o aparelho comprometido | Credencial única da frota |
| ACL por tópico | Autorização (quem publica onde) | Broker de teste aberto |
| Assinatura + contador no comando | Integridade e anti-replay | Ausente |

## 4. Replay e spoofing

O comando em `.../cmd` não tem nonce nem contador. Isso abre duas classes de ataque:

- **Replay:** uma captura do comando pode ser reenviada e será aceita de novo.
- **Spoofing:** qualquer cliente com a credencial publica no tópico, sem provar que é o operador legítimo.

A demonstração controlada F04 (`network/evidence/demo-f04.txt` e `demo-f04-serial.png`) confirma o spoofing: um comando forjado alterou o intervalo de telemetria e ligou o modo debug remotamente, usando só a credencial extraída do firmware.

## 5. Limitações

- Captura em simulação, no gateway público do Wokwi.
- O broker `test.mosquitto.org` é público e serve só de transporte de laboratório. Nenhum teste foi feito fora de `nexahome/nx-4b9e21/#`.

## Fontes

- OASIS. *MQTT Version 3.1.1*. https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
- Eclipse Mosquitto. *Test server (test.mosquitto.org)*. https://test.mosquitto.org/
