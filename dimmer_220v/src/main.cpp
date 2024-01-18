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
const char *clientId = "room_light";
String clientIp = "";
// const char *roomLightConfigFeed = "homeassistant/light/room_light/config"; // TODO: agregar el config
// const char *roomLightAvailabilityFeed = "homeassistant/light/room_light/availability"; // TODO: agregar el availability
const char *roomLightSetFeed = "homeassistant/light/room_light/set";
const char *roomLightStateFeed = "homeassistant/light/room_light/state";
const char *roomLightIpFeed = "homeassistant/text/room_light/ip";
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
  setBrillo(0, 0);
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
    if (client.connect(clientId))
    {
      Serial.println("connected");
      // Clear retained messages
      // TODO: Agregar el config y el availability
      client.publish(roomLightStateFeed, "", true);
      client.publish(roomLightIpFeed, clientIp.c_str(), true);
      // Subscribe
      client.subscribe(roomLightSetFeed);
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
  if ((String(topic) == roomLightSetFeed))
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
  setBrillo(brightness, transition);
}

// brillo final, tiempo en segundos
void setBrillo(int brillo, float fadeTime)
{
  String json = "";
  Serial.printf("Brillo: %d.\nDelay since cross: %d.\n", brillo, delaySinceCross);
  if (brillo != 0)
  {
    if (lastBrillo == 0)
    { // Hardcodeado para evitar que home assistant lo haga prender con el brillo mínimo
      brillo = maxBrillo;
    }
    json = "{\"state\":\"ON\",\"brightness\":" + String(brillo) + "}";
  }
  else
  {
    json = "{\"state\":\"OFF\"}";
  }
  client.publish(roomLightStateFeed, json.c_str(), true);
  fadeTime = constrain(fadeTime, 0, maxFadeTime);
  if (fadeTime == 0 || fadeTime == 5) // 5 es el valor por defecto de home assistant
  {
    fadeTime = defaultFadeTime;
  }
  if (brillo != lastBrillo)
  {
    int stepTime = 1000 * fadeTime / abs(brillo - lastBrillo);
    if (lastBrillo < brillo)
    {
      for (int i = lastBrillo; i <= brillo; i += 1)
      {
        setDelaySinceCross(i);
        delay(stepTime); // fade
      }
    }
    else
    {
      for (int i = lastBrillo; i >= brillo; i -= 1)
      {
        setDelaySinceCross(i);
        delay(stepTime); // fade
      }
    }
  }
  else
  {
    setDelaySinceCross(brillo);
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