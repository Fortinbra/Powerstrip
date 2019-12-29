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
DNSClient dns;

const int timeZone = -6;
unsigned int localPort = 8888;
unsigned long lastMillis = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  const char* timeServerURL = "pool.ntp.org";
  Serial.println("Initialize Ethernet with DHCP");
  bool success = true;
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found. Sorry, that's kinda important here.");
    } else if (Ethernet.linkStatus() == LinkOFF{
    Serial.println("Ethernet cable is unplugged");
    }
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

    rtc.begin();
    rtc.setTime(hour(), minute(), second());
    rtc.setDate(day(), month(), year());

    client.begin("certifiedglutenfree", net);
    client.onMessage(messageReceived);

    connect();
  }
  timeClient.begin();
}




void loop() {
  client.loop();
  if (!client.connected()) {
    connect();
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
