const int timeZone = 1;

WiFiUDP Udp;
unsigned int localPort = 8888;

void setupNTP() 
{
  Udp.begin(localPort);
  Serial.println("NTP Setting provider");
  setSyncProvider(getNtpTime);
  setSyncInterval(86400);
}

time_t prevDisplay = 0;

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ;
  Serial.println("[NTP] Transmit NTP Request");
  sendNTPpacket();
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("[NTP] Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long secsSince1900;
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      time_t ntpTime = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      if(IsDST(ntpTime)){
        ntpTime = ntpTime + SECS_PER_HOUR;
      }
      return ntpTime;
    }
  }
  Serial.println("No NTP Response");
  return 0;
}

void sendNTPpacket()
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;            
  Udp.beginPacket(NTP_SERVER, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
