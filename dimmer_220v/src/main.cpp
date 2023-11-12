// Librerías
#include <Arduino.h> // Necesaria para Platformio

// Definición de pines
#define zeroCrossingPin 12
#define pwmPin 14

// Constantes
const int minBrillo = 0;
const int maxBrillo = 1000;
const int minDelayCross = 4050; // Para el brillo máximo: Disminuir hasta que se vea una diferencia real entre el brillo en 900 y el brillo en 1000
const int maxDelayCross = 8100; // Para el brillo mínimo: Aumentar hasta antes del punto donde comienza a haber flicker

// Variables
int delaySinceCross = 0;
String serialInput = "";
char receivedChar;
bool newChar;

void ICACHE_RAM_ATTR zeroCrossInt()
{ // Esta interrupción se ejecuta en el cruce por cero, cada 10mS para 50Hz
  // Cálculo para 50Hz: 1/50=20mS
  // Entonces medio ciclo= 10mS = 10000uS
  //(10000uS - 10uS) / 128 = 75 (aproximadamente), 10uS propagación
  // (10000uS -10uS) / 1000
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

void serialEvent()
{
  newChar = true;
  receivedChar = Serial.read();
}

void setBrillo(int brillo)
{
  delaySinceCross = map(brillo, minBrillo, maxBrillo, maxDelayCross, minDelayCross);
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println("\nEncendido");
  pinMode(pwmPin, OUTPUT);
  attachInterrupt(zeroCrossingPin, zeroCrossInt, RISING);
  Serial.onReceive(serialEvent);
  setBrillo(0);
}
// brillo inicial, brillo final, tiempo en segundos
void fade(int inicial, int final, float fadeTime)
{
  int stepTime = 1000 * fadeTime / abs(final - inicial);
  if (inicial < final)
  {
    for (int i = inicial; i <= final; i += 1)
    {
      Serial.println(i);
      setBrillo(i);
      delay(stepTime); // fade
    }
  }
  else if (inicial > final)
  {
    for (int i = inicial; i >= final; i -= 1)
    {
      Serial.println(i);
      setBrillo(i);
      delay(stepTime); // fade
    }
  }
}
void loop()
{
  // fade(minBrillo, maxBrillo, 10);
  // delay(5000);
  // fade(maxBrillo, minBrillo, 10);
  // delay(5000);
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
    else if (isDigit(receivedChar)) // Veo si es un número
    {
      // Append the received character to the serialInput string
      serialInput += receivedChar;
      Serial.println(serialInput);
    }
  }
  delay(500);
}