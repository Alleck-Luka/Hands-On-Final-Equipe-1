#define MAX_MESSAGE 200
#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "TIM_ULTRAFIBRA_321C_2G";
const char* password = "9Daw5ZvPiU";

// Public MQTT Broker (Free for testing)
const char* mqtt_server = "broker.hivemq.com"; 

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

int rx_count = 0;
char last_rx[256] = "";

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// This function runs automatically whenever a message is RECEIVED
void callback(char* topic, byte* payload, unsigned int length) {
  char message_buffer[256];
  snprintf(message_buffer, sizeof(message_buffer), "Message arrived [%s]: ", topic);
  snprintf(last_rx, sizeof(last_rx), "%s", message_buffer);
  snprintf(last_rx + strlen(last_rx), sizeof(last_rx) - strlen(last_rx), "%.*s", length, payload);
  rx_count++;
}

void reconnect() {
  while (!client.connected()) {
    // Attempt to connect with a random client ID
    String clientId = "ESP32Client-" + String(random(0, 1000));
    if (client.connect(clientId.c_str())) {
      // SUBSCRIBE to a topic to receive messages
      client.subscribe("LoRaComm207407/commands");
      client.publish("LoRaComm207407/status", "ESP32 connected to MQTT Broker!");
    } else {
      delay(5000);
    }
  }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    delay(1000);
}


void handle_command(String command) {

    // SEND <mensagem>
    if (command.startsWith("SEND")) {
        String message = command.substring(5);

        // Aqui, em um dispositivo real:
        // LoRa.send(message);
        client.publish("LoRaComm207407/responses", message.c_str());
        Serial.print("RES SEND OK ");
        Serial.println(message);

        return;
    }

    // PING
    if (command == "PING") {

        // Aqui, em um dispositivo real:
        // LoRa.send("PING");

        client.publish("LoRaComm207407/responses", "PONG");
        Serial.println("RES PING PONG");

        return;
    }

    // GET_RX_COUNT
    if (command == "GET_RX_COUNT") {

        Serial.print("RES GET_RX_COUNT ");
        Serial.println(rx_count);

        return;
    }

    // GET_LAST_RX
    if (command == "GET_LAST_RX") {

        Serial.print("RES GET_LAST_RX ");
        Serial.println(last_rx);

        return;
    }

    // GET_AUX
    if (command == "GET_AUX") {
      if(random(0, 2)) {
          Serial.println("RES GET_AUX RX");
      } else {
          Serial.println("RES GET_AUX TX");
      }
      return;
    }

    Serial.println("RES UNKNOWN ERROR");
}

void receiveMsg() {

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    handle_command(command);
  }
  client.loop(); // Keeps the connection alive and handles incoming messages
}
