#include <NTPClient.h>
#include <ArduinoJson.h>
#include <Ethernet.h>
#include <MQTT.h>
#include <RTCZero.h>
#include <SD.h>
#include <EthernetUdp.h>
#include <Dns.h>


byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress timeServer(132, 163, 4, 101);

File settings;

EthernetClient net;
MQTTClient client;
RTCZero rtc;
EthernetUDP Udp;
DNSClient dns;
NTPClient timeClient(Udp);

const int timeZone = -6;
unsigned int localPort = 1883;

unsigned long lastMillis = 0;

void loop() {

  client.loop();

  if (!client.connected()) {
    connect();
  }

  maintainEthernet();

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

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(A6, OUTPUT);
  pinMode(0, OUTPUT);



  //SD.begin(SDCARD_SS_PIN);
  //settings = SD.open("settings.json", FILE_WRITE);

  //const size_t capacity = JSON_ARRAY_SIZE(7) + JSON_OBJECT_SIZE(3) + 110;
  //DynamicJsonBuffer jsonBuffer(capacity);

  //  const char* json = "{\"clientID\":\"powerStrip1\",\"timeServerURL\":\"pool.ntp.org\",\"outlets\":[true,false,true,false,true,false,true]}";

  //JsonObject& root = jsonBuffer.parseObject(settings.read());

  //  const char* clientID = root["clientID"]; // "powerStrip1"
  const char* timeServerURL = "pool.ntp.org";//root["timeServerURL"];

  //  JsonArray& outlets = root["outlets"];
  //  bool outlets_0 = outlets[0]; // true
  //  bool outlets_1 = outlets[1]; // false
  //  bool outlets_2 = outlets[2]; // true
  //  bool outlets_3 = outlets[3]; // false
  Serial.println("Initialize Ethernet with DHCP:");
  bool success = true;
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }

    // no point in carrying on, so do nothing forevermore:
    success = false;
  }
  if (!success) {
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
    dns.begin(Ethernet.dnsServerIP());
    if (dns.getHostByName(timeServerURL, timeServer) == 1) {
      Serial.print(F("ntp = "));
      Serial.println(timeServer);
    }
    else Serial.print(F("dns lookup failed"));
    Udp.begin(localPort);
    Serial.println("waiting for sync");
    //setSyncProvider(getNtpTime);
    rtc.begin();
    rtc.setEpoch(timeClient.getEpochTime());
    //rtc.setTime(timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
    //rtc.setDate(timeClient.getDay(),timeClient.getMonth(), timeClient.getYear());

    client.begin("ASUS-MAXIMUS", net);
    client.onMessage(messageReceived);

    connect();
  }
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
