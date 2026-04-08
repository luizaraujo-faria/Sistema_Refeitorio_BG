#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

// ===== WIFI =====
const char* ssid = "Paciente";
const char* password = "p@cient3";

// ===== API =====
const char* apiUrl = "http://172.31.36.97:5045/api/rm";

// LEDs
#define LED_AZUL 2
#define LED_AMARELO 15

// Buzzer
#define BUZZER 26

HardwareSerial rfidSerial(2);

String tag = "";
String ultimaTag = "";

unsigned long ultimoTempoLeitura = 0;
const int intervaloLeitura = 3000;

// ===== WIFI =====
void conectarWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi conectado");
  delay(1500);
}

// 🔊 Feedback sonoro
void beep(int freq, int tempo) {
  tone(BUZZER, freq);
  delay(tempo);
  noTone(BUZZER);
}

// 🌐 ENVIO API
bool enviarParaAPI(String uid) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(apiUrl);
  http.addHeader("Content-Type", "application/json");

  String json = "{\"uid\":\"" + uid + "\"}";

  int code = http.POST(json);

  Serial.print("HTTP Code: ");
  Serial.println(code);

  http.end();

  return (code > 0);
}

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_AZUL, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(BUZZER, LOW);
  noTone(BUZZER);

  rfidSerial.begin(9600, SERIAL_8N1, 16, 17);

  lcd.setCursor(0, 0);
  lcd.print("Sistema RFID");
  lcd.setCursor(0, 1);
  lcd.print("Inicializando...");

  Serial.println("=== SISTEMA RFID + API ===");

  conectarWiFi();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aproxime cartao");
}

void loop() {
  while (rfidSerial.available()) {
    char c = rfidSerial.read();

    if (c == 0x02) {
      tag = "";
    }
    else if (c == 0x03) {
      verificarLeitura(tag);
    }
    else {
      tag += c;
    }
  }
}

void verificarLeitura(String uid) {

  unsigned long agora = millis();

  if (uid == ultimaTag && (agora - ultimoTempoLeitura < intervaloLeitura)) {
    return;
  }

  ultimaTag = uid;
  ultimoTempoLeitura = agora;

  processarTag(uid);
}

void processarTag(String uid) {

  Serial.println("\nCartao detectado!");
  Serial.print("RAW: ");
  Serial.println(uid);

  // 🔥 LIMPEZA DO UID
  String uidLimpo = "";

  for (int i = 0; i < uid.length(); i++) {
    char c = uid[i];

    if (isDigit(c) || (c >= 'A' && c <= 'F')) {
      uidLimpo += c;
    }
  }

  // corta para 12
  if (uidLimpo.length() >= 12) {
    uidLimpo = uidLimpo.substring(0, 12);
  }

  Serial.print("UID LIMPO: ");
  Serial.println(uidLimpo);

  lcd.clear();

  // ❌ leitura inválida
  if (uidLimpo.length() != 12) {

    Serial.println("FALHA NA LEITURA");

    lcd.setCursor(0, 0);
    lcd.print("Falha leitura");

    digitalWrite(LED_AMARELO, HIGH);
    beep(500, 200);

    delay(1500);

    digitalWrite(LED_AMARELO, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aproxime cartao");

    return;
  }

  // ✅ MOSTRA UID
  lcd.setCursor(0, 0);
  lcd.print("UID:");
  lcd.setCursor(0, 1);
  lcd.print(uidLimpo);

  // 🌐 ENVIO API
  bool enviado = enviarParaAPI(uidLimpo);

  if (enviado) {
    Serial.println("ENVIADO PARA API");

    lcd.setCursor(0, 2);
    lcd.print("Enviado API");

    digitalWrite(LED_AZUL, HIGH);
    beep(2000, 150);
  } else {
    Serial.println("ERRO AO ENVIAR");

    lcd.setCursor(0, 2);
    lcd.print("Erro envio");

    digitalWrite(LED_AMARELO, HIGH);
    beep(500, 150);
  }

  delay(2000);

  digitalWrite(LED_AZUL, LOW);
  digitalWrite(LED_AMARELO, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aproxime cartao");

  Serial.println("Aguardando...");
}