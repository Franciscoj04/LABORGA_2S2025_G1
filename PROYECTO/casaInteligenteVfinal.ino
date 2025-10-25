#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

const int ledSala = 2;
const int ledComedor = 3;
const int ledCocina = 4;
const int ledBano = 5;
const int ledHabitacion = 6;
const int ledAzul = 13;
const int ledVerde = 7;
const int ledRojo = 14;//8
const int pinVentilador = 10;
const int pinServo = 11;
const int pinBotonPuerta = 12;
bool puertaAbierta = true;

Servo servoPuerta;
SoftwareSerial bluetooth(9, 8);
LiquidCrystal_I2C lcd(0x27, 16, 2);

struct Modo {
  bool leds[5];
  bool ventilador;
  const char* nombre;
};

const int eepromBase = 0;
const int modoCount = 5;
const int blockSize = 6;
Modo modos[modoCount] = {
  {{true, false,true,false,true}, true, "FIESTA"},
  {{true,true,true,true,true}, false, "RELAJADO"},
  {{false,false,false,false,false}, false, "NOCHE"},
  {{true,true,true,true,true}, true, "ENCENDERTODO"},
  {{false,false,false,false,false}, false, "APAGARTODO"}
};
// Pines asociados a cada ambiente
const int pinesAmbientes[5] = {ledSala, ledComedor, ledCocina, ledBano, ledHabitacion};
const char* nombresAmbientes[5] = {"sala", "comedor", "cocina", "bano", "habitacion"};
int modoActual = 4;

bool cargandoArchivo = false;
int modoCargando = -1;
int cini=0;
int cmodo=0;

void guardarModosEnEEPROM() {//se guarda cada modo en la memo EEPROM,guardando el array de modos como 1 para true y 0 para false
  for (int i=0; i<modoCount; i++) {
    int addr = eepromBase + i*blockSize;
    for (int j=0; j<5; j++) {
      EEPROM.write(addr+j, modos[i].leds[j] ? 1 : 0);
    }
    EEPROM.write(addr+5, modos[i].ventilador ? 1 : 0);
  }
  Serial.println("Guardando modos");
}

void cargarModosDesdeEEPROM() {//cargando los modos, es leer la eeprom
  for (int i=0; i<modoCount; i++) {
    int addr = eepromBase + i*blockSize;
    for (int j=0; j<5; j++) {//lena un array de modos, segun el array formado, por lo que se guardó a la memora
      modos[i].leds[j] = EEPROM.read(addr+j) == 1;//localiza cada led de cada modo, leyendo la dirección de memoria donde se ubica en la EEPROM
    }
    modos[i].ventilador = EEPROM.read(addr+5) == 1;//recorre cada modo y lee en la dirección de memoria de cada modo que configuración le da al ventilador si encendido o apagado
    
  }
  
}

void imprimirTablaEEPROM() {
  Serial.println("+--------------+-------------------+----------------------------+");
  Serial.println("| Modo         | Dirección EEPROM  | Estados (LEDs, Ventilador)  |");
  Serial.println("+--------------+-------------------+----------------------------+");
  
  for (int i = 0; i < modoCount; i++) {
    int addr = eepromBase + i * blockSize;
    Serial.print("| ");
    Serial.print(modos[i].nombre);
    Serial.print(" ");

    // Ajustar espacio para formato tabla
    for (int espacio = String(modos[i].nombre).length(); espacio < 12; espacio++) Serial.print(" ");//String(modos[i].nombre)

    Serial.print("| ");
    Serial.print(addr);
    Serial.print("                 | ");

    // Mostrar estado LEDs
    for (int j = 0; j < 5; j++) {
      Serial.print(modos[i].leds[j] ? "ON " : "OFF ");
    }
    Serial.print("| ");
    
    Serial.print(modos[i].ventilador ? "ON" : "OFF");

    Serial.println("                    |");
  }
  
  Serial.println("+--------------+-------------------+----------------------------+");
}


void aplicarModo(int modo) {//arreglar esto muy ciclico
  lcd.clear();
  lcd.print("Modo ");
  lcd.print(modos[modo].nombre);//muestra en la pantalla el nombre del modo actual 
  lcd.setCursor(0,1);
  lcd.print("Ventil: ");//indica el estado del ventilador 
  lcd.print(modos[modo].ventilador ? "ON" : "OFF");//la salida es si el ventilador debe estar encendido o apagado, según el true o false que está almacenado en el objeto, modo actual 
  if(modo != 0){
        digitalWrite(ledSala, modos[modo].leds[0]);
        digitalWrite(ledComedor, modos[modo].leds[1]);//la salida de cada led va a ser lo que hay almacenado en el array de led del modo actual, el indice de led corresponde al numero de habitación
        digitalWrite(ledCocina, modos[modo].leds[2]);
        digitalWrite(ledBano, modos[modo].leds[3]);//Escritura digital, output para cada LED según el modo escogido
        digitalWrite(ledHabitacion, modos[modo].leds[4]);
        digitalWrite(pinVentilador, modos[modo].ventilador ? HIGH : LOW);//puede venir high o low, se recorre el array de modos y se muestra la salida del ventilador en el modo actual
  }else{
      while(bluetooth.readStringUntil('\n') == 0){
        // Detectar botón para abrir/cerrar puerta
      static unsigned long lastDebounceTime2 = 0;
        int botonEstado2 = digitalRead(pinBotonPuerta);
        if (botonEstado2 == LOW && (millis() - lastDebounceTime2) > 1000) {// comprueba si el boton está presionado LOW y si han pasado mas de 50 ms antes de la última detección de si está abierta o cerrada la puerta
          
          cerrarPuerta();
          lastDebounceTime2 = millis();
        }else if (botonEstado2 == HIGH && (millis() - lastDebounceTime2) > 200) {// comprueba si el boton está presionado LOW y si han pasado mas de 50 ms antes de la última detección de si está abierta o cerrada la puerta
          
          togglePuerta();//cambia el estado actual de la puerta
          lastDebounceTime2 = millis();//listo
        }

        digitalWrite(ledSala, true);
        digitalWrite(ledComedor, true);
        digitalWrite(ledCocina, true);
        digitalWrite(ledBano, true);
        digitalWrite(ledHabitacion, true);
        
        delay(500);
        digitalWrite(ledSala, false);
        digitalWrite(ledComedor, false);
        digitalWrite(ledCocina, false);
        digitalWrite(ledBano, false);
        digitalWrite(ledHabitacion, false);
        digitalWrite(pinVentilador, modos[modo].ventilador ? HIGH : LOW);
        delay(500); 
      }  
  }
  
  
  Serial.print("Modo" );
  Serial.print(modos[modo].nombre);
  Serial.print("Ventil ");
  Serial.println(modos[modo].ventilador);
  modoActual = modo;//se actualiza la variable global y el programa ya sabe que el modo actual es el que se le envió el  numero desde setup
}

void setup() {
  pinMode(ledSala, OUTPUT);
  pinMode(ledComedor, OUTPUT);
  pinMode(ledCocina, OUTPUT);
  pinMode(ledBano, OUTPUT);
  pinMode(ledHabitacion, OUTPUT);
  pinMode(ledAzul, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledRojo, OUTPUT);
  pinMode(pinVentilador, OUTPUT);
  pinMode(pinBotonPuerta, INPUT_PULLUP);
  servoPuerta.attach(pinServo);
  Serial.begin(9600);
  bluetooth.begin(9600);
  lcd.init();
  lcd.backlight();
  cargarModosDesdeEEPROM();
  aplicarModo(4);//encender todo 
  servoPuerta.write(0);
  cargandoArchivo = false;//desde setup, que solo se ejecuta una vez , aquí no se ha cargado el archivo .org, se carga hasta que se comience a ejecutar el void loop()
}

void loop() {
  // Detectar botón para abrir/cerrar puerta
  static unsigned long lastDebounceTime = 0;
  int botonEstado = digitalRead(pinBotonPuerta);
  if (botonEstado == LOW && (millis() - lastDebounceTime) > 3000) {// comprueba si el boton está presionado LOW y si han pasado mas de 50 ms antes de la última detección de si está abierta o cerrada la puerta
    cerrarPuerta();
    lastDebounceTime = millis();
  }else if (botonEstado == HIGH && (millis() - lastDebounceTime) > 500) {// comprueba si el boton está presionado LOW y si han pasado mas de 50 ms antes de la última detección de si está abierta o cerrada la puerta
    
    togglePuerta();//cambia el estado actual de la puerta
    lastDebounceTime = millis();
  }

  // Procesar comandos Bluetooth o Serial
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');//se va a leer cada linea del arch. de config .org, separando lineas por \n
    line.trim();//Elimina espacios o caracteres invisibles al inicio y final para limpiar la línea.
    if (line == "INICIAR CARGA") {
      digitalWrite(ledAzul, LOW);
      cargandoArchivo = true;//se actualiza la variable que indica que se está leyendo un archivo ahora
      lcd.clear();
      lcd.print("Carga iniciado");
    } else if (cargandoArchivo) {//Si ya estamos en modo de carga, envía cada línea leída a la función que procesa el contenido del archivo .org.
      procesarArchivoOrg(line);
    }
  }
  if (bluetooth.available()) {
    String cmd = bluetooth.readStringUntil('\n');
    cmd.trim();
    if (!cargandoArchivo) {
      digitalWrite(ledAzul, HIGH);
      procesarComandoBluetooth(cmd);
      
    }
  }
}



void procesarArchivoOrg(String linea) {//procesar una linea del archivo org
  
  if (linea.startsWith("modo") && cini>cmodo) {//si la linea actual empieza con la palabra Modo , quité un espacio
    
    if (linea.indexOf("fiesta") >= 0) 
    {modoCargando = 0;
    cini++;
    cmodo++;}
    //la linea actual que se está procesando lleva la palabra FIESTA despues de Modo, se actualiza la var modoCargando ya no sería -1 sino que 0
    else if (linea.indexOf("relajado") >= 0) {
      modoCargando = 1;
      cini++;
    cmodo++;}
    else if (linea.indexOf("noche") >= 0){
       modoCargando = 2;
       cini++;
    cmodo++;}
    else if (linea.indexOf("encender_todo") >= 0) {
      modoCargando = 3;
      cini++;
    cmodo++;}
    else if (linea.indexOf("apagar_todo") >= 0){
       modoCargando = 4;
       cini++;
    cmodo++;}
    else {
      modoCargando = -1;//caso en que vino otra palabra no valida despues de MODO, modoCargado seguirá siendo -1
      digitalWrite(ledRojo, HIGH);
    }
  } else if (modoCargando != -1) {//si el modo que se está cargando es válido, ejecutar las instrucciones que indica el .ORG para las LED y para el ventilador
    digitalWrite(ledRojo, LOW);
    if (linea.startsWith("LED'S: ")) {
  //    Serial.print("Leyendo LEDS");
      String val = linea.substring(7);//5
      val.toUpperCase();
      Serial.print("-----------------------leds----->");
      Serial.println(val);
      if(val != "ALTERNANDOSE"){//String(modos[i].nombre)
        bool encendido = (val == "ON");//hacer config. aparte para modo fiesta 
        for (int i=0; i<5; i++) modos[modoCargando].leds[i] = encendido;
      }else {
        for (int i=0; i<5; i++) modos[modoCargando].leds[i] = true;//todos los LEDS empiezan encendidos
      }

    } else if (linea.startsWith("Ventil")) {
      String val = linea.substring(11);//10
      
      val.trim();
      modos[modoCargando].ventilador = (val == "ON");

    } else if (linea.length() == 0) {//detecta que ya se terminó de ingresar un modo definido en el .org
      Serial.print("linea vacía");
      guardarModosEnEEPROM();
      lcd.clear();
      lcd.print("Config guardada");
      delay(1000);
      modoCargando = -1;
      cargarModosDesdeEEPROM();//carga los modos desde la EEPROM en la memoria RAM del arduino
      aplicarModo(modoActual);//borrar esto
      delay(2000);//MDL
      lcd.clear();
      lcd.print("modo ingresado");//
      delay(2000);
    }
  }//agregue esto yo
  else if(linea == "conf_ini" ){
     cini = cini+1;
    
  }else if(cini<=cmodo){
cargandoArchivo = false;//se anula la carga del archivo .org por no seguir instrucciones
      lcd.clear();
      lcd.println("Error invalid org");//el archivo no se carga y se resetean los contadores
      delay(3000);
      lcd.clear();
      cini=0;
      cmodo=0;
      digitalWrite(ledRojo, HIGH);
      delay(5000);
      digitalWrite(ledRojo, LOW);
      cini =0;
    cmodo=0;
  }
  else if(linea == "conf:fin"){
    lcd.clear();
    lcd.print("Fin Archivo Org");
    cargandoArchivo = false;
    parpadeoVerde();

    imprimirTablaEEPROM();
    cini =0;
    cmodo=0;
    
  }
}//termine de modificar

void procesarComandoBluetooth(String cmd) {
  cmd.toUpperCase(); // Convertir todo a mayúsculas para comparación simple
  if (cmd.startsWith("MODO")) {
    ComandoModo(cmd);
  }else{
    onOffAmbiente(cmd);
  }
  
}

void togglePuerta() {
  if (puertaAbierta) {
    cerrarPuerta();
  } else {
    abrirPuerta();
  }
}

void abrirPuerta() {
  servoPuerta.write(90);
  lcd.clear();
  lcd.print("puerta abierta");
  puertaAbierta = true;
}

void cerrarPuerta() {
  if(puertaAbierta){//puertaAbierta== true
    servoPuerta.write(0);//le indica al servomotor cerrar la puerta 
    lcd.clear();
    lcd.print("puerta cerrada");
    puertaAbierta = false;
  }
  
}

void ComandoModo(String cmd) {
  
  if (cmd == "MODO_FIESTA") {
    aplicarModo(0);
    bluetooth.println("Modo FIESTA activado");
  
  } else if (cmd == "MODO_RELAJADO") {
    aplicarModo(1);
    bluetooth.println("Modo RELAJADO activado");
    
  } else if (cmd == "MODO_NOCHE") {
    aplicarModo(2);
    bluetooth.println("Modo NOCHE activado");
    
  } else if (cmd == "MODO_ENCENDER_TODO") {
    aplicarModo(3);
    bluetooth.println("Modo ENCENDERTODO activado");
    
  } else if (cmd == "MODO_APAGAR_TODO") {
    aplicarModo(4);
    bluetooth.println("Modo APAGARTODO activado");
    
  } else {
    bluetooth.println("Comando desconocido");//tendria que brillar led rojo
    parpadeoRojo();
  }
}



void onOffAmbiente(String comando) {
  comando.trim();
  comando.toLowerCase(); // Para evitar problemas con mayúsculas
  
  int sep = comando.indexOf(':');
  if (sep == -1) { 
    parpadeoRojo();
    return;} // Comando mal formado
  
  String ambiente = comando.substring(0, sep);
  ambiente.trim();
  String estado = comando.substring(sep + 1);
  estado.trim();
  bool v = false;

  
  // Reemplazar letras con tildes o ñ si necesario (opcional)
  ambiente.replace("ó", "o");
  ambiente.replace("ñ", "n");
  
  // Buscar el índice del ambiente
  int idx = -1;
  if(ambiente == "ventilador"){
    v=true;
    
  }else{
    for (int i = 0; i < 5; i++) {
        if (ambiente == nombresAmbientes[i]) {
          idx = i;
          break;
        }
      }
  }
  
  
  if (idx == -1 && v == false) {
    // Ambiente no encontrado
    Serial.println("Ambiente no reconocido: " + ambiente);
    parpadeoRojo();
    return;
  }
  
  // Determinar estado ON/OFF
  bool encender = false;
  if (estado == "on") encender = true;
  else if (estado == "off") encender = false;
  else {
    Serial.println("Estado no reconocido: " + estado);
    parpadeoRojo();
    return;
  }
  
  

  if(!v){
    // Controlar el LED
  
  digitalWrite(pinesAmbientes[idx], encender ? HIGH : LOW);
  Serial.print("LED ");
  Serial.print(nombresAmbientes[idx]);
  Serial.print(" ");
  Serial.println(encender ? "encendido" : "apagado");
  lcd.clear();
  lcd.println("LED ENCENDIDO");
  delay(1000);
  }else{
    digitalWrite(pinVentilador, encender ? HIGH : LOW);
  }
}

void parpadeoRojo() {//cuando hay error
    digitalWrite(ledRojo, HIGH);
    delay(500);
    digitalWrite(ledRojo, LOW);
    delay(500);
    digitalWrite(ledRojo, HIGH);
    delay(500);
    digitalWrite(ledRojo, LOW);
    delay(500);
    digitalWrite(ledRojo, HIGH);
    delay(500);
    digitalWrite(ledRojo, LOW);
    delay(500);
}

void parpadeoVerde() {//cuando hay error
    delay(2000);
    digitalWrite(ledVerde, HIGH);
    delay(500);
    digitalWrite(ledVerde, LOW);
    delay(500);
    digitalWrite(ledVerde, HIGH);
    delay(500);
    digitalWrite(ledVerde, LOW);
    delay(500);
    digitalWrite(ledVerde, HIGH);
    delay(500);
    digitalWrite(ledVerde, LOW);
}

