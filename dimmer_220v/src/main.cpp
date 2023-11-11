#include <Arduino.h>
/*
   Programa de ejemplo para utilizar el módulo dimmer con cruce con cero, con Wemos y Fuente integradas.
 
   Más info:
   https://github.com/lu4ult/dimmer-modulo-desarrollo
 
   Video ejemplo:
   https://youtube.com/shorts/cVwYHIf1eo4
 */

#define zeroCrossingPin 12
#define pwmPin 14

int espera = 100;
int brilloDimmer = 0;

ICACHE_RAM_ATTR
void zeroCrossInt()
{ // Esta interrupción se ejecuta en el cruce por cero, cada 10mS para 50Hz
  // Cálculo para 50Hz: 1/50=20mS
  // Entonces medio ciclo= 10mS = 10000uS
  //(10000uS - 10uS) / 128 = 75 (aproximadamente), 10uS propagación

  if (brilloDimmer < 10 or brilloDimmer > 120)
  {
    if (brilloDimmer < 10)
      digitalWrite(pwmPin, HIGH);
    if (brilloDimmer > 120)
      digitalWrite(pwmPin, LOW);
  }
  else
  {
    delayMicroseconds(75 * brilloDimmer);
    digitalWrite(pwmPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(pwmPin, LOW);
  }
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println("Hola!");
  pinMode(pwmPin, OUTPUT);
  attachInterrupt(zeroCrossingPin, zeroCrossInt, RISING);
}

void loop()
{
  for (int i = 0; i < 128; i++)
  {
    brilloDimmer = i;
    delay(espera);
    // Serial.print("Brillo dimmer: ");
    // Serial.println(brilloDimmer);
  }
  delay(500);

  for(int i = 128; i > 0; i--)
  {
    brilloDimmer = i;
    delay(espera);
    // Serial.print("Brillo dimmer: ");
    // Serial.println(brilloDimmer);
  }
  delay(500);
}

