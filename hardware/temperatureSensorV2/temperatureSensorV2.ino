//Tutorial: https://github.com/CurlyWurly-1/ESP8266-WIFIMANAGER-MQTT/blob/master/MQTT_with_WiFiManager.ino
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager 
#include "PubSubClient.h"

// ############### CONSTANTS ################# //

//General
const char* SERVER_URL="192.168.2.106"; // Defined. Both MQTT and config servers
const String SERVER_PORT="5000"; // Defined
const int PERIODICITY=5000; // Defined milliseconds
const String PATH_CONFIG="/device?id="; // Fixed

//Device-specific
#define DHTTYPE DHT11 // DHT 11
#define DHTPIN 3 //PINO DIGITAL UTILIZADO PELO SENSOR
const String DEVICE_TYPE = "Temp";

// ############### DEFAULT VARIABLES ################# //

//General
String api_key; //recovered from web server
String device_id; //recovered from web server
int mqtt_port; //recovered from web server
String id_mqtt; // defined
String topic_publish; // defined
String topic_subscribe; // defined

//Device-specific
float temperature = 0; //VARIÁVEL DO TIPO FLOAT
float temperatureAnt = 0;
float humidity = 0; //VARIÁVEL DO TIPO FLOAT
float humidityAnt = 0;
char data[10];
char valueRequest[10];

// ############### OBJECTS ################# //

//General
WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
WiFiClient httpClient;
HTTPClient http;

//Device-specific
DHT dht(DHTPIN, DHTTYPE);

// ############# PROTOTYPES ############### //

//General
void initSerial();
void initWiFi();
void initConfig();
int httpRequest(String path);

//Device-specific
void initDHT();
void deviceControl();//recebe e envia informações do dispositivo

// ############## SETUP ################# //

void setup() {    

  initSerial();
  initWiFi();
  initConfig();
  mqtt.setServer(SERVER_URL, mqtt_port);  
    
  // ## JUST CHANGE IN THIS PART ## //
  
  initDHT(); 
  
  // ############################## //
}

// ############### LOOP ################# //

void loop() { 
  if (!mqtt.connected()) {
    reconnect();
  }  

  // ## JUST CHANGE IN THIS PART ## //
  
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
          api_key = obj["api_key"].as<char*>(); 
          device_id = obj["device_id"].as<char*>(); 
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

void initDHT(){
  dht.begin(); //Inicializa o sensor
}

void deviceControl() 
{
    
  temperatureAnt = temperature;
  temperature = dht.readTemperature(); 

  humidityAnt = humidity;
  humidity = dht.readHumidity();
    
  if(temperature != temperatureAnt && !(isnan(temperature))){
    //Serial.print("Temperatura = "); 
    //Serial.print(temperature); 
    //Serial.print(" °C      |       "); 
    //Serial.print("Umidade = "); 
    //Serial.print(humidity); 
    //Serial.println(); 

    snprintf(data, sizeof(data), "%.2f", temperature); 
    strcpy(valueRequest, "t|");
    strcat(valueRequest, data);
    //Serial.print("===============> "); 
    //Serial.println(valueRequest); 

    mqtt.publish(topic_publish.c_str(), valueRequest);
  }

  if(humidity != humidityAnt && !(isnan(humidity))){
    
    snprintf(data, sizeof(data), "%.2f", humidity); 
    strcpy(valueRequest, "h|");
    strcat(valueRequest, data);
    //Serial.print("===============> "); 
    //Serial.println(valueRequest); 
    //Serial.println(); 

    mqtt.publish(topic_publish.c_str(), valueRequest);
         
  }
    
}
