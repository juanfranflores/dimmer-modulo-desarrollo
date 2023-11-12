// Librerías
#include <Arduino.h> // Necesaria para Platformio

// Definición de pines
#define zeroCrossingPin 12
#define pwmPin 14

// Constantes
const int minBrillo = 0;
const int maxBrillo = 1000;
const int minOffset = 100;
const int maxOffset = 600;

// Variables
int brillo = 0;
int valDimmer = 0;
String serialInput = "";
char receivedChar;
bool newChar;

void ICACHE_RAM_ATTR zeroCrossInt()
{ // Esta interrupción se ejecuta en el cruce por cero, cada 10mS para 50Hz
  // Cálculo para 50Hz: 1/50=20mS
  // Entonces medio ciclo= 10mS = 10000uS
  //(10000uS - 10uS) / 128 = 75 (aproximadamente), 10uS propagación

    delayMicroseconds(75 * valDimmer);
    digitalWrite(pwmPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(pwmPin, LOW);
  // }
}

void serialEvent()
{
  newChar = true;
  receivedChar = Serial.read();
}

void setValDimmer(int brillo)
{
  if (brillo <= minOffset)
  {
    brillo = minBrillo;
  }
  if (brillo >= maxOffset)
  {
    brillo = maxBrillo;
  } // haciendo esto tengo tiempos muertos para todos los valores en los rangos en los que la luz realmente no varía. Tengo que modificarlo.
  valDimmer = map(constrain(brillo, minBrillo, maxBrillo), minBrillo, maxBrillo, 128, 0);
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println("\nEncendido");
  pinMode(pwmPin, OUTPUT);
  attachInterrupt(zeroCrossingPin, zeroCrossInt, RISING);
  Serial.onReceive(serialEvent);
}
// brillo inicial, brillo final, tiempo en segundos
void fade(int inicial, int final, float fadeTime)
{
  int stepTime = 1000 * fadeTime / abs(final - inicial);
  if (inicial < final)
  {
    for (int i = inicial; i <= final; i += 1)
    {
      brillo = i;
      Serial.println(brillo);
      setValDimmer(brillo);
      delay(stepTime); // fade
    }
  }
  else if (inicial > final)
  {
    for (int i = inicial; i >= final; i -= 1)
    {
      brillo = i;
      Serial.println(brillo);
      setValDimmer(brillo);
      delay(stepTime); // fade
    }
  }
  else
  {
    setValDimmer(inicial); // No debería darse el caso, pero por las dudas :)
  }
}
void loop()
{
  fade(minOffset, maxOffset, 60);
  delay(500);
  fade(maxOffset, minOffset, 60);
  delay(500);
  // if (newChar)
  // {
  //   newChar = false;
  //   if (receivedChar == '\n') // Veo si es un enter
  //   {
  //     brillo = serialInput.toInt();
  //     Serial.print("Brillo: ");
  //     Serial.println(brillo);
  //     setValDimmer(brillo);
  //     Serial.print("valDimmer: ");
  //     Serial.println(valDimmer);
  //     serialInput = "";
  //   }
  //   else if (receivedChar == '\b' && serialInput.length() > 0) // Veo si es un backspace
  //   {
  //     serialInput.remove(serialInput.length() - 1);
  //     Serial.println(serialInput);
  //   }
  //   else if (isDigit(receivedChar)) // Veo si es un número
  //   {
  //     // Append the received character to the serialInput string
  //     serialInput += receivedChar;
  //     Serial.println(serialInput);
  //   }
  // }
  // delay(500);
}