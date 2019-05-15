/*
MQTT to Serial bridge by Steven (2018)
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// OTA includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// DSMR lib include

#define DEBUG

void sendMQTTMessage(String topic, String message);

// Update these with values suitable for your network.
const char* ssid = "De Jong Airport";
const char* password = "Appeltaart";
const char* mqtt_server = "raspi";
const char* mqtt_debug_topic = "debug/mqttserial/debug";
const char* mqtt_topic_tx = "mqtt_serial/tx";
const char* mqtt_topic_rx = "mqtt_serial/rx";

WiFiClient espClient;
PubSubClient client(espClient);

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

unsigned long reconnect_at_time = 0;

void print_to_mqtt(String message) {
#ifdef DEBUG
  sendMQTTMessage(mqtt_debug_topic, message);
  Serial1.println("debug: " + message);
#endif
}

void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
    Serial1.println("Connecting to ");
    Serial1.println(ssid);
    WiFi.mode(WIFI_STA);  //station only, no accesspoint
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      //Serial1.println(".");
    }
  randomSeed(micros());
  Serial1.println("");
  Serial1.println("WiFi connected");
  Serial1.println("IP address: ");
  Serial1.println(WiFi.localIP());

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname("mqtt_serial_VLT_esp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial1.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial1.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial1.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial1.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial1.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial1.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial1.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial1.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial1.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial1.println("Ready");
  Serial1.print("IP address: ");
  Serial1.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial1.println("Command received at: ");
  Serial1.println(topic);
  Serial1.print("Data: ");
  Serial1.print((char*)payload);
  Serial1.print(" len: ");
  Serial1.println(length);
//if(strcmp(topic, mqtt_topic_tx) == 0) {
  Serial.write((char*)payload, 22);
// }
} //end callback

void reconnect() {
  // dont Loop until we're reconnected
  Serial1.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  //if you MQTT broker has clientID,username and password
  //please change following line to    if (client.connect(clientId,userName,passWord))
  if (client.connect(clientId.c_str()))
  {
    Serial1.println("connected");
   //once connected to MQTT broker, subscribe command if any
    Serial1.print("Subscribing to: ");
    Serial1.println(mqtt_topic_tx);
    client.subscribe(mqtt_topic_tx);
  } else {
    Serial1.println("failed, rc=");
    //Serial1.println(client.state());
  }
} //end reconnect()

void sendMQTTMessage(String topic, String message) {
    char msg[50];
    char tpc[50];
    topic.toCharArray(tpc, 50);
    message.toCharArray(msg,50);
    Serial1.print("Sending MQTT");
    Serial1.print("Topic: ");
    Serial1.println(topic);
    Serial1.print("msg: ");
    Serial1.print(msg);
    //publish sensor data to MQTT broker
    client.publish(tpc, msg);
    Serial1.println("..sent");
}

void setup() {
  Serial.begin(1200); //used for communication
#ifdef DEBUG
  Serial1.begin(115200);  // cannot receive data - used for debug output
#endif
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  inputString.reserve(50);
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void getSerial() {
  if (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    //Serial1.print("inputString: ");
    //Serial1.println(inputString);
    // if the incoming character is a '>', set a flag
    // so the main loop can do something about it:
    if (inChar == '>') {
      stringComplete = true;
    }
  }
}

void loop() {
  if (!client.connected() && millis() > reconnect_at_time) {
    reconnect();
    //failed to reconnect?
    if (!client.connected()) {
      //try again 6seconds later
      Serial1.println("Reconnect failed, trying again in 6 seconds");
      reconnect_at_time = millis()+6000;
    }
  }
  client.loop();
  ArduinoOTA.handle();
  getSerial();

  //when '>' is seen send it
  if (stringComplete) {
    Serial1.print("inputString: ");
    Serial1.println(inputString);
    sendMQTTMessage(mqtt_topic_rx, inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
 
}
