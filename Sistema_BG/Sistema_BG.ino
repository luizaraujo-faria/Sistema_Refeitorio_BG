#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// =====================================================
// LCD
// =====================================================

LiquidCrystal_I2C lcd(0x27, 20, 4);

// =====================================================
// WIFI
// =====================================================

const char* ssid = "Família_Rodrigues";
const char* password = "leaodejuda262408";
// const char* ssid = "Shaolin";
// const char* password = "luiz308he";

// =====================================================
// API
// =====================================================
const char* apiUrl = "http://192.168.0.104:5045/api/Registro";
// const char* apiUrl = "http://192.168.43.124:5045/api/Registro";

// =====================================================
// BUZZER
// =====================================================

#define BUZZER 26

// =====================================================
// SCANNER
// =====================================================

const int pinoRX = 32;
const int pinoTX = 33;

HardwareSerial leitor(1);

// =====================================================
// CONTROLE DE DUPLICIDADE
// =====================================================

String ultimoCodigo = "";

unsigned long ultimoTempoLeitura = 0;

const unsigned long intervaloLeitura = 3000;

// =====================================================
// BEEP SUCESSO
// =====================================================

void beepSucesso() {

  tone(BUZZER, 2000);
  delay(120);
  noTone(BUZZER);
}

// =====================================================
// BEEP ERRO
// =====================================================

void beepErro() {

  for (int i = 0; i < 3; i++) {

    tone(BUZZER, 180);

    delay(250);

    noTone(BUZZER);

    delay(120);
  }
}

// =====================================================
// CONECTAR WIFI
// =====================================================

void conectarWiFi() {

  Serial.print("Conectando WiFi");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print("Aguarde...");
  }

  Serial.println();
  Serial.println("WiFi conectado!");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi conectado");

  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  beepSucesso();

  delay(2000);
}

// =====================================================
// ENVIO PARA API
// =====================================================

bool enviarParaAPI(String codigo) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi desconectado");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi caiu");

    return false;
  }

  HTTPClient http;

  http.begin(apiUrl);

  http.addHeader("Content-Type", "application/json");

  String json = "{\"RM\":\"" + codigo + "\"}";

  Serial.println("Enviando:");
  Serial.println(json);

  int httpCode = http.POST(json);

  Serial.print("HTTP Code: ");
  Serial.println(httpCode);

  String resposta = http.getString();

  Serial.println("Resposta API:");
  Serial.println(resposta);

  // ============================
  // CAPTURA ERRO
  // ============================

  DynamicJsonDocument doc(512);

  DeserializationError erro = deserializeJson(doc, resposta);

  if (!erro) {

    String mensagem = doc["erro"] | "-";

    Serial.println();
    Serial.println("========== MENSAGEM ==========");
    Serial.println(mensagem);
    Serial.println("==============================");

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(mensagem);
  }
  else {

    Serial.println("Erro ao interpretar JSON:");
    Serial.println(erro.c_str());

    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print("ERRO AO IDENTIFICAR");
  }

  http.end();

  return (httpCode >= 200 && httpCode < 300);
}

// =====================================================
// PROCESSAR CARTAO
// =====================================================

void processarCodigo(String codigo) {

  codigo.trim();

  if (codigo.length() == 0)
    return;

  unsigned long agora = millis();

  if (codigo == ultimoCodigo &&
      (agora - ultimoTempoLeitura) < intervaloLeitura) {

    Serial.println("Leitura ignorada");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Leitura repetida");

    delay(1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aproxime o cartao");

    return;
  }

  ultimoCodigo = codigo;
  ultimoTempoLeitura = agora;

  Serial.println();
  Serial.println("================================");
  Serial.print("Cartao detectado: ");
  Serial.println(codigo);
  Serial.println("================================");

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Cartao:");

  lcd.setCursor(0, 1);
  lcd.print(codigo);

  bool sucesso = enviarParaAPI(codigo);

  if (sucesso) {

    Serial.println("ENVIADO COM SUCESSO");

    lcd.setCursor(0, 1);
    lcd.print("REGISTRO OK");

    beepSucesso();
  }
  else {

    Serial.println("ERRO AO ENVIAR");

    lcd.setCursor(0, 0);
    lcd.print("ERRO AO ENVIAR");

    beepErro();
  }

  delay(2500);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Aproxime cartao");
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Inicializando");

  Serial.println();
  Serial.println("=== SISTEMA INICIADO ===");

  conectarWiFi();

  leitor.begin(9600, SERIAL_8N1, pinoRX, pinoTX);

  Serial.println("Scanner pronto");

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Scanner pronto");

  delay(1500);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Aproxime cartao");
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  if (leitor.available()) {

    String codigoLido = leitor.readStringUntil('\n');

    codigoLido.trim();

    if (codigoLido.length() > 0) {

      Serial.print("RAW: [");
      Serial.print(codigoLido);
      Serial.println("]");

      processarCodigo(codigoLido);
    }
  }
}