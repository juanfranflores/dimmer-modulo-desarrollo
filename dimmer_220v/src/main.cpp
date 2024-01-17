// ------------------------------LIBRERÍAS------------------------------
#include <Arduino.h> // Necesaria para Platformio

// Definición de pines
#define zeroCrossingPin 12
#define pwmPin 14

// ------------------------------CONSTANTES------------------------------
const int minBrillo = 0;
const int maxBrillo = 1000;
const int minDelayCross = 3500; // Para el brillo máximo: Disminuir hasta antes de que se vea una diferencia entre el brillo en 1000 y el valor anterior
const int maxDelayCross = 8200; // Para el brillo mínimo: Aumentar hasta antes de que comience a haber flicker

// ------------------------------VARIABLES------------------------------
int delaySinceCross = 0;
String serialInput = "";
char receivedChar;
bool newChar = false;

// ------------------------------DECLARACIÓN DE FUNCIONES------------------------------
void IRAM_ATTR zeroCrossInt();
// Serial
void serialInterrupt();    // Interrupción para recibir datos por serial
void processSerialInput(); // Procesa los datos recibidos por serial
// Brillo
void setBrillo(int val);                           // Setea el brillo
void fade(int inicial, int final, float fadeTime); // brillo inicial, brillo final, tiempo en segundos

// MQTT

void publicarBrillo(int val);

// ------------------------------SETUP------------------------------
void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.println("\nEncendido");
  pinMode(pwmPin, OUTPUT);
  attachInterrupt(zeroCrossingPin, zeroCrossInt, RISING);
  Serial.onReceive(serialInterrupt);
  setBrillo(0);
}
// ------------------------------LOOP------------------------------
void loop()
{
  processSerialInput();
}

// ------------------------------DEFINICION DE FUNCIONES------------------------------
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

void serialInterrupt()
{
  newChar = true;
  receivedChar = Serial.read();
}

void processSerialInput()
{
  if (newChar)
  {
    newChar = false;
    if (serialInput.length() > 0)
    {
      if (receivedChar == '\n') // Veo si es un enter
      {
        Serial.print("Brillo: ");
        Serial.println(serialInput);
        setBrillo(serialInput.toInt());
        serialInput = "";
      }
      else if (receivedChar == '\b') // Veo si es un backspace
      {
        serialInput.remove(serialInput.length() - 1);
        Serial.println(serialInput);
      }
    }
    if (isDigit(receivedChar)) // Veo si es un número
    {
      // Append the received character to the serialInput string
      serialInput += receivedChar;
      Serial.println(serialInput);
    }
  }
  delay(100);
}

void setBrillo(int val)
{
  delaySinceCross = map(val, minBrillo, maxBrillo, maxDelayCross, minDelayCross);
}

// brillo inicial, brillo final, tiempo en segundos
void fade(int inicial, int final, float fadeTime)
{
  if (inicial != final)
  {
    int stepTime = 1000 * fadeTime / abs(final - inicial);
    if (inicial < final)
    {
      for (int i = inicial; i <= final; i += 1)
      {
        Serial.print("fade brillo: ");
        Serial.println(i);
        setBrillo(i);
        delay(stepTime); // fade
      }
    }
    else
    {
      for (int i = inicial; i >= final; i -= 1)
      {
        Serial.print("fade brillo: ");
        Serial.println(i);
        setBrillo(i);
        delay(stepTime); // fade
      }
    }
  }
  else
  {
    setBrillo(final);
  }
}