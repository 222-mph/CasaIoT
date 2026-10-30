#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>

// ==================== CONFIGURACIÓN ====================
#define WIFI_SSID "Megacable_2.4G_1840"
#define WIFI_PASSWORD "gtG3YKu6"
#define FIREBASE_HOST "casaiot-b87fa-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyCAWWrdm9LyAYROa3Ll7dSCZrJBVGpo-gY"

FirebaseData fbdo;
FirebaseData firebaseData;

// ==================== PINES ====================
#define TRIG_PIN 4
#define ECHO_PIN 5

#define LED_SALA 13
#define LED_COMEDOR 14
#define LED_HAB1 16

#define LED_AMARILLO_13 27
#define LED_AMARILLO_2  32

#define BUZZER 15

// Relés (lógica Active-HIGH: HIGH = encender, LOW = apagar)
#define RELAY_PISCINA1 19
#define RELAY_PISCINA2 21
#define RELAY_VENTILADOR 22
#define RELAY_BOMBA_TINACO 18

#define SERVO_PRINCIPAL 23
#define SERVO_HAB1 25
#define SERVO_PATIO 26

#define RGB_ROJO 12
#define RGB_VERDE 17
#define RGB_AZUL 33

// ==================== OBJETOS ====================
Servo servoPrincipal;
Servo servoHab1;
Servo servoPatio;

// ==================== VARIABLES GLOBALES ====================
bool sistemaSeguridadActivo = true;
bool alarmaActiva = false;
unsigned long ultimoParpadeo = 0;
bool estadoParpadeo = false;
unsigned long ultimoBeep = 0;
bool estadoBeep = false;

String estadoPuertaPrincipal = "";
String estadoPuertaHab1 = "";
String estadoPuertaPatio = "";

#define PUERTA_CERRADA 0
#define PUERTA_ABIERTA 90

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);

  pinMode(LED_SALA, OUTPUT);
  pinMode(LED_COMEDOR, OUTPUT);
  pinMode(LED_HAB1, OUTPUT);
  pinMode(LED_AMARILLO_13, OUTPUT);
  pinMode(LED_AMARILLO_2, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY_PISCINA1, OUTPUT);
  pinMode(RELAY_PISCINA2, OUTPUT);
  pinMode(RELAY_VENTILADOR, OUTPUT);
  pinMode(RELAY_BOMBA_TINACO, OUTPUT);
  pinMode(RGB_ROJO, OUTPUT);
  pinMode(RGB_VERDE, OUTPUT);
  pinMode(RGB_AZUL, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(LED_SALA, LOW);
  digitalWrite(LED_COMEDOR, LOW);
  digitalWrite(LED_HAB1, LOW);
  digitalWrite(LED_AMARILLO_13, LOW);
  digitalWrite(LED_AMARILLO_2, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(RGB_ROJO, LOW);
  digitalWrite(RGB_VERDE, LOW);
  digitalWrite(RGB_AZUL, LOW);

  // Relés: arrancar apagados (LOW = 0V)
  digitalWrite(RELAY_PISCINA1, LOW);
  digitalWrite(RELAY_PISCINA2, LOW);
  digitalWrite(RELAY_VENTILADOR, LOW);
  digitalWrite(RELAY_BOMBA_TINACO, LOW);

  servoPrincipal.attach(SERVO_PRINCIPAL);
  servoHab1.attach(SERVO_HAB1);
  servoPatio.attach(SERVO_PATIO);
  servoPrincipal.write(PUERTA_CERRADA);
  servoHab1.write(PUERTA_CERRADA);
  servoPatio.write(PUERTA_CERRADA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConectado. IP: " + WiFi.localIP().toString());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}

// ==================== FUNCIONES AUXILIARES ====================
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);
  float distancia = duracion * 0.0343 / 2.0;
  return distancia;
}

void controlarAlarma(float distancia) {
  if (sistemaSeguridadActivo && distancia > 0 && distancia < 10) {
    alarmaActiva = true;
  } else {
    alarmaActiva = false;
  }
}

void actualizarAlarma() {
  unsigned long ahora = millis();

  if (alarmaActiva) {
    if (ahora - ultimoParpadeo >= 250) {
      ultimoParpadeo = ahora;
      estadoParpadeo = !estadoParpadeo;

      if (estadoParpadeo) {
        digitalWrite(LED_AMARILLO_13, HIGH);
        digitalWrite(LED_AMARILLO_2, LOW);
      } else {
        digitalWrite(LED_AMARILLO_13, LOW);
        digitalWrite(LED_AMARILLO_2, HIGH);
      }
    }

    if (ahora - ultimoBeep >= 250) {
      ultimoBeep = ahora;
      estadoBeep = !estadoBeep;
      digitalWrite(BUZZER, estadoBeep ? HIGH : LOW);
    }
  } else {
    digitalWrite(LED_AMARILLO_13, LOW);
    digitalWrite(LED_AMARILLO_2, LOW);
    digitalWrite(BUZZER, LOW);
  }
}

void controlarLedRGB(String modo) {
  digitalWrite(RGB_ROJO, LOW);
  digitalWrite(RGB_VERDE, LOW);
  digitalWrite(RGB_AZUL, LOW);

  if (modo == "Rojo") {
    digitalWrite(RGB_ROJO, HIGH);
  } else if (modo == "Verde") {
    digitalWrite(RGB_VERDE, HIGH);
  } else if (modo == "Azul") {
    digitalWrite(RGB_AZUL, HIGH);
  }
}

void modoFiesta() {
  static unsigned long ultimoCambio = 0;
  static int estado = 0;
  unsigned long ahora = millis();

  if (ahora - ultimoCambio >= 200) {
    ultimoCambio = ahora;
    digitalWrite(RGB_ROJO, LOW);
    digitalWrite(RGB_VERDE, LOW);
    digitalWrite(RGB_AZUL, LOW);

    switch (estado) {
      case 0: digitalWrite(RGB_ROJO, HIGH); break;
      case 1: digitalWrite(RGB_VERDE, HIGH); break;
      case 2: digitalWrite(RGB_AZUL, HIGH); break;
      case 3: digitalWrite(RGB_ROJO, HIGH); digitalWrite(RGB_VERDE, HIGH); break;
      case 4: digitalWrite(RGB_VERDE, HIGH); digitalWrite(RGB_AZUL, HIGH); break;
      case 5: digitalWrite(RGB_ROJO, HIGH); digitalWrite(RGB_AZUL, HIGH); break;
    }
    estado = (estado + 1) % 6;
  }
}

// ==================== LOOP ====================
void loop() {
  float distancia = medirDistancia();
  controlarAlarma(distancia);
  actualizarAlarma();

  Firebase.RTDB.setFloat(&fbdo, "/CasaIoT/Sensores/distancia", distancia);
  Firebase.RTDB.setBool(&fbdo, "/CasaIoT/Sensores/alarma_activa", alarmaActiva);

  // Luces
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Luces/sala")) {
    digitalWrite(LED_SALA, firebaseData.stringData() == "Encendido" ? HIGH : LOW);
  }
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Luces/comedor")) {
    digitalWrite(LED_COMEDOR, firebaseData.stringData() == "Encendido" ? HIGH : LOW);
  }
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Luces/habitacion1")) {
    digitalWrite(LED_HAB1, firebaseData.stringData() == "Encendido" ? HIGH : LOW);
  }

  // Ventilador (Active-HIGH): HIGH = 3.3V, LOW = 0V
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Abanico/habitacion1")) {
    digitalWrite(RELAY_VENTILADOR, firebaseData.stringData() == "Encendido" ? HIGH : LOW);
  }

  // Válvulas y bomba (Active-HIGH): HIGH = 3.3V, LOW = 0V
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Valvulas/piscina1")) {
    digitalWrite(RELAY_PISCINA1, firebaseData.stringData() == "Encendido" ? HIGH : LOW);
  }
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Valvulas/piscina2")) {
    digitalWrite(RELAY_PISCINA2, firebaseData.stringData() == "Encendido" ? HIGH : LOW);
  }
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Valvulas/tinaco")) {
    digitalWrite(RELAY_BOMBA_TINACO, firebaseData.stringData() == "Encendido" ? HIGH : LOW);
  }

  // Puertas
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Puertas/principal")) {
    String nuevo = firebaseData.stringData();
    if (nuevo != estadoPuertaPrincipal) {
      estadoPuertaPrincipal = nuevo;
      servoPrincipal.write(nuevo == "Abierto" ? PUERTA_ABIERTA : PUERTA_CERRADA);
      Serial.println("Puerta principal: " + nuevo);
    }
  }
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Puertas/habitacion1")) {
    String nuevo = firebaseData.stringData();
    if (nuevo != estadoPuertaHab1) {
      estadoPuertaHab1 = nuevo;
      servoHab1.write(nuevo == "Abierto" ? PUERTA_ABIERTA : PUERTA_CERRADA);
      Serial.println("Puerta habitación 1: " + nuevo);
    }
  }
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/Puertas/patio_trasero")) {
    String nuevo = firebaseData.stringData();
    if (nuevo != estadoPuertaPatio) {
      estadoPuertaPatio = nuevo;
      servoPatio.write(nuevo == "Abierto" ? PUERTA_ABIERTA : PUERTA_CERRADA);
      Serial.println("Puerta patio trasero: " + nuevo);
    }
  }

  // Seguridad
  if (Firebase.RTDB.getBool(&firebaseData, "/CasaIoT/Seguridad/sistema_activo")) {
    sistemaSeguridadActivo = firebaseData.boolData();
  }

  // LED RGB
  if (Firebase.RTDB.getString(&firebaseData, "/CasaIoT/LedRGB/modo")) {
    String modo = firebaseData.stringData();
    if (modo == "Fiesta") {
      modoFiesta();
    } else {
      controlarLedRGB(modo);
    }
  }

  delay(100);
}