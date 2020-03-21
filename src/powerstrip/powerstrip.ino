#include <Arduino.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <RTCZero.h>
#include <MQTT.h>
#include <MQTTClient.h>
#include <NTPClient.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

EthernetUDP Udp;
NTPClient timeClient(Udp);
File settings;
EthernetClient net;
MQTTClient client;
RTCZero rtc;
struct Config {
  char hostname[64];
  bool outlet1;
  bool outlet2;
  bool outlet3;
  bool outlet4;
  bool outlet5;
  bool outlet6;
  bool outlet7;
  bool outlet8;
};
const char *filename = "/config.txt";  // <- SD library uses 8.3 filenames
Config config;
// Loads the configuration from a file
void loadConfiguration(const char *filename, Config &config) {
  // Open file for reading
  File file = SD.open(filename);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  strlcpy(config.hostname,                  // <- destination
          doc["hostname"] | "example.com",  // <- source
          sizeof(config.hostname));         // <- destination's capacity

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}
// Saves the configuration to a file
void saveConfiguration(const char *filename, const Config &config) {
  // Delete existing file, otherwise the configuration is appended to the file
  SD.remove(filename);

  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["hostname"] = config.hostname;
 

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}
// Prints the content of a file to the Serial
void printFile(const char *filename) {
  // Open file for reading
  File file = SD.open(filename);
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  while (!SD.begin()) {
    Serial.println(F("Failed to initialize SD library"));
    delay(1000);
  }
  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  loadConfiguration(filename, config);
  // Dump config file
  Serial.println(F("Print config file..."));
  printFile(filename);
  Serial.println("Initialize Ethernet with DHCP");
  bool success = false;
  while (!success) {
    if (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP");
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found. Sorry, that's kinda important here.");
      } else if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is unplugged");
      }
      success = false;
    } else {
      success = true;
    }
  }
  if (success) {
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
    Serial.println(Ethernet.gatewayIP());
    Serial.println(Ethernet.dnsServerIP());
    Serial.println(Ethernet.subnetMask());
    timeClient.begin();
    Serial.println("waiting for sync");
    time_t epoch = timeClient.getEpochTime();
    rtc.begin();
    rtc.setTime(hour(), minute(), second());
    rtc.setDate(day(), month(), year());
    Serial.println(rtc.getHours());
    //    client.begin("certifiedglutenfree", net);
    //    client.onMessage(messageReceived);
    //
    //    connect();
  }
}

void loop() {
  client.loop();
  if (!client.connected()) {

  }
  maintainEthernet();
  timeClient.update();
}

void maintainEthernet() {
  switch (Ethernet.maintain()) {
    case 1:
      //renewed fail
      Serial.println("Error: renewed fail");
      break;

    case 2:
      //renewed success
      Serial.println("Renewed success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    case 3:
      //rebind fail
      Serial.println("Error: rebind fail");
      break;

    case 4:
      //rebind success
      Serial.println("Rebind success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    default:
      //nothing happened
      break;
  }
}
void connect() {

  Serial.print("MQTT connecting...");
  while (!client.connect("arduino", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nMQTT connected!");

  client.subscribe("powerstrip");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}
