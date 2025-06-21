/*
 * original from https://www.hackster.io/mdraber/digital-clock-with-arduino-uno-rev4-wifi-s-rtc-an-led-matrix-484d2e
 * Nicu FLORICA (niq_ro) used NTP updates based on https://github.com/eremef/aur4_clock
 * v3a - updated number style 
 * v3b - moved seconds after hours (instead before minutes)
 */

#include <WiFiS3.h>
#include "RTC.h"
#include "Arduino_LED_Matrix.h"

#define SECRET_SSID "bbk2"
#define SECRET_PASS "internet2"

#define TIMEZONE_OFFSET_HOURS 3 //2

unsigned long currentMillis;
unsigned long previousMillis = 0;

// please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password (use for WPA, or use as key for WEP)

constexpr unsigned int LOCAL_PORT = 2390;  // local port to listen for UDP packets
constexpr int NTP_PACKET_SIZE = 48; // NTP timestamp is in the first 48 bytes of the message

int wifiStatus = WL_IDLE_STATUS;
IPAddress timeServer(162, 159, 200, 123); // pool.ntp.org NTP server
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

ArduinoLEDMatrix matrix;

byte Time[8][12] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};
/*
byte  Digits  [5][30]{                                                                 
{ 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1 },
{ 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 },
{ 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
};                                   
*/ 
byte  Digits  [5][30]{                                                                 
  { 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0 },
  { 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1 },
  { 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1 },
  { 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0 },
  };  

int currentSecond;
boolean secondsON_OFF = 1;
int hours, minutes, seconds, year, dayofMon;
String dayofWeek, month;

void displayDigit(int d, int s_x, int s_y){
  for (int i=0;i<3;i++)
    for (int j=0;j<5;j++)
      Time[i+s_x][11-j-s_y] = Digits[j][i+d*3];   
    
  matrix.renderBitmap(Time, 8, 12);
}


DayOfWeek convertDOW(String dow){
  if (dow == String("Mon")) return DayOfWeek::MONDAY;
  if (dow == String("Tue")) return DayOfWeek::TUESDAY;
  if (dow == String("Wed")) return DayOfWeek::WEDNESDAY;
  if (dow == String("Thu")) return DayOfWeek::THURSDAY;
  if (dow == String("Fri")) return DayOfWeek::FRIDAY;
  if (dow == String("Sat")) return DayOfWeek::SATURDAY;
  if (dow == String("Sun")) return DayOfWeek::SUNDAY;
}

Month convertMonth(String m){
 if (m == String("Jan")) return Month::JANUARY;
  if (m == String("Feb")) return Month::FEBRUARY;
  if (m == String("Mar")) return Month::MARCH;
  if (m == String("Apr")) return Month::APRIL;
  if (m == String("May")) return Month::MAY;
  if (m == String("Jun")) return Month::JUNE;
  if (m == String("Jul")) return Month::JULY;
  if (m == String("Aug")) return Month::AUGUST;
  if (m == String("Sep")) return Month::SEPTEMBER;
  if (m == String("Oct")) return Month::OCTOBER;
  if (m == String("Nov")) return Month::NOVEMBER;
  if (m == String("Dec")) return Month::DECEMBER;
}

void getCurTime(String timeSTR,String* d_w,int* d_mn, String* mn,int* h,int* m,int* s,int* y){
  
  *d_w = timeSTR.substring(0,3);
  *mn = timeSTR.substring(4,7);
  *d_mn = timeSTR.substring(8,11).toInt();
  *h = timeSTR.substring(11,13).toInt();
  *m = timeSTR.substring(14,16).toInt();
  *s = timeSTR.substring(17,19).toInt();
  *y = timeSTR.substring(20,24).toInt();
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address) {
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

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void connectToWiFi(){
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    wifiStatus = WiFi.begin(ssid, pass);
  }
  delay(5000);
  Serial.println("Connected to WiFi");
  printWifiStatus();
}

/**
 * Calculates the current unix time, that is the time in seconds since Jan 1 1970.
 * It will try to get the time from the NTP server up to `maxTries` times,
 * then convert it to Unix time and return it.
 * You can optionally specify a time zone offset in hours that can be positive or negative.
*/
unsigned long getUnixTime(int8_t timeZoneOffsetHours = 0, uint8_t maxTries = 5){
  // Try up to `maxTries` times to get a timestamp from the NTP server, then give up.
  for (size_t i = 0; i < maxTries; i++){
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);

    if (Udp.parsePacket()) {
      Serial.println("Packet received.");
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

      //the timestamp starts at byte 40 of the received packet and is four bytes,
      //or two words, long. First, extract the two words:
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      
      // Combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;

      // Now convert NTP time into everyday time:
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      unsigned long secondsSince1970 = secsSince1900 - seventyYears + (timeZoneOffsetHours * 3600);
      return secondsSince1970;
    } else {
      Serial.println("Packet not received. Trying again.");
    }
  }
  return 0;
}

void updateTime(){
  yield();  
  Serial.println("\nStarting connection to NTP server...");
  auto unixTime = getUnixTime(TIMEZONE_OFFSET_HOURS, 25);
  Serial.print("Unix time = ");
  Serial.println(unixTime);
  if(unixTime == 0){
    unixTime = getUnixTime(2,25);
  }
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);
  Serial.println("Time updated.");
}



void setup() {  
  Serial.begin(9600); 
  connectToWiFi();
  Udp.begin(LOCAL_PORT); 
  matrix.begin();
  RTC.begin();   
  updateTime();

   /*
  String timeStamp = __TIMESTAMP__;
  getCurTime(timeStamp,&dayofWeek,&dayofMon,&month,&hours,&minutes,&seconds,&year);
  RTCTime startTime(dayofMon, convertMonth(month) , year, hours, minutes, seconds, 
                    convertDOW(dayofWeek), SaveLight::SAVING_TIME_ACTIVE); 
  RTC.setTime(startTime);
  */
}

void loop(){
  currentMillis = millis();
  if(currentMillis - previousMillis > 43200000)
   { // 1000 * 60 * 60 * 12
    updateTime();
    previousMillis = currentMillis;
   }

  RTCTime currentTime;
  RTC.getTime(currentTime);
  if (currentTime.getSeconds()!=currentSecond)
  {
    secondsON_OFF ?  secondsON_OFF = 0 : secondsON_OFF = 1;
    displayDigit((int)(currentTime.getHour()/10),0,0);
    displayDigit(currentTime.getHour()%10,4,0);
    displayDigit((int)(currentTime.getMinutes()/10),0,7);
    displayDigit(currentTime.getMinutes()%10,4,7);
    Time[7][5]=secondsON_OFF;
    Time[7][6]=secondsON_OFF;
    currentSecond=currentTime.getSeconds();
    matrix.renderBitmap(Time, 8, 12);
    Serial.println(secondsON_OFF);
  }
  delay(500);
} // end main loop
