#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

const char* ssid = "XA-211";
const char* password = "AlondraInfiel";
const char* serverURL = "https://agua-api-6fiw.vercel.app/api/flujo";

volatile int pulsos1 = 0;
volatile int pulsos2 = 0;
volatile int pulsos3 = 0;

float caudal1_Lmin = 0;
float caudal2_Lmin = 0;
float caudal3_Lmin = 0;

void IRAM_ATTR contarPulsos1() { pulsos1++; }
void IRAM_ATTR contarPulsos2() { pulsos2++; }
void IRAM_ATTR contarPulsos3() { pulsos3++; }

void setup() {
  Serial.begin(115200);

  pinMode(12, INPUT_PULLUP); 
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(12), contarPulsos1, RISING);
  attachInterrupt(digitalPinToInterrupt(13), contarPulsos2, RISING);
  attachInterrupt(digitalPinToInterrupt(14), contarPulsos3, RISING);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Conectado");

  // Hora NTP
  configTime(-6 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Esperando hora NTP...");
  time_t now = time(nullptr);
  while (now < 100000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nHora sincronizada.");
}

void loop() {
  pulsos1 = 0;
  pulsos2 = 0;
  pulsos3 = 0;
  delay(1000);

  caudal1_Lmin = (pulsos1 / 450.0) * 60.0;
  caudal2_Lmin = (pulsos2 / 450.0) * 60.0;
  caudal3_Lmin = (pulsos3 / 450.0) * 60.0;

  Serial.print("Sensor 1: ");
  Serial.print(caudal1_Lmin);
  Serial.print(" | Sensor 2: ");
  Serial.print(caudal2_Lmin);
  Serial.print(" | Sensor 3: ");
  Serial.println(caudal3_Lmin);

  if (caudal1_Lmin < 1.0 || caudal1_Lmin > 30.0) Serial.println("Sensor 1 fuera de rango");
  if (caudal2_Lmin < 1.0 || caudal2_Lmin > 30.0) Serial.println("Sensor 2 fuera de rango");
  if (caudal3_Lmin < 1.0 || caudal3_Lmin > 30.0) Serial.println("Sensor 3 fuera de rango");

  if ((caudal1_Lmin > 0.1) || (caudal2_Lmin > 0.1) || (caudal3_Lmin > 0.1)) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL);
      http.addHeader("Content-Type", "application/json");

      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);

      char fecha[11];
      strftime(fecha, sizeof(fecha), "%Y-%m-%d", timeinfo);

      char hora[9];
      strftime(hora, sizeof(hora), "%H:%M:%S", timeinfo);

      String datosJSON = "{";
      datosJSON += "\"sensor1\": " + String(caudal1_Lmin, 2) + ",";
      datosJSON += "\"sensor2\": " + String(caudal2_Lmin, 2) + ",";
      datosJSON += "\"sensor3\": " + String(caudal3_Lmin, 2) + ",";
      datosJSON += "\"fecha\": \"" + String(fecha) + "\",";
      datosJSON += "\"hora\": \"" + String(hora) + "\"";
      datosJSON += "}";

      Serial.println("Enviando JSON:");
      Serial.println(datosJSON);

      int respuesta = http.POST(datosJSON);

      if (respuesta > 0) {
        String body = http.getString();
        Serial.println("Respuesta: " + body);
      } else {
        Serial.print("Error al enviar: ");
        Serial.println(http.errorToString(respuesta));
      }

      http.end();
    } else {
      Serial.println("WiFi desconectado");
    }
  } else {
    Serial.println("Sin flujo, no se envía");
  }

  delay(1000);
}
