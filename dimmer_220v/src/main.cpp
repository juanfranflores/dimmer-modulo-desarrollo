// ------------------------------LIBRERÍAS------------------------------
#include <Arduino.h> // Necesaria para Platformio
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>

// ------------------------Definición de pines---------------------------
#define zeroCrossingPin 12
#define pwmPin 14

// ------------------------------CONSTANTES------------------------------
const char *ssid = "Flores 2.4GHz";
const char *password = "Lilas549";
const char *mqtt_server = "192.168.1.11";
const char *clientId = "ceiling_light";
String clientIp = "";
// const char *ceilingLightConfigTopic = "homeassistant/light/ceiling_light/config"; // TODO: agregar el config
const char *ceilingLightAvailabilityTopic = "homeassistant/light/ceiling_light/availability"; // TODO: agregar el availability
const char *ceilingLightSetTopic = "homeassistant/light/ceiling_light/set";
const char *ceilingLightStateTopic = "homeassistant/light/ceiling_light/state";
const char *ceilingLightIpTopic = "homeassistant/text/ceiling_light/ip";
const int minBrillo = 0;
const int maxBrillo = 255;
const float defaultFadeTime = 0.5;
const float maxFadeTime = 300;  // 5 minutos
const int minDelayCross = 3500; // Para el brillo máximo: Disminuir hasta antes de que se vea una diferencia entre el brillo en 1000 y el valor anterior
const int maxDelayCross = 8000; // Para el brillo mínimo: Aumentar hasta antes de que comience a haber flicker

// ------------------------------VARIABLES------------------------------

int delaySinceCross = 0;
int lastBrillo = 0;

// ----------------------Inicialización de librerías----------------------
WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

// ------------------------------DECLARACIÓN DE FUNCIONES------------------------------
void setup_wifi();
void setup_mqtt();
void reconnect();
void callback(char *topic, byte *message, unsigned int length);
void processJson(String json);
void IRAM_ATTR zeroCrossInt();
void setDelaySinceCross(int val);           // Setea el brillo
void setBrillo(int brillo, float fadeTime); // brillo final, tiempo en segundos

// ------------------------------SETUP------------------------------
void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("\nEncendido");
  pinMode(pwmPin, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();
  attachInterrupt(digitalPinToInterrupt(zeroCrossingPin), zeroCrossInt, RISING);
  setBrillo(0, 0); // cambiar por newBrillo=0 y newBrilloFlag=true
}
// ------------------------------LOOP------------------------------
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  server.handleClient();
  ElegantOTA.loop();
  client.loop();
}

// ------------------------------DEFINICION DE FUNCIONES------------------------------
void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  clientIp = WiFi.localIP().toString() + "/update";
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(clientIp);

  server.on("/",
            []()
            {
              server.send(200, "text/html", (String("Este es el webserver para subir firmware en <b>") + clientId + String("</b>. <a href=\"http://") + clientIp + String("/update\">Subir firmware</a>")).c_str());
            });

  ElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId, ceilingLightAvailabilityTopic, 0, true, "offline"))
    {
      Serial.println("connected");
      // Clear retained messages
      // TODO: Agregar el config
      client.publish(ceilingLightAvailabilityTopic, "online", true);
      client.publish(ceilingLightStateTopic, "", true);
      client.publish(ceilingLightIpTopic, clientIp.c_str(), true);
      // Subscribe
      client.subscribe(ceilingLightSetTopic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char *topic, byte *message, unsigned int length)
{
  // Llegó un nuevo mensaje
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++)
  {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  if ((String(topic) == ceilingLightSetTopic))
  {
    processJson(messageTemp);
  }
}

void processJson(String json)
{
  const size_t capacity = JSON_OBJECT_SIZE(3) + 60;
  DynamicJsonDocument doc(capacity);

  auto error = deserializeJson(doc, json);
  if (error)
  {
    Serial.println(F("Failed to read JSON"));
    setDelaySinceCross(0);
    return;
  }
  String state = doc["state"];        // Extracts value of "state"
  int transition = doc["transition"]; // Extracts value of "transition"
  int brightness = doc["brightness"]; // Extracts value of "brightness"
  /*
  casos posibles:
  1. state = ON y brightness == 0 -> brightness = maxBrillo
  2. state = ON y brightness != 0 -> brightness = brightness
  2. state = OFF                  -> brightness =0
  */
  if (state == "ON" && brightness == 0)
  {
    brightness = maxBrillo;
  }
  else if (state == "OFF")
  {
    brightness = 0;
  }
  setBrillo(brightness, transition);
}

void setBrillo(int brillo, float fadeTime) // brillo final, tiempo en segundos
{
  // brillo = (lastBrillo == 0) ? maxBrillo : constrain(brillo, minBrillo, maxBrillo);
  brillo = constrain(brillo, minBrillo, maxBrillo);
  String json = (brillo == 0) ? "{\"state\":\"OFF\"}" : "{\"state\":\"ON\",\"brightness\":" + String(brillo) + "}";
  client.publish(ceilingLightStateTopic, json.c_str(), true);
  Serial.printf("Brillo: %d.\nDelay since cross: %d.\n", brillo, delaySinceCross);

  fadeTime = (fadeTime == 0 || fadeTime == 5) ? defaultFadeTime : constrain(fadeTime, 0, maxFadeTime);
  int stepTime = (brillo == lastBrillo) ? 0 : 1000 * fadeTime / abs(brillo - lastBrillo);
  int increment = (lastBrillo <= brillo) ? 1 : -1;
  for (int i = lastBrillo; (increment > 0 ? i <= brillo : i >= brillo); i += increment)
  {
    setDelaySinceCross(i);
    delay(stepTime); // fade
  }
  lastBrillo = brillo;
}

void setDelaySinceCross(int val)
{
  val = constrain(val, minBrillo, maxBrillo);
  delaySinceCross = map(val, minBrillo, maxBrillo, maxDelayCross, minDelayCross);
}

void IRAM_ATTR zeroCrossInt()
{
  if (delaySinceCross <= minDelayCross)
  {
    digitalWrite(pwmPin, HIGH); // Totalmente prendido
  }
  else if (delaySinceCross >= maxDelayCross)
  {
    digitalWrite(pwmPin, LOW); // Totalmente apagado
  }
  else
  {
    delayMicroseconds(delaySinceCross);
    digitalWrite(pwmPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(pwmPin, LOW);
  }
}