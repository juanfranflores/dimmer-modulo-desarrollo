# Modificación para usar con ESP32 en Platformio
## Funcionamiento final:
La función de lu4ult para controlar la salida anda perfecto, así que en vez de modificarla, sólo adapté para que tome valores de brillo de 0 a 100. 
Lee el valor de brillo desde un feed de mqtt y lo transforma al valor correcto para que la función de zero crossing lo interprete.
También tiene un feed de tiempo en segundos desde el valor actual hasta el valor deseado. Con ese tiempo calcula cada cuánto debe variar el brillo  hasta llegar al valor deseado.
## Funcionamiento de debugging:
Lee el valor de brillo ingresado por serial y lo transforma a valor de cuenta una vez que se presiona enter. También se puede borrar caracteres con backspace.