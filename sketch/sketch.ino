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
#define BOT_TOKEN "7618978665:AAHuhrlPt6Uz2XXOc9dn2Ba3SmibbUfoy5w"
#define CHAT_ID "-4924904468"
const int TEMP_MIN = 20;
const int TEMP_MAX = 30;


int temp_ini = 15;
bool estado_vent = false;

WiFiClient espClient;
PubSubClient client(espClient);

int status = WL_IDLE_STATUS;

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;


// Datos del Broker 

const char* MQTT_BROKER_ADRESS = "192.168.0.34";//ip local aca, misma que el pc
const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_NAME = "iot2025_clua_borrajo";


unsigned long lastNotificationTime = 0; // ultima notificacion
const unsigned long notificationInterval = 300000; // Tiempo de intervalo entre notificaciones = 5 minutos

unsigned long lastMeasurement = 0;
const unsigned long measurementInterval = 30000; // Tiempo entre mediciones = 30 segundos

int temp = 0;
bool automatico = true;


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

void actuar_cambio_temp(){
  if (temp > TEMP_MAX)
    abrir_ventanas();
  else
    if (temp < TEMP_MIN)
      cerrar_ventanas();
    else
      Serial.println("No accion ante cambio temp");
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
    messageTemp = (char)message[i];
  
    Serial.println();

    // Feel free to add more if statements to control more GPIOs with MQTT

    // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
    // Changes the output state according to the message
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
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(" MQTT connection...");
    
    if (client.connect("ESP32Client")) { //OJO credenciales MOSQUITTO MQTT_USER, MQTT_PASS
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

//Iniciamos los pines del ESP32
void setup() {

  Serial.begin(115200);

  setup_wifi();
  client.setServer(MQTT_BROKER_ADRESS, MQTT_PORT);
  client.setCallback(callback);

  configTime(-3 * 3600, 0, "pool.ntp.org");  // UTC-3 para Argentina, por ejemplo
  // Esperar a que se sincronice
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(1000);
    Serial.println("Esperando hora...");
  }

  pinMode(PIN_LED, OUTPUT);
}


//Iniciamos la funcion bucle que se repetira indefinidamente
void loop(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  unsigned long now = millis();
  if ((now - lastMeasurement) > measurementInterval) {
    temp = get_temperatura();

    // Convert the value to a char array
    char tempString[8];
    dtostrf(temp, 1, 2, tempString);
    Serial.print("Temperatura: ");
    Serial.println(tempString);
    client.publish("iot2025_clua_borrajo/temperatura", tempString);
    lastNotificationTime = millis();

  }
  delay(1000);
}