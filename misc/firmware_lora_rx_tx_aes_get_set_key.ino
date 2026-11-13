#include <HardwareSerial.h>
#include "mbedtls/aes.h"
 
#define E32_BAUD       9600     
#define E32_RX_PIN     16       
#define E32_TX_PIN     17      
#define E32_M0_PIN     18       
#define E32_M1_PIN     19      
#define E32_AUX_PIN    21       
 
#define MAX_RADIO_PAYLOAD 200
#define USB_BAUD 115200
 
// SENHA DE SEGURANÇA: Pode ter qualquer tamanho (ex: 5, 8 ou 16 caracteres)
// DEVE ser idêntica nas duas placas!
//const char* SECRET_KEY = "12345678";
char SECRET_KEY[17] = "12345678";
 
HardwareSerial E32Serial(2);
 
String usbBuffer;
String radioBuffer;
String lastRx = "";
unsigned long rxCount = 0;
 
// --- FUNÇÕES DE CRIPTOGRAFIA (AES-128 COM MODO SILENCIOSO) ---
 
// Ajusta a senha informada para o tamanho exato de 16 bytes exigido pelo AES
void prepareKey(uint8_t keyOut[16]) {
  memset(keyOut, 0, 16);
  int len = strlen(SECRET_KEY);
  if (len > 16) len = 16;
  memcpy(keyOut, SECRET_KEY, len);
}
 
String encryptAES(String plainText) {
  uint8_t key[16];
  prepareKey(key);
 
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key, 128);
 
  // Preenchimento PKCS7
  int len = plainText.length();
  int padLen = 16 - (len % 16);
  int totalLen = len + padLen;
 
  uint8_t input[totalLen];
  memcpy(input, plainText.c_str(), len);
  for (int i = len; i < totalLen; i++) {
    input[i] = padLen;
  }
 
  uint8_t output[totalLen];
  for (int i = 0; i < totalLen; i += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input + i, output + i);
  }
  mbedtls_aes_free(&aes);
 
  // Converte o bloco binário encriptado para String Hexadecimal
  String encryptedHex = "";
  for (int i = 0; i < totalLen; i++) {
    if (output[i] < 16) encryptedHex += "0";
    encryptedHex += String(output[i], HEX);
  }
  return encryptedHex;
}
 
String decryptAES(String hexText) {
  int len = hexText.length() / 2;
  if (len == 0 || len % 16 != 0) return ""; // Tamanho incompatível com bloco AES
 
  uint8_t input[len];
  for (int i = 0; i < len; i++) {
    String byteString = hexText.substring(i * 2, i * 2 + 2);
    input[i] = (uint8_t) strtol(byteString.c_str(), NULL, 16);
  }
 
  uint8_t key[16];
  prepareKey(key);
 
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, key, 128);
 
  uint8_t output[len];
  for (int i = 0; i < len; i += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, input + i, output + i);
  }
  mbedtls_aes_free(&aes);
 
  // Valida o preenchimento PKCS7
  int padLen = output[len - 1];
  if (padLen < 1 || padLen > 16) return ""; // Falha ao decifrar (Senha errada ou pacote inválido)
 
  int realLen = len - padLen;
  char decrypted[realLen + 1];
  memcpy(decrypted, output, realLen);
  decrypted[realLen] = '\0';
 
  return String(decrypted);
}
 
// --- LÓGICA DE HARDWARE DO E32 ---
 
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
 
  // Criptografa o payload antes do envio
  String cipherText = encryptAES(payload);
 
  E32Serial.print(cipherText);
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
 
  if (cmd == "GET_KEY") {
      Serial.print("RES GET_KEY ");
      Serial.println(SECRET_KEY);
      return;
  }
 
  if (cmd.startsWith("SET_KEY ")) {
      String k = cmd.substring(8);
      k.trim();
      if (k.length() > 0) {
          memset(SECRET_KEY, 0, sizeof(SECRET_KEY));
          strncpy(SECRET_KEY, k.c_str(), 16);
          Serial.println("RES SET_KEY 1");
      } else {
          Serial.println("RES SET_KEY 0");
      }
      return;
  }
 
  if (cmd == "PING") {
    bool ok = sendRadio("PING");
    Serial.print("RES PING ");
    Serial.println(ok ? 1 : 0);
    return;
  }
 
  if (cmd == "HELP") {
    Serial.println("Comandos:");
    Serial.println("  SEND <texto>");
    Serial.println("  PING");
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
 
        // Tenta descriptografar
        String plainText = decryptAES(radioBuffer);
 
        // MODO INVISÍVEL: Só emite mensagem se for decifrada com a senha correta.
        // Pacotes de terceiros ou com senha errada são descartados em absoluto silêncio.
        if (plainText.length() > 0) {
          lastRx = plainText;
          rxCount++;
          Serial.print("EVT RX ");
          Serial.println(plainText);
        }
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
  Serial.println("LoRa Transceiver Pronto (Modo Seguro e Discreto)");
}
 
void loop() {
  processUsb();
  processRadio();
  delay(1);
}
