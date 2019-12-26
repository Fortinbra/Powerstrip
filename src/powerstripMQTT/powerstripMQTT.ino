#include <ArduinoJson.h>
#include <Ethernet.h>
#include <MQTT.h>
#include <RTCZero.h>
#include <SD.h>
#include <EthernetUdp.h>
#include <TimeLib.h>
#include <Dns.h>


byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress timeServer(132, 163, 4, 101);

File settings;

EthernetClient net;
MQTTClient client;
RTCZero rtc;
EthernetUDP Udp;
DNSClient dns;

const int timeZone = -6;
unsigned int localPort = 8888;

unsigned long lastMillis = 0;

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
    setSyncProvider(getNtpTime);
    rtc.begin();
    rtc.setTime(hour(), minute(), second());
    rtc.setDate(day(), month(), year());

    client.begin("certifiedglutenfree", net);
    client.onMessage(messageReceived);

    connect();
  }
}

void loop() {

  client.loop();

  if (!client.connected()) {
    connect();
  }

  maintainEthernet();

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
/*-------- NTP code ----------*/
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
