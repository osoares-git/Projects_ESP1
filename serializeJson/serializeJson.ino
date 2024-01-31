#include "WiFiEsp.h"         //INCLUSÃO DA BIBLIOTECA
#include "SoftwareSerial.h"  //INCLUSÃO DA BIBLIOTECA
#include <ArduinoJson.h>

SoftwareSerial Serial1(6, 7);  //PINOS QUE EMULAM A SERIAL, ONDE O PINO 6 É O RX E O PINO 7 É O TX

char ssid[] = "WIFI_SSID";     //VARIÁVEL QUE ARMAZENA O NOME DA REDE SEM FIO
char pass[] = "WIFI_password";  //VARIÁVEL QUE ARMAZENA A SENHA DA REDE SEM FIO

int status = WL_IDLE_STATUS;  //STATUS TEMPORÁRIO ATRIBUÍDO QUANDO O WIFI É INICIALIZADO E PERMANECE ATIVO
//ATÉ QUE O NÚMERO DE TENTATIVAS EXPIRE (RESULTANDO EM WL_NO_SHIELD) OU QUE UMA CONEXÃO SEJA ESTABELECIDA
//(RESULTANDO EM WL_CONNECTED)

WiFiEspServer server(80);  //CONEXÃO REALIZADA NA PORTA 80

RingBuffer buf(8);  //BUFFER PARA AUMENTAR A VELOCIDADE E REDUZIR A ALOCAÇÃO DE MEMÓRIA

int statusLed = LOW;  //VARIÁVEL QUE ARMAZENA O ESTADO ATUAL DO LED (LIGADO / DESLIGADO)

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);    //DEFINE O PINO COMO SAÍDA (LED_BUILTIN = PINO 13)
  digitalWrite(LED_BUILTIN, LOW);  //PINO 13 INICIA DESLIGADO
  Serial.begin(9600);              //INICIALIZA A SERIAL
  Serial1.begin(9600);             //INICIALIZA A SERIAL PARA O ESP8266
  Serial.println("Inicio da comunicacao com o ES8266");
  WiFi.init(&Serial1);  //INICIALIZA A COMUNICAÇÃO SERIAL COM O ESP8266
  Serial.println("Fim da comunicacao com o ES8266");
  WiFi.config(IPAddress(192, 168, 23, 12));  //COLOQUE UMA FAIXA DE IP DISPONÍVEL DO SEU ROTEADOR

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

  Serial.println("Fim do setup");
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
          sendHttpResponse(client);
          break;
        }
        if (buf.endsWith("GET /H")) {       //SE O PARÂMETRO DA REQUISIÇÃO VINDO POR GET FOR IGUAL A "H", FAZ
          digitalWrite(LED_BUILTIN, HIGH);  //ACENDE O LED
          statusLed = 1;                    //VARIÁVEL RECEBE VALOR 1(SIGNIFICA QUE O LED ESTÁ ACESO)

        } else {                             //SENÃO, FAZ
          if (buf.endsWith("GET /L")) {      //SE O PARÂMETRO DA REQUISIÇÃO VINDO POR GET FOR IGUAL A "L", FAZ
            digitalWrite(LED_BUILTIN, LOW);  //APAGA O LED
            statusLed = 0;                   //VARIÁVEL RECEBE VALOR 0(SIGNIFICA QUE O LED ESTÁ APAGADO)
          }
        }
      }
    }
    client.stop();  //FINALIZA A REQUISIÇÃO HTTP E DESCONECTA O CLIENTE]
  }
}






//MÉTODO DE RESPOSTA A REQUISIÇÃO HTTP DO CLIENTE
void sendHttpResponse(WiFiEspClient client) {

  StaticJsonDocument<200> doc;
  doc["module"] = "U-08";
  doc["time"] = millis() / 1000;
  JsonArray data = doc.createNestedArray("status");

  if (statusLed) {
    data.add("ativado");
  } else {
    data.add("desligado");
  }

  data.add(statusLed);


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
