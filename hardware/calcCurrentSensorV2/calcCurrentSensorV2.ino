//Tutorial: https://github.com/CurlyWurly-1/ESP8266-WIFIMANAGER-MQTT/blob/master/MQTT_with_WiFiManager.ino
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "Arduino.h"
#include "Wire.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager 
#include "PubSubClient.h"

// ############### CONSTANTS ################# //

//Generalcd 
const char* SERVER_URL="200.19.187.67"; // Defined. Both MQTT and config servers
const String SERVER_PORT="50002"; // Defined
const int PERIODICITY=5000; // Defined milliseconds
const String PATH_CONFIG="/device?id="; // Fixed

//Device-specific
#define I2Caddress 0x48
#define MARGEMDEERRO 200
#define HOURSTOMILLIS 3600000.0   //Valor de 1 hora em millisegundos
#define CORRENTEMAXIMA 52     //Corrente maxima enviada para o ads1115 convertida na proporção 100A:50mA
#define TENSAOMAXIMA 0.512    //Tensão maxima lida no ads1115
const String DEVICE_TYPE = "Curr";

// ############### DEFAULT VARIABLES ################# //

//General
String api_key; //recovered from web server
String device_id; //recovered from web server
int mqtt_port; //recovered from web server
String id_mqtt; // defined
String topic_publish; // defined
String topic_subscribe; // defined

//Device-specific
float consumoTotal = 0;
unsigned long inicioTempoLigado =0;
unsigned long fimTempoLigado =0;
bool estaLigado = false;
double kwValue;
double maxValue = 0, correnteAtual = 0, maxValueAnt = 0;

int i = 0;
float multiplier = 0.000015625;

char data[20] = "";
char valueRequest[350] = "";

// Read two bytes and convert to full 16-bit int
int16_t convertedValue;
int16_t actualValue[19];

// ############### OBJECTS ################# //

//General
WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
WiFiClient httpClient;
HTTPClient http;

//Device-specific
//DHT dht(DHTPIN, DHTTYPE);

// ############# PROTOTYPES ############### //

//General
void initSerial();
void initWiFi();
void initConfig();
int httpRequest(String path);

//Device-specific
//void initWire();
void deviceControl();//recebe e envia informações do dispositivo

// ############## SETUP ################# //

void setup() {    

  initSerial();
  initWiFi();
  initConfig();
  mqtt.setServer(SERVER_URL, mqtt_port);  
    
  // ## JUST CHANGE IN THIS PART ## //
  
  //initWire(); 
  
  // ############################## //
}

// ############### LOOP ################# //

void loop() { 
  if (!mqtt.connected()) {
    reconnect();
  }  

  // ## JUST CHANGE IN THIS PART ## //
  
  calcularConsumo();
  Serial.print("Consumo Total: ");
  Serial.println(consumoTotal);
  snprintf(data, sizeof(data), "%f", consumoTotal);
  strcat(valueRequest, data);
  mqtt.publish(topic_publish.c_str(), valueRequest);
  consumoTotal = 0;
  strcpy(valueRequest, "c|");
  deviceControl();

  // ############################## //
 
  mqtt.loop();
  delay(PERIODICITY);    
}

// ############### PROTOTYPES IMPLEMENTATION ################# //

void initSerial() {
  Serial.begin(115200);
}

void initWiFi() {
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  res = wm.autoConnect((DEVICE_TYPE+ESP.getChipId()).c_str(),"password"); // password protected ap
  if(!res) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.reset();
    delay(5000);
  } else {
    //if you get here you have connected to the WiFi    
    Serial.println("Connected!");
  }
}

void initConfig() {
  int code = httpRequest(PATH_CONFIG+ESP.getChipId());
  if(code!=200) {
    Serial.println("Device configuration failed, ,reseting device...");
    delay(3000);
    ESP.reset();
    delay(5000);
  } else {
    //if you get here you have connected to the WiFi    
    id_mqtt = device_id+"_device";
    topic_publish = "/"+api_key+"/"+device_id+"/attrs";
    topic_subscribe = "/"+api_key+"/"+device_id+"/cmd";
    Serial.print("id_mqtt:");
    Serial.println(id_mqtt);
    Serial.print("topic_publish:");
    Serial.println(topic_publish);
    Serial.print("topic_subscribe:");
    Serial.println(topic_subscribe);
    Serial.println("Device configured!");
  }
}

int httpRequest(String path) {

  String address = SERVER_URL;
  String baseURL = "http://"+address+":"+SERVER_PORT+path;    
  Serial.print("Trying to get device config data from... ");
  Serial.println(baseURL);

  if(WiFi.status()== WL_CONNECTED){      
    http.begin(httpClient,(baseURL).c_str());
  
    // Send HTTP GET request
    int httpResponseCode = http.GET();
  
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      if (httpResponseCode==200) {        
        DynamicJsonDocument doc(1024);
        // Parse JSON object
        DeserializationError error = deserializeJson(doc, http.getString());
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return -9999;          
        } else {
          JsonObject obj = doc.as<JsonObject>();   
          api_key = obj["api_key"].as<const char*>(); 
          device_id = obj["device_id"].as<const char*>(); 
          mqtt_port = obj["mqtt_port"].as<int>();    
          Serial.print("api_key:");
          Serial.println(api_key);
          Serial.print("device_id:");
          Serial.println(device_id);
          Serial.print("mqtt_port:");
          Serial.println(mqtt_port);
        }        
      } else {        
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
    return httpResponseCode;
  } else {
    Serial.println("WiFi Disconnected");
    return -9999;
  }  
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (mqtt.connect(id_mqtt.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// ## DEFINE YOUR FUNCTIONS STARTING HERE ## //

void deviceControl() 
{
  
  Wire.begin(0, 2);  
  
  // Step 1: Point to Config register - set to continuous conversion
  Wire.beginTransmission(I2Caddress);

  // Point to Config Register
  Wire.write(0b00000001);

  // Write the MSB + LSB of Config Register
  // MSB: Bits 15:8
  // Bit  15    0=No effect, 1=Begin Single Conversion (in power down mode)
  // Bits 14:12   How to configure A0 to A3 (comparator or single ended)
  // Bits 11:9  Programmable Gain 000=6.144v 001=4.096v 010=2.048v .... 111=0.256v
  // Bits 8     0=Continuous conversion mode, 1=Power down single shot
  Wire.write(0b00001000);

  // LSB: Bits 7:0
  // Bits 7:5 Data Rate (Samples per second) 000=8, 001=16, 010=32, 011=64,
  //      100=128, 101=250, 110=475, 111=860
  // Bit  4   Comparator Mode 0=Traditional, 1=Window
  // Bit  3   Comparator Polarity 0=low, 1=high
  // Bit  2   Latching 0=No, 1=Yes
  // Bits 1:0 Comparator # before Alert pin goes high
  //      00=1, 01=2, 10=4, 11=Disable this feature
  Wire.write(0b11100010);

  // Send the above bytes as an I2C WRITE to the module
  Wire.endTransmission();

  // ====================================

  // Step 2: Set the pointer to the conversion register
  Wire.beginTransmission(I2Caddress);

  //Point to Conversion register (read only , where we get our results from)
  Wire.write(0b00000000);

  // Send the above byte(s) as a WRITE
  Wire.endTransmission();

  // =======================================

  // Step 3: Request the 2 converted bytes (MSB plus LSB)
  Wire.requestFrom(I2Caddress, 2);

  // Read the the first byte (MSB) and shift it 8 places to the left then read
  // the second byte (LSB) into the last byte of this integer
  convertedValue = -(Wire.read() << 8 | Wire.read()); //Sinal negativo se os pinos estiverem invertidos

  i ++;
  
  actualValue[i] = abs(convertedValue);

  
  if(i >= 15){
    
    for(int j=0; j <= i; j++){
      if(actualValue[j] > maxValue)
        maxValue = actualValue[j];
    }
    
    Serial.println(maxValue);
    if(maxValue > 100 && estaLigado == false){
      //Serial.println("Conectou");
      estaLigado = true;
      inicioTempoLigado = millis(); 
    }else if(maxValue <= 100 && estaLigado == true) {
      //Serial.println("Desconectou");
      fimTempoLigado = millis();
      estaLigado = false;
    }

    Serial.println(abs(maxValue - maxValueAnt));
    if(abs(maxValue - maxValueAnt) >= MARGEMDEERRO){
      calcularConsumo();  
    }
    maxValueAnt = maxValue;

    correnteAtual = (maxValue * multiplier * CORRENTEMAXIMA)/TENSAOMAXIMA; //Calculo para determinar o valor da corrente atual lida pelo sensor
    kwValue = (correnteAtual * 220)/1000; //Calculo para determinar o valor atual de Kw
    maxValue = 0;  

    
    delay(1000);
    delay(0);
    i=0;
  }
  
  for(int k=0; k<3000; k++){
    asm("nop");
  }
  
}

void calcularConsumo(){
  float tempoConsumido = 0;
  
  if(estaLigado == true){
    fimTempoLigado = millis();
  }

  
  //Serial.println(inicioTempoLigado);
  //Serial.println(fimTempoLigado);

  tempoConsumido = (float(fimTempoLigado - inicioTempoLigado)/HOURSTOMILLIS);
  inicioTempoLigado = millis();
  fimTempoLigado = inicioTempoLigado;

   //Serial.print("Corrente: ");
   //Serial.println(correnteAtual);
   //Serial.print("KW: ");
   //Serial.println(kwValue);
   //Serial.print("Tempo: ");
   //Serial.println(tempoConsumido);
  
  consumoTotal +=  tempoConsumido * kwValue;
}

