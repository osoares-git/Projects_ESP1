void (*resetFunc)(void) = 0;  // Reset digital
#include "WiFiEsp.h"          //INCLUSÃO DA BIBLIOTECA
#include "SoftwareSerial.h"   //INCLUSÃO DA BIBLIOTECA
#include <ArduinoJson.h>

SoftwareSerial Serial1(6, 7);  //PINOS QUE EMULAM A SERIAL, ONDE O PINO 6 É O RX E O PINO 7 É O TX

char ssid[] = "WIFI_SSID";  //VARIÁVEL QUE ARMAZENA O NOME DA REDE SEM FIO
char pass[] = "WIFI_password";    //VARIÁVEL QUE ARMAZENA A SENHA DA REDE SEM FIO

int status = WL_IDLE_STATUS;  //STATUS TEMPORÁRIO ATRIBUÍDO QUANDO O WIFI É INICIALIZADO E PERMANECE ATIVO
//ATÉ QUE O NÚMERO DE TENTATIVAS EXPIRE (RESULTANDO EM WL_NO_SHIELD) OU QUE UMA CONEXÃO SEJA ESTABELECIDA
//(RESULTANDO EM WL_CONNECTED)

WiFiEspServer server(80);  //CONEXÃO REALIZADA NA PORTA 80

RingBuffer buf(8);  //BUFFER PARA AUMENTAR A VELOCIDADE E REDUZIR A ALOCAÇÃO DE MEMÓRIA

int statusLed = LOW;  //VARIÁVEL QUE ARMAZENA O ESTADO ATUAL DO LED (LIGADO / DESLIGADO)

// Variaveis de controle de contagem por interrupt
volatile int master_count = 0;  // universal count inicia com -1 para virar 0 quando o faz o setup do interrupt
char msg[15];
int volatile ismoving = 0;
char myString1[300];  // String for message




void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  //DEFINE O PINO COMO SAÍDA (LED_BUILTIN = PINO 13)
  blink(1);
  digitalWrite(LED_BUILTIN, LOW);  //PINO 13 INICIA DESLIGADO
  Serial.begin(9600);              //INICIALIZA A SERIAL
  Serial1.begin(9600);             //INICIALIZA A SERIAL PARA O ESP8266
  Serial.println("Inicio da comunicacao com o ES8266");
  WiFi.init(&Serial1);  //INICIALIZA A COMUNICAÇÃO SERIAL COM O ESP8266
  Serial.println("Fim da comunicacao com o ES8266");
  WiFi.config(IPAddress(192, 168, 23, 11));  //COLOQUE UMA FAIXA DE IP DISPONÍVEL DO SEU ROTEADOR

  //INÍCIO - VERIFICA SE O ESP8266 ESTÁ CONECTADO AO ARDUINO, CONECTA A REDE SEM FIO E INICIA O WEBSERVER
  if (WiFi.status() == WL_NO_SHIELD) {
    while (true)
      ;
  }
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
  }
  server.begin();
  //FIM - VERIFICA SE O ESP8266 ESTÁ CONECTADO AO ARDUINO, CONECTA A REDE SEM FIO E INICIA O WEBSERVER

  Serial.print("master_count pre setup: ");
  Serial.println(master_count);

  attachInterrupt(digitalPinToInterrupt(2), interrupt, RISING);  //  function for creating external interrupts at pin2 on Rising (LOW to HIGH)

  Serial.print("master_count pos setup: ");
  Serial.println(master_count);

  Serial.println("Fim do setup");
  blink(3);
}

void loop() {
  WiFiEspClient client = server.available();  //ATENDE AS SOLICITAÇÕES DO CLIENTE

  if (client) {                   //SE CLIENTE TENTAR SE CONECTAR, FAZ
    buf.init();                   //INICIALIZA O BUFFER
    while (client.connected()) {  //ENQUANTO O CLIENTE ESTIVER CONECTADO, FAZ
      if (client.available()) {   //SE EXISTIR REQUISIÇÃO DO CLIENTE, FAZ
        char c = client.read();   //LÊ A REQUISIÇÃO DO CLIENTE
        buf.push(c);              //BUFFER ARMAZENA A REQUISIÇÃO

        //IDENTIFICA O FIM DA REQUISIÇÃO HTTP E ENVIA UMA RESPOSTA
        if (buf.endsWith("\r\n\r\n")) {
          sprintf(msg, "%s", "");
          sendHttpResponse(client);
          break;
        }
        if (buf.endsWith("GET /H")) {       //SE O PARÂMETRO DA REQUISIÇÃO VINDO POR GET FOR IGUAL A "H", FAZ
          digitalWrite(LED_BUILTIN, HIGH);  //ACENDE O LED
          sprintf(msg, "%s", "ON");
          statusLed = 1;  //VARIÁVEL RECEBE VALOR 1(SIGNIFICA QUE O LED ESTÁ ACESO)
          sendHttpResponse(client);

        }                                  //SENÃO, FAZ
        if (buf.endsWith("GET /L")) {      //SE O PARÂMETRO DA REQUISIÇÃO VINDO POR GET FOR IGUAL A "L", FAZ
          digitalWrite(LED_BUILTIN, LOW);  //APAGA O LED
          sprintf(msg, "%s", "OFF");
          statusLed = 0;  //VARIÁVEL RECEBE VALOR 0(SIGNIFICA QUE O LED ESTÁ APAGADO)
          sendHttpResponse(client);
        }

        if (buf.endsWith("GET /R")) {  //Faz o reset e comunica
          sprintf(msg, "%s", "Reset!");
          sendHttpResponse(client);
          blink(8);
          reset();
        }

        if (buf.endsWith("GET /Z")) {  //Faz o reset e comunica
          master_count = 0;
          sprintf(msg, "%s", "zero_counts");
          sendHttpResponse(client);
          blink(5);
        }
      }
    }
  }
  client.stop();  //FINALIZA A REQUISIÇÃO HTTP E DESCONECTA O CLIENTE]


  if (ismoving) {
    blink(1);
    ismoving = 0;
  }
}




//MÉTODO DE RESPOSTA A REQUISIÇÃO HTTP DO CLIENTE
void sendHttpResponse(WiFiEspClient client) {

  float rain_mm = master_count * 0.233;

  StaticJsonDocument<200> doc;

  doc["time"] = millis() / 1000;
  doc["counts"] = master_count;
  doc["rain_mm"] = rain_mm;
  doc["msg"] = msg;


  // Write response headers
  client.println(F("HTTP/1.0 200 OK"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.print(F("Content-Length: "));
  client.println(measureJsonPretty(doc));
  client.println();

  // Write JSON document
  serializeJsonPretty(doc, client);

  // Disconnect
  client.stop();
}



void interrupt() {

  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > 100) {
    master_count += 1;
    sprintf(myString1, "%d", master_count);
    ismoving = 1;
  }
  last_interrupt_time = interrupt_time;
}


void blink(int x) {
  for (int i = 0; i < x; i++) {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(100);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(100);                       // wait for a second
  }
}


void reset() {
  Serial.print("Reset at ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  delay(200);
  resetFunc();  //Arduino will never pass that line
}
