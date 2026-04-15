#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

// ===== WIFI =====
const char* ssid = "Paciente";
const char* password = "p@cient3";

// ===== API =====
const char* apiUrl = "http://172.31.36.103:5045/api/Registro";

// LEDs
#define LED_AZUL 2
#define LED_AMARELO 15

// Buzzer
#define BUZZER 26

HardwareSerial rfidSerial(2);

String tag = "";
String ultimaTag = "";

// CONTROLE DE PRESENÇA
bool cartaoPresente = false;
unsigned long ultimoSinal = 0;
const int timeoutCartao = 1000;

unsigned long ultimoTempoLeitura = 0;
const int intervaloLeitura = 5000;

// ===== WIFI =====
void conectarWiFi() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi conectado");
    delay(1500);
  } else {
    Serial.println("\nFalha WiFi!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Sem conexao");
    delay(2000);
  }
}

// 🔊 Feedback sonoro
void beep(int freq, int tempo) {
  tone(BUZZER, freq);
  delay(tempo);
  noTone(BUZZER);
}

// 🌐 ENVIO API
int enviarParaAPI(String uid) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado");
    return -1;
  }

  HTTPClient http;
  http.begin(apiUrl);
  http.addHeader("Content-Type", "application/json");

  String json = "{\"uid\":\"" + uid + "\"}";

  int code = http.POST(json);

  Serial.print("HTTP Code: ");
  Serial.println(code);

  http.end();

  return code;
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

  // 🔄 Reconecta WiFi automaticamente
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // 🔥 DETECTA REMOÇÃO DO CARTÃO
  if (cartaoPresente && (millis() - ultimoSinal > timeoutCartao)) {
    Serial.println("Cartao removido");
    cartaoPresente = false;
    ultimaTag = "";
  }

  while (rfidSerial.available()) {

    char c = rfidSerial.read();

    // 🔥 atualiza sinal (cartão ainda presente)
    ultimoSinal = millis();

    if (c == 0x02) {
      tag = "";
    }
    else if (c == 0x03) {
      verificarLeitura(tag);
      tag = "";
    }
    else {
      tag += c;
    }
  }
}

void verificarLeitura(String uidBruto) {

  // 🔥 LIMPEZA DO UID
  String uidLimpo = "";

  for (int i = 0; i < uidBruto.length(); i++) {
    char c = uidBruto[i];

    if (isDigit(c) || (c >= 'A' && c <= 'F')) {
      uidLimpo += c;
    }
  }

  if (uidLimpo.length() >= 12) {
    uidLimpo = uidLimpo.substring(0, 12);
  }

  Serial.println("\nCartao detectado!");
  Serial.print("UID LIMPO: ");
  Serial.println(uidLimpo);

  // ❌ inválido
  if (uidLimpo.length() != 12) return;

  // 👻 UID fantasma
  if (uidLimpo == "000000000000") return;

  // 🔁 BLOQUEIO REAL (cartão ainda presente)
  if (cartaoPresente && uidLimpo == ultimaTag) {
    Serial.println("Cartao ainda presente - ignorado");
    return;
  }

  // ✅ NOVA LEITURA
  cartaoPresente = true;
  ultimaTag = uidLimpo;

  processarTag(uidLimpo);
}

void processarTag(String uid) {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("UID:");
  lcd.setCursor(0, 1);
  lcd.print(uid);

  int resposta = enviarParaAPI(uid);

  if (resposta > 0 && resposta < 300) {

    Serial.println("ENVIADO PARA API");

    lcd.setCursor(0, 2);
    lcd.print("Sucesso API");

    digitalWrite(LED_AZUL, HIGH);
    beep(2000, 350);

  } else {

    Serial.println("ERRO AO ENVIAR");

    lcd.setCursor(0, 2);
    lcd.print("Erro API");

    lcd.setCursor(0, 3);

    if (resposta == -1) {
      lcd.print("Sem WiFi");
    } else {
      lcd.print("Cod:");
      lcd.print(resposta);
    }

    digitalWrite(LED_AMARELO, HIGH);
    beep(500, 700);
    // delay(100);
    // beep(500, 300);
  }

  delay(2000);

  digitalWrite(LED_AZUL, LOW);
  digitalWrite(LED_AMARELO, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aproxime cartao");

  Serial.println("Aguardando...");
}