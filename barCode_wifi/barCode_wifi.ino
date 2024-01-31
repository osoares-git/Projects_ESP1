void (*resetFunc)(void) = 0; // Reset function
// DECLARAÇÃO DE VARIÁVEIS  ESP1 CLIENT

#include "WiFiEsp.h"

// Emulate Serial1 on pins 2/3 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(2, 3);  // RX, TX
#endif
char ssid[] = "WIFI_SSID";          // your network SSID (name)
char pass[] = "password";    // your network password
int status = WL_IDLE_STATUS;  // the Wifi radio's status

// Inicializando o servidor
char server[] = "192.168.23.10";  // endereço do servidor
#define NUMPORT 9000              // porta de comunicação com o servidor

// Initialize the Ethernet client object
WiFiEspClient client;


////////////////////////////
#include <usbhid.h>
#include <usbhub.h>
#include <hiduniversal.h>
#include <hidboot.h>
#include <SPI.h>
///////////////////////////
String stringTwo = String("");
byte status_leitura = false;
///////////////////

class MyParser : public HIDReportParser {
public:
  MyParser();
  void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
protected:
  uint8_t KeyToAscii(bool upper, uint8_t mod, uint8_t key);
  virtual void OnKeyScanned(bool upper, uint8_t mod, uint8_t key);
  virtual void OnScanFinished();
};


MyParser::MyParser() {}


void MyParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  // If error or empty, return
  if (buf[2] == 1 || buf[2] == 0) return;

  for (uint8_t i = 7; i >= 2; i--) {
    // If empty, skip
    if (buf[i] == 0) continue;

    // If enter signal emitted, scan finished
    if (buf[i] == UHS_HID_BOOT_KEY_ENTER) {
      OnScanFinished();
    }

    // If not, continue normally
    else {
      // If bit position not in 2, it's uppercase words
      OnKeyScanned(i > 2, buf, buf[i]);
    }

    return;
  }
}


uint8_t MyParser::KeyToAscii(bool upper, uint8_t mod, uint8_t key) {
  // Letters
  if (VALUE_WITHIN(key, 0x04, 0x1d)) {
    if (upper) return (key - 4 + 'A');
    else return (key - 4 + 'a');
  }

  // Numbers
  else if (VALUE_WITHIN(key, 0x1e, 0x27)) {
    return ((key == UHS_HID_BOOT_KEY_ZERO) ? '0' : key - 0x1e + '1');
  }

  return 0;
}


void MyParser::OnKeyScanned(bool upper, uint8_t mod, uint8_t key) {
  uint8_t ascii = KeyToAscii(upper, mod, key);
  //Serial.print((char)ascii);  // Aqui imprime o valor lido na serial
  stringTwo.concat((char)ascii);
}


void MyParser::OnScanFinished() {
  //Entra nessa funcao quando terminar o scan
  //Serial.println("");
  status_leitura = true;  // Finalizado o scan ja eh possivel realizar a transmissao
}


USB Usb;
USBHub Hub(&Usb);
HIDUniversal Hid(&Usb);
MyParser Parser;


void setup() {

  Serial.begin(9600);
  Serial1.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial1);
  IPAddress ip(192, 168, 23, 180);  //IP Address
  WiFi.config(ip);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    //while (true);
  }

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  // you're connected now, so print out the data
  Serial.println("You're connected to the network");
  printWifiStatus();
  Serial.println();
  Serial.println("Fim do setup da antena");

  if (Usb.Init() == -1) {
    Serial.println("Erro ao iniciar o Leitor");
  } else {
    Serial.println("Leitor OK!");
  }
  delay(200);
  Hid.SetReportParser(0, &Parser);
}


void loop() {

  Usb.Task();

  if (status_leitura) {
    printToServer();
    status_leitura = false;
  }

  if (millis() / 1000 % 86400 == 0) {  //RESET 24H
    reset();
  }
}


void reset() {
  Serial.print("Reset at ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  delay(200);
  resetFunc();  //Arduino will never pass that line
}


void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


void printToServer() {
  //Verify if connected to the server
  if (client.connect(server, NUMPORT)) {
    Serial.println("Connected to the server");
  }

  client.print(stringTwo);
  Serial.println(stringTwo);
  stringTwo = String("");
  client.stop();
}
