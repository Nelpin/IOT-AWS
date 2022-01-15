/*
 * Ejemplo leer temperatura y humedad
 * Sensor DHT11 y ESP32s
 * Nelson Pinto
 */

#include "SPIFFS.h"
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

/*
 * Conexión a la Red 
*/
const char* ssid = "NEMOJO.";
const char* password="1098633329";
const char* mqtt_server ="a111lbzdszydeo-ats.iot.us-east-2.amazonaws.com";//---------cambiar
const int mqtt_port= 8883;

String Read_rootca;
String Read_cert;
String Read_privatekey;

#define BUFFER_LEN 256
long lastMsg = 0;
char msg[BUFFER_LEN];
int value = 0;
byte mac[6];
char mac_Id[18];
int count = 1;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi() {  
  delay(10);
  Serial.println();
  Serial.println("Conectando...");
  Serial.println(ssid);

  WiFi.begin(ssid,password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println(".");  
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("Conectado");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());  
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.println("Mensaje recibido [");
  Serial.println(topic);
  Serial.println("]");
  for(int i=0;i<length;i++){
    Serial.print((char)payload[i]);  
  }
  Serial.println();
}

void reconnect(){
  while(!client.connected()){
    Serial.println("Intenado conectarse a MQTT...");
    // Creando un ID aleatorio
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    // Intentando conectarse
    if (client.connect("ESPthing")) {
      Serial.println("Conectada");  
      // Conectado y publicando un payload
      client.publish("ei_out","hello world");
      // y suscribiendo
      client.subscribe("ei_in");      
    }else{
      Serial.println("Falló, rc=");
      Serial.println(client.state());
      Serial.println("Esperando 5 segundos");
      delay(5000);  
    }
  }
}

void setup(){
  Serial.begin(115200);
  dht.begin();
  Serial.setDebugOutput(true);
  // Inicializa con el PIN led2
  pinMode(2, OUTPUT);
  setup_wifi();
  delay(1000);

  if(!SPIFFS.begin(true)){
    Serial.println("Se ha producico un error al montar SPIFFS");
    return;
  }

  // Root CA leer el archivo
  File file2 = SPIFFS.open("/AmazonRootCA1.pem","r");//cambiar
  if(!file2){
    Serial.println("No se pudo abrir el archivo CA");  
    return;
  }

  Serial.println("Contenido del archivo root CA");
  while(file2.available()){
    Read_rootca = file2.readString();
    Serial.println(Read_rootca);
  }

  // Cert leer el archivo
  File file4 = SPIFFS.open("/certificate.pem.crt","r");
  if(!file4){
    Serial.println("No se pudo abrir el archivo cert");  
    return;
  }

  Serial.println("Contenido del archivo root Cert");
  while(file4.available()){
    Read_cert = file4.readString();
    Serial.println(Read_cert);
  }

  // Private key leer el archivo
  File file6 = SPIFFS.open("/private.pem.key","r");
  if(!file6){
    Serial.println("No se pudo abrir el archivo key");  
    return;
  }

  Serial.println("Contenido del archivo key");
  while(file6.available()){
    Read_privatekey = file6.readString();
    Serial.println(Read_privatekey);
  }

  //=======================================
  char* pRead_rootca;
  pRead_rootca = (char *)malloc(sizeof(char)* (Read_rootca.length() + 1));
  strcpy(pRead_rootca,Read_rootca.c_str());

  char* pRead_cert;
  pRead_cert = (char *)malloc(sizeof(char)* (Read_cert.length() + 1));
  strcpy(pRead_cert,Read_cert.c_str());

  char* pRead_privateKey;
  pRead_privateKey = (char *)malloc(sizeof(char)* (Read_privatekey.length() + 1));
  strcpy(pRead_privateKey,Read_privatekey.c_str());

  Serial.println("====================================");
  Serial.println();
  Serial.println("ROOT CA:");
  Serial.println(pRead_rootca);
  Serial.println("====================================");
  Serial.println();
  Serial.println("CERT;");
  Serial.println(pRead_cert);
  Serial.println("====================================");
  Serial.println();
  Serial.println("PrivateKey;");
  Serial.println(pRead_privateKey);
  Serial.println("====================================");

  espClient.setCACert(pRead_rootca);
  espClient.setCertificate(pRead_cert);
  espClient.setPrivateKey(pRead_privateKey);

  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);

  WiFi.macAddress(mac);
  snprintf(mac_Id, sizeof(mac_Id),"%02x:%02x:%02x:%02x:%02x:%02x:",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.println(mac_Id);
  delay(2000);
}

void loop() {
  // Antes de leer Temp, hum
  //Esperar 2 segundos entre cada lectura
  delay(2000);

  float h = dht.readHumidity();
  // Leer temperatura ºC
  float t = dht.readTemperature();
  // si se le pasa a la funcion el parametro true obtenemos la temperatura en ºF
  //float f = dht.readTemperature(true);

  if(isnan(h) || isnan(t)){
    Serial.println("Error al leer el sensor");
    return;  
  }

  if(!client.connected()){
    reconnect();  
  }

  client.loop();

  long now = millis();

  if(now - lastMsg > 5000){
    lastMsg = now;

      String macIdStr = mac_Id;
      String Temperature = String(t);
      String Humidity = String(h);
      snprintf(msg,BUFFER_LEN,"{\"mac_Id\" : \"%s\",\"Temperatura\" : \"%s\",\"Humendad\" : \"%s\}", macIdStr,Temperature,Humidity);
      Serial.println("Publicando mensaje");
      Serial.println(count);
      Serial.println(msg);
      client.publish("sensor",msg);
      count = count + 1;
  }

//  Serial.print("Humedad: ");
//  Serial.print(h);
//  Serial.print("%  Temperatura: ");
//  Serial.print(t);
//  Serial.print("°C  ");
//  Serial.print(f);
//  Serial.print("°F");
//  Serial.print('\n');

}
