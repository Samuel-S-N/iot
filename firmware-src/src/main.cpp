// NexaSense v1 — FIRMWARE EDUCACIONAL COM FRAGILIDADES INTENCIONAIS.
// Uso exclusivo em laboratório (Wokwi). Valores fictícios / credenciais públicas de teste.
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <HTTPUpdate.h>
#include "commands.h"

// ---- Configuração de fábrica embutida no binário (F01) ----
#define FW_VERSION  "1.0.3-lab"
#define DEVICE_ID   "nx-4b9e21"
#define WIFI_SSID   "Wokwi-GUEST"
#define WIFI_PASS   ""
#define MQTT_HOST   "test.mosquitto.org"
#define MQTT_PORT   1884
#define MQTT_USER   "rw"
#define MQTT_PASS   "readwrite"
#define API_TOKEN   "NXH-LAB-7731-ALPHA-SECRET"
#define OTA_URL     "http://updates.nexahome.lab/nexasense/latest.bin"
#define DHT_PIN     15

DHT dht(DHT_PIN, DHT22);
WiFiClient net;              // TCP puro, sem TLS (F03)
PubSubClient mqtt(net);

unsigned long intervalMs = 5000;
unsigned long lastPub = 0;
bool debugMode = true;       // debug ligado por padrão (F02)
const String topicTel = String("nexahome/") + DEVICE_ID + "/telemetry";
const String topicCmd = String("nexahome/") + DEVICE_ID + "/cmd";

void dumpConfig() {
  Serial.println("[DEBUG] ===== CONFIG DUMP =====");
  Serial.printf("[DEBUG] device_id=%s fw=%s\n", DEVICE_ID, FW_VERSION);
  Serial.printf("[DEBUG] wifi_ssid=%s wifi_pass=%s\n", WIFI_SSID, WIFI_PASS);
  Serial.printf("[DEBUG] mqtt=%s:%d user=%s pass=%s\n", MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
  Serial.printf("[DEBUG] api_token=%s\n", API_TOKEN);
  Serial.printf("[DEBUG] ota_url=%s\n", OTA_URL);
  Serial.printf("[DEBUG] interval_ms=%lu\n", intervalMs);
}

void runOta(const std::string& url) {
  // F05: HTTP puro, sem assinatura, sem checagem de versão (anti-rollback)
  Serial.printf("[OTA] baixando %s\n", url.c_str());
  t_httpUpdate_return r = httpUpdate.update(net, String(url.c_str()));
  Serial.printf("[OTA] resultado=%d %s\n", (int)r, httpUpdate.getLastErrorString().c_str());
}

void handleCommand(const Command& c) {
  switch (c.type) {
    case CmdType::SetInterval: intervalMs = c.value * 1000UL;
      Serial.printf("[CMD] intervalo=%lds\n", c.value); break;
    case CmdType::DebugOn:  debugMode = true;  Serial.println("[CMD] debug=on");  dumpConfig(); break;
    case CmdType::DebugOff: debugMode = false; Serial.println("[CMD] debug=off"); break;
    case CmdType::Reboot:   Serial.println("[CMD] reboot"); delay(200); ESP.restart(); break;
    case CmdType::Ota:      runOta(c.arg); break;
    default:                Serial.println("[CMD] desconhecido"); break;
  }
}

void onMqtt(char* topic, byte* payload, unsigned int len) {
  // F04: nenhum controle de origem, assinatura ou autorização
  std::string msg((const char*)payload, len);
  Serial.printf("[MQTT] %s <- %s\n", topic, msg.c_str());
  handleCommand(parseCommand(msg));
}

void connectWifi() {
  Serial.printf("[BOOT] conectando Wi-Fi SSID=%s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.printf("\n[BOOT] IP=%s\n", WiFi.localIP().toString().c_str());
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.printf("[BOOT] MQTT %s:%d user=%s\n", MQTT_HOST, MQTT_PORT, MQTT_USER);
    if (mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASS)) {
      mqtt.subscribe(topicCmd.c_str());
      Serial.printf("[BOOT] inscrito em %s\n", topicCmd.c_str());
    } else {
      Serial.printf("[BOOT] falha rc=%d, nova tentativa em 2s\n", mqtt.state());
      delay(2000);
    }
  }
}

void serialConsole() {
  // Console de manutenção sem autenticação (F02)
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line == "config") dumpConfig();
  else if (line == "help") Serial.println("[CONSOLE] comandos: config, help");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.printf("[BOOT] NexaSense fw=%s device=%s\n", FW_VERSION, DEVICE_ID);
  Serial.printf("[BOOT] MAC=%s\n", WiFi.macAddress().c_str());
  if (debugMode) dumpConfig();
  dht.begin();
  connectWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);
  connectMqtt();
}

void loop() {
  if (!mqtt.connected()) connectMqtt();
  mqtt.loop();
  serialConsole();
  if (millis() - lastPub >= intervalMs) {
    lastPub = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    char json[200];
    snprintf(json, sizeof json,
      "{\"device_id\":\"%s\",\"temp\":%.1f,\"hum\":%.1f,\"fw\":\"%s\",\"token\":\"%s\"}",
      DEVICE_ID, t, h, FW_VERSION, API_TOKEN);
    mqtt.publish(topicTel.c_str(), json);
    if (debugMode) Serial.printf("[PUB] %s %s\n", topicTel.c_str(), json);
  }
}
