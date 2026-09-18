#include <HardwareSerial.h>

#define E32_BAUD       9600
#define E32_RX_PIN     16
#define E32_TX_PIN     17
#define E32_M0_PIN     18
#define E32_M1_PIN     19
#define E32_AUX_PIN    21

#define MAX_RADIO_PAYLOAD 200
#define USB_BAUD 115200

HardwareSerial E32Serial(2);

String usbBuffer;
String radioBuffer;

String lastRx = "";
unsigned long rxCount = 0;

bool waitAux(unsigned long timeoutMs = 2000) {
  unsigned long start = millis();
  while (digitalRead(E32_AUX_PIN) == LOW) {
    if (millis() - start >= timeoutMs) return false;
    delay(1);
  }
  return true;
}

void setNormalMode() {
  digitalWrite(E32_M0_PIN, LOW);
  digitalWrite(E32_M1_PIN, LOW);
  delay(20);
  if (!waitAux(3000)) {
    Serial.println("ERR E32 AUX timeout");
  }
}

bool sendRadio(const String &payload) {
  if (payload.length() == 0 || payload.length() > MAX_RADIO_PAYLOAD) return false;
  if (!waitAux()) return false;

  E32Serial.print(payload);
  E32Serial.write('\n');
  E32Serial.flush();
  delay(2);
  return waitAux();
}

void processUsbCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.startsWith("SEND ")) {
    String payload = cmd.substring(5);
    if (payload.length() == 0 || payload.length() > MAX_RADIO_PAYLOAD) {
      Serial.println("RES SEND 0");
      return;
    }
    bool ok = sendRadio(payload);
    Serial.print("RES SEND ");
    Serial.println(ok ? 1 : 0);
    return;
  }

  if (cmd == "PING") {
    bool ok = sendRadio("PING");
    Serial.print("RES PING ");
    Serial.println(ok ? 1 : 0);
    return;
  }

  if (cmd == "GET_AUX") {
    Serial.print("RES GET_AUX ");
    Serial.println(digitalRead(E32_AUX_PIN) == HIGH ? 1 : 0);
    return;
  }

  if (cmd == "GET_RX_COUNT") {
    Serial.print("RES GET_RX_COUNT ");
    Serial.println(rxCount);
    return;
  }

  if (cmd == "GET_LAST_RX") {
    Serial.print("RES GET_LAST_RX ");
    Serial.println(lastRx);
    return;
  }

  if (cmd == "HELP") {
    Serial.println("Commands:");
    Serial.println("  SEND <text>");
    Serial.println("  PING");
    Serial.println("  GET_AUX");
    Serial.println("  GET_RX_COUNT");
    Serial.println("  GET_LAST_RX");
    return;
  }

  Serial.print("ERR Comando desconhecido: ");
  Serial.println(cmd);
}

void processUsb() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processUsbCommand(usbBuffer);
      usbBuffer = "";
    } else if (c != '\r') {
      if (usbBuffer.length() < 256) usbBuffer += c;
    }
  }
}

void processRadio() {
  while (E32Serial.available()) {
    char c = E32Serial.read();
    if (c == '\n') {
      radioBuffer.trim();
      if (radioBuffer.length() > 0) {
        lastRx = radioBuffer;
        rxCount++;
        Serial.print("EVT RX ");
        Serial.println(radioBuffer);
      }
      radioBuffer = "";
    } else if (c != '\r') {
      if (radioBuffer.length() < MAX_RADIO_PAYLOAD) radioBuffer += c;
    }
  }
}

void setup() {
  Serial.begin(USB_BAUD);
  pinMode(E32_M0_PIN, OUTPUT);
  pinMode(E32_M1_PIN, OUTPUT);
  pinMode(E32_AUX_PIN, INPUT);
  digitalWrite(E32_M0_PIN, LOW);
  digitalWrite(E32_M1_PIN, LOW);
  E32Serial.begin(E32_BAUD, SERIAL_8N1, E32_RX_PIN, E32_TX_PIN);
  delay(500);
  setNormalMode();
  Serial.println("LoRa Sender Ready!");
}

void loop() {
  processUsb();
  processRadio();
  delay(1);
}
