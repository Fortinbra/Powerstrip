int incomingByte = 0;
void loop()
{
  Serial.print(".");
  if (Serial.available() > 0)
  {
    incomingByte = Serial.read();
    Serial.println(incomingByte, DEC);
  }
}

void messageReceived(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Serial Begin");
}
