#include <EEPROM.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <NECIRrcv.h>

NECIRrcv ir(7); // Patilla digital 7 para recibir codigo IR
unsigned long ircode; //Para almacenar el código recibido del mando a distancia
byte dir = 0x27; // Dirección Memoria Pantalla LCD
LiquidCrystal_I2C lcd(dir, 2, 1, 0, 4, 5, 6, 7); // Inicializar la pantalla

// CONFIGURACIÓN DEL CÓDIGO PARA ADAPTAR DISPOSITIVO. TAMBIÉN HABRÁ QUE MIRAR LOS CÓDIGOS DEL MANDO
const int errorHumedad = 900; // Si es mayor que este valor, la humedad se considera incorrecta
const int AlimLDR = 4; // +5v para LDR
const int ValorLDR = 2; // Lectura analógica LDR
const int AlimHum1 = 5; // +5v para Humedad 1
const int ValorHum1 = 1; // Lectura analógica Humedad 1
const int AlimHum2 = 6; // +5v para Humedad 2
const int ValorHum2 = 0; // Lectura analógica Humedad 2
const int PinValvula = 13; // +5v para activar relé 12v que alimenta ElectroBomba
const int umbralSolMax = 800;
const int umbralSolMin = 100;
const int umbralHumedadMax = 800;
const int umbralHumedadMin = 20;
const long TRiegoMax = 60; // segundos o minutos según multiplicador
const int TRiegoMin = 6; // segundos o minutos según multiplicador
const long TRiegoPaso = 2; // segundos o minutos según multiplicador, en los que se incrementa o decrementa TRiego en las opcions
const int tiempoMax = 60; // minutos
const int tiempoMin = 5; // minutos
const int tiempoPaso = 5; // minutos en los que se incrementa o decrementa tiempo en opciones
const bool EstadoReleApagado = true; //Algunos relés tendran que estar en true para no regar, y otros en false, dependiendo si el estado reposo del mismo es en HIGH o LOW.
const int MultiplicadorTRiego = 1; // El riego es en segundos. Si se quiere en minutos se pondrá 60 en el multiplicador.
const String UnidadTRiego = "segundos"; // Para los mensajes en pantalla, escribir esta constante como unidad de medida.
const int esperaLuz = 15; // segundos que permanece encendida la luz

int sol = 0; // Variable para sensor luz
int humedad = 0; // Variable para sensor humedad tierra
int contador = 0; // Para controlar el tiempo entre ciclos
int umbralSol = 500; // Cuando el sensor devuelve un valor menor que esta constante, se considera NOCHE
int umbralHumedad = 100; // Cuando el sensor devuelve un valor MAYOR que esta constante, se considera SECO
int menu = 0; // Para controlar menu, 0 es fuera de menu.
int tiempo = 5; // Tiempo que pasa entre ciclos de comprobación en minutos
int TRiego = 10; // Tiempo que se mantiene abierta la válvula automáticamente en segundos o minutos según multiplicador
bool LuzPantalla = true; // Variable para cambiar estado luz LCD Encendido/Apagado
bool EstadoRegando = false; // Variable para controlar estado riego Regando/Parado.
byte valor; // Variable para leer y escribir EEPROM
long contadorRiego = 0; // Para controlar el tiempo que se tiene que regar, en milisegundos.
int contadorLuz = esperaLuz * 1000; // Variable para controlar el autoapagado de la luz.

void setup() {
  // put your setup code here, to run once:
  cargarEEPROM();

  lcd.begin(16, 2);
  lcd.setBacklightPin(3, POSITIVE);
  mensajeBienvenida();
  delay(500);
  pinMode(AlimLDR, OUTPUT); // +5v para LDR
  pinMode(AlimHum1, OUTPUT); // +5v para Sensor Humedad
  pinMode(AlimHum2, OUTPUT); // +5v para Sensor Humedad
  pinMode(PinValvula, OUTPUT); // +5v para activar relé 12v que alimenta ElectroBomba
  digitalWrite(PinValvula, EstadoReleApagado); // Empieza apagado
  Serial.begin(9600);
  ir.begin();
  ir.blink13(0);//Desactiva el parpadeo del LED pin 13. Lo usamos para la electroválvula.

}

void loop() {
  // put your main code here, to run repeatedly:
  if (contador == 0 && menu == 0 && EstadoRegando == false)
  {
    tomarMedicion();
    imprimeLCD();

    if (sol < umbralSol) // Si es de noche
    {
      Serial.print(" -> Noche -> ");

      if (humedad > umbralHumedad) // Si está seco
      {
        if (humedad < errorHumedad)
        {
          Serial.println(" -> Seco -> Regar");
          regar(TRiego);
        }
        else
        {
          Serial.println(" -> Error -> No Regar");
        }
      }
      else
      {
        Serial.println(" -> Humedo -> No Regar");
      }
    }
    else
    {
      Serial.println(" -> Dia -> No Regar");
    }

    ir.flush();//Borra caché Infrarrojos
  }

  comprobarIR(); // BUCLE PARA CONTROLAR EL MANDO IR. ADAPTAR CÓDIGOS AL MANDO

  if (EstadoRegando == false)
  {
    contador++;
    if (contador >= tiempo * 60 * 10) // Este contador hace que se compruebe los valores de humedad y se riegue cada "tiempo" núm. de minutos.
    {
      contador = 0;
    }
  }
  else
  {
    contadorRiego -= 100;
    if (contadorRiego <= 0)
    {
      noRegar();
    }
  }
  if (contadorLuz > 0)
  {
    contadorLuz -= 100;
  }
  else
  {
    apagaLCD();
  }
  delay(100);
}

void comprobarIR() // AQUÍ HABRÁ QUE MODIFICAR LOS CÓDIGOS SEGÚN EL MANDO USADO.
{
  while (ir.available())
  {
    ircode = ir.read();
    Serial.print("Codigo recibido: ");
    Serial.println(ircode);
    switch (ircode) {
      case 4077715200 : //TECLA 1 <------------------------------------------------COD. IR
        regar(10); // Riega durante segundos seleccionados
        break;
      case 3877175040 : //TECLA 2 <------------------------------------------------COD. IR
        regar(20); // Riega durante segundos seleccionados
        break;
      case 2707357440 : //TECLA 3 <------------------------------------------------COD. IR
        regar(30); // Riega durante segundos seleccionados
        break;
      case 4144561920 : //TECLA 4 <------------------------------------------------COD. IR
        regar(40); // Riega durante segundos seleccionados
        break;
      case 3810328320 : //TECLA 5 <------------------------------------------------COD. IR
        regar(50); // Riega durante segundos seleccionados
        break;
      case 2774204160 : //TECLA 6 <------------------------------------------------COD. IR
        regar(60); // Riega durante segundos seleccionados
        break;
      case 3175284480 : //TECLA 7 <------------------------------------------------COD. IR
        regar(70); // Riega durante segundos seleccionados
        break;
      case 2907897600 : //TECLA 8 <------------------------------------------------COD. IR
        regar(80); // Riega durante segundos seleccionados
        break;
      case 3041591040 : //TECLA 9 <------------------------------------------------COD. IR
        regar(90); // Riega durante segundos seleccionados
        break;
      case 3910598400 : //TECLA 10 <-----------------------------------------------COD. IR
        regar(100); // Riega durante segundos seleccionados
        break;
      case 4061003520 : // Tecla Cancelar <----------------------------------------COD. IR
        noRegar(); // Para el riego cuando se pulsa la tecla cancelar
        break;
      case 4127850240 : // Tecla Display <-----------------------------------------COD. IR
        if (LuzPantalla == false)
          enciendeLCD();
        else
          apagaLCD();
        break;
      case 3158572800 : // Tecla Menu <--------------------------------------------COD. IR
        if (menu == 0)
          menu = 1;
        else
        {
          salvando();
          salvarEEPROM();
          menu = 0;
          tomarMedicion();
          imprimeLCD();
        }
        imprimeLCD();
        break;
      case 3141861120 : // Tecla Izquierda <---------------------------------------COD. IR
        menu--;
        if (menu == 0)
          menu = 4;
        imprimeLCD();
        break;
      case 3208707840 : // Tecla Derecha <-----------------------------------------COD. IR
        menu++;
        if (menu == 5)
          menu = 1;
        imprimeLCD();
        break;
      case 3927310080 : // Tecla Arriba <------------------------------------------COD. IR
        switch (menu) {
          case 1: // Sol
            if (umbralSol < umbralSolMax)
              umbralSol += 4;
            else
              umbralSol = umbralSolMax;
            imprimeLCD();
            break;
          case 2: // Humedad
            if (umbralHumedad < umbralHumedadMax)
              umbralHumedad += 4;
            else
              umbralHumedad = umbralHumedadMax;
            imprimeLCD();
            break;
          case 3: // Tiempo Riego
            if (TRiego < TRiegoMax)
              TRiego += TRiegoPaso;
            else
              TRiego = TRiegoMax;
            imprimeLCD();
            break;
          case 4: // Tiempo Medicion
            if (tiempo < tiempoMax)
              tiempo += tiempoPaso;
            else
              tiempo = tiempoMax;
            imprimeLCD();
            break;
        }
        break;
      case 4161273600 : // Tecla Abajo <-------------------------------------------COD. IR
        switch (menu) {
          case 1: // Sol
            if (umbralSol > umbralSolMin)
              umbralSol -= 4;
            else
              umbralSol = umbralSolMin;
            imprimeLCD();
            break;
          case 2: // Humedad
            if (umbralHumedad > umbralHumedadMin)
              umbralHumedad -= 4;
            else
              umbralHumedad = umbralHumedadMin;
            imprimeLCD();
            break;
          case 3: // Tiempo Riego
            if (TRiego > TRiegoMin)
              TRiego -= TRiegoPaso;
            else
              TRiego = TRiegoMin;
            imprimeLCD();
            break;
          case 4: // Tiempo Medicion
            if (tiempo > tiempoMin)
              tiempo -= tiempoPaso;
            else
              tiempo = tiempoMin;
            imprimeLCD();
            break;
        }
        break;
      default:
        Serial.println(" - Tecla no valida -");
    }
  }
}
void regar(long t) // t en segundos o minutos, activa riego
{
  regando(); // Pone mensaje regando en pantalla;
  EstadoRegando = true;
  digitalWrite(PinValvula, !EstadoReleApagado); // Activa Relé de la Electroválvula
  contadorRiego = t * 1000 * MultiplicadorTRiego; // Riega durante t segundos * MultiplicadorTRiego * 1000 (Al final, milisegundos)

}

void noRegar() // desactiva riego
{
  EstadoRegando = false;
  contadorRiego = 0;
  digitalWrite(PinValvula, EstadoReleApagado); // Corta Relé Riego
  tomarMedicion(); // Toma de nuevo valores Humedad y Luz
  imprimeLCD(); // Quita mensaje regando y actualiza el LCD con los nuevos valores
}

void mensajeBienvenida()
{
  enciendeLCD();
  lcd.home();
  lcd.print("Riego Javier");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Inicializando...");
}
void muestraError()
{
  enciendeLCD();
  lcd.clear();
  lcd.home();
  lcd.print("Error Medicion");
  lcd.setCursor(0, 1);
  lcd.print("Comprobar cables");
}

void salvando()
{
  enciendeLCD();
  lcd.clear();
  lcd.home();
  lcd.print("Guardando...");
  lcd.setCursor(0, 1);
  delay(500);
  lcd.print("...configuracion");
  delay(500);
}

void regando()
{
  enciendeLCD();
  lcd.clear();
  lcd.home();
  lcd.print("Riego Javier");
  lcd.setCursor(0, 1);
  lcd.print("Regando...");
}

void tomarMedicion()
{
  int h1 = 1000;
  int h2 = 1000;
  int cont = 0;
  humedad = 0;
  digitalWrite(AlimLDR, HIGH); // Enciende LDR
  delay(500);
  sol = analogRead(ValorLDR); // Lee el valor del Sol
  digitalWrite(AlimLDR, LOW); // Apaga LDR
  Serial.print("Luz: ");
  Serial.print(sol);
  Serial.print(" - ");

  digitalWrite(AlimHum1, HIGH); // Enciende sensor humedad
  delay(500);
  h1 = analogRead(ValorHum1); // Lee la humedad de la tierra
  digitalWrite(AlimHum1, LOW); //Apaga el sensor de humedad
  delay(500);
  digitalWrite(AlimHum2, HIGH); // Enciende sensor humedad
  delay(500);
  h2 = analogRead(ValorHum2); // Lee la humedad de la tierra
  digitalWrite(AlimHum2, LOW); //Apaga el sensor de humedad

  if (h1 <= 900)
  {
    cont ++;
    humedad = humedad + h1;
  }
  if (h2 <= 900)
  {
    cont++;
    humedad = humedad + h2;
  }
  if (cont > 0)
  {
    humedad = humedad / cont;
  }
  else
  {
    humedad = 1000;
  }
  Serial.print("Humedad 1: ");
  Serial.print(h1);
  Serial.print(" - Humedad 2: ");
  Serial.print(h2);
  Serial.print("- Humedad Total: ");
  Serial.print(humedad);
}

void salvarEEPROM()
{
  EEPROM.write(0, umbralSol / 4);
  EEPROM.write(1, umbralHumedad / 4);
  EEPROM.write(2, TRiego);
  EEPROM.write(3, tiempo);
}


void cargarEEPROM()
{
  valor = EEPROM.read(0); // Lineas para cargar valor umbralSol de la EPPROM
  if (valor * 4 >= umbralSolMin && valor * 4 <= umbralSolMax)
    umbralSol = valor * 4;
  valor = EEPROM.read(1); // Lineas para cargar valor umbralHumedad de la EPPROM
  if (valor * 4 >= umbralHumedadMin && valor * 4 <= umbralHumedadMax)
    umbralHumedad = valor * 4;
  valor = EEPROM.read(2); // Lineas para cargar valor TRiego de la EPPROM
  if (valor >= TRiegoMin && valor <= TRiegoMax)
    TRiego = valor;
  valor = EEPROM.read(3); // Lineas para cargar valor tiempo entre mediciones de la EPPROM
  if (valor >= tiempoMin && valor <= tiempoMax)
    tiempo = valor;
}
void enciendeLCD ()
{
  contadorLuz = esperaLuz * 1000;
  LuzPantalla = true;
  lcd.setBacklight(LuzPantalla);
}
void apagaLCD ()
{
  contadorLuz = 0;
  LuzPantalla = false;
  lcd.setBacklight(LuzPantalla);
}
void imprimeLCD ()
{
  enciendeLCD();
  switch (menu) {
    case 0:
      lcd.clear();
      lcd.home();
      lcd.print("Luz: "); lcd.print(sol); lcd.print(" (<"); lcd.print(umbralSol); lcd.print(")");
      lcd.setCursor(0, 1);
      if (humedad <= errorHumedad)
      {
        lcd.print("Hum: "); lcd.print(humedad); lcd.print(" (>"); lcd.print(umbralHumedad); lcd.print(")");
      }
      else
      {
        muestraError();
      }
      break;
    case 1:
      lcd.clear();
      lcd.home();
      lcd.print("Luz (max 800):");
      lcd.setCursor(0, 1);
      lcd.print("Ajustar +/-: "); lcd.print(umbralSol);
      break;
    case 2:
      lcd.clear();
      lcd.home();
      lcd.print("Hum (max 800):");
      lcd.setCursor(0, 1);
      lcd.print("Ajustar +/-: "); lcd.print(umbralHumedad);
      break;
    case 3:
      lcd.clear();
      lcd.home();
      lcd.print("Riego(");
      lcd.print(UnidadTRiego);
      lcd.print("):");
      lcd.setCursor(0, 1);
      lcd.print("Ajustar +/-: "); lcd.print(TRiego);
      break;
    case 4:
      lcd.clear();
      lcd.home();
      lcd.print("Medir (minutos):");
      lcd.setCursor(0, 1);
      lcd.print("Ajustar +/-: "); lcd.print(tiempo);
      break;
  }
}
