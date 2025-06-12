#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <PubSubClient.h>
#include <cstdlib>
#include <ArduinoJson.h>
#include <secret.h>

#define PIN_LED 12
const int TEMP_MIN = 20;
const int TEMP_MAX = 30;

bool estado_vent = false;

WiFiClient espClient;
PubSubClient client(espClient);

int status = WL_IDLE_STATUS;

// Datos del Broker 

const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_NAME = "ESP32_clua_borrajo";

unsigned long lastMeasurement = 0;
const unsigned long measurementInterval = 30000; // Tiempo entre mediciones = 30 segundos

int temp = 0;


bool abrir_ventanas() {
  if (estado_vent == false){
    Serial.println("Abriendo ventana...");
    digitalWrite(PIN_LED, HIGH);
    delay(10000);
    digitalWrite(PIN_LED, LOW);
    estado_vent = true;
    return true;
  }
  return false;
}

bool cerrar_ventanas() {
  if (estado_vent == true){
    Serial.println("Cerrando ventana...");
    digitalWrite(PIN_LED, HIGH);
    delay(10000);
    digitalWrite(PIN_LED, LOW);
    estado_vent = false;
    return true;
  }
  return false;
  
}


String estado_vent_to_str(){
  if (estado_vent)
    return "Abiertas";
  else
    return "Cerradas";
}

int get_temperatura(){
  return random(15,35);
}

void toggle_automatico(){
  automatico = !automatico;
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length){
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "iot2025_clua_borrajo/accion_ventanas") {
    Serial.print("Changing output to ");
    if(messageTemp == "abrir"){
      Serial.println("abrir");
      abrir_ventanas();
    }
    else if(messageTemp == "cerrar"){
      Serial.println("cerrar");
      cerrar_ventanas();
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(" MQTT connection...");
    
    if (client.connect(MQTT_CLIENT_NAME)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("iot2025_clua_borrajo/accion_ventanas");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  setup_wifi();
  client.setServer(MQTT_BROKER_ADRESS, MQTT_PORT);
  client.setCallback(callback);

  pinMode(PIN_LED, OUTPUT);
}


void loop(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  unsigned long now = millis();
  if ((now - lastMeasurement) > measurementInterval) {
    temp = get_temperatura();

    // Crear el JSON
    StaticJsonDocument<200> doc;
    doc["temperatura"] = temp;
    doc["estado"] = estado_vent;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    client.publish("iot2025_clua_borrajo/temperatura", jsonBuffer);
    lastMeasurement = millis();

  }
  delay(1000);
}