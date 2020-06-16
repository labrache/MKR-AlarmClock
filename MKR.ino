#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <TimeLib.h>
#include <timer.h>
#include "ArduinoJson.h"
#include <LCBUrl.h>
#include "config.h"
#include "arduino_secrets.h"

// *********** WIFI
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
WiFiClient client;
int clista = -1;

// *********** NTP
WiFiUDP ntpUDP;
int cur_hour = -1;
int cur_min = 0;

//*********** Segment LED
Adafruit_7segment matrix = Adafruit_7segment();
uint8_t dots = NODOT;

// *********** TimeUpd
auto dateUpdTimer = timer_create_default();

// *********** Stream
String streamhost;
String streampath;
int streamport;
uint8_t mp3buff[BUFFERSIZE];
File dataFile;
int playmode = 0;

// *********** Stream Example
char* fghost = "radiofg.impek.com";
String fgpath = "/fgd.mp3";
// *********** Stream Example
char* fihost = "direct.franceinfo.fr";
String fipath = "/live/franceinfo-midfi.mp3";

// *********** VS1053
Adafruit_VS1053 musicPlayer(-1, VS1053_CS, VS1053_DCS, VS1053_DREQ);
boolean sdcardpresence = false;
boolean sdloaded = false; 
boolean btnPlus_state = false;
boolean btnMoins_state = false;
boolean btnSnooze_state = false;
boolean switch_state = false;
int volume;

void setup() {
  Serial.begin(115200);
  setupRGBLed();
  matrix.begin(0x70);
  matrix.setBrightness(SEGBRIGHTNESS);
  matrix.clear();
  matrix.writeDisplay();
  
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    red(255);
    matrix.print(0xEE01, HEX);
    matrix.writeDisplay();
    while (true);
  }
  if (!musicPlayer.begin()) {
     Serial.println(F("VS1053 Error, pinout ?"));
     red(255);
     matrix.print(0xEE02, HEX);
     matrix.writeDisplay();
     while (1) delay(10);
  }
  volume = INITIAL_VOLUME;
  musicPlayer.setVolume(volumeconvert(volume), volumeconvert(volume));
  musicPlayer.GPIO_pinMode(buttonPlus, INPUT);
  musicPlayer.GPIO_pinMode(buttonSnooze, INPUT);
  musicPlayer.GPIO_pinMode(buttonMoins, INPUT);
  musicPlayer.GPIO_pinMode(switchBtn, INPUT);
  pinMode(CARDCD,INPUT_PULLUP);
  
  sdcardpresence = digitalRead(CARDCD);
  if(sdcardpresence){tryLoadSd();} else {
     matrix.print(0xEE03, HEX);
     matrix.writeDisplay();
  }
  WiFi.begin(ssid, pass);
  setupNTP(); 
  dateUpdTimer.every(1000, secondIdle);
  startServer();
  Serial.println("End Setup");

}

void loop() {
  if (Serial.available() > 0) {
    char input[INPUT_SIZE + 1];
    byte size = Serial.readBytesUntil(10, input, INPUT_SIZE);
    input[size] = 0;
    processSerial(input);
  }
  processButton();
  if(playmode == 1){
    if(sdloaded){
      if (musicPlayer.readyForData()) {
        if (dataFile.available()) {
          uint8_t bytesread = dataFile.read(mp3buff,BUFFERSIZE);
          musicPlayer.playData(mp3buff, bytesread);
        } else { stopPlaying();}
      }
    } else { stopPlaying();}
  }else if(playmode == 2){
    if(client.connected()){
      if (client.available() > 0) {
        if(canProcessMusic(client)){
          if (musicPlayer.readyForData()) {
            uint8_t bytesread = client.read(mp3buff, 32);
            musicPlayer.playData(mp3buff, bytesread);
          }
        }
      }
    } else {
      if(isRedirection(streamhost,streampath,streamport)){
        startWebPlaying();
      }
    }
  } else {
    handleClients();
    if(!isAlarmActive()){
      ArduinoOTA.poll();
    }
  }
  dateUpdTimer.tick();
}

bool secondIdle(void *) {
  controlWifi();
  if(cur_hour != hour() || cur_min != minute()){
    cur_hour = hour();
    cur_min = minute();
    segHandle();
    minutesprocess();
  }
  secondsprocessutils();
  secondprocessalarm();
  return true;
}
boolean tryLoadSd(){
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD fail"));
    sdloaded = false;
  } else {
    sdloaded = true;
    if(!loadAlarm()){
      Serial.println("Alarm not loaded");  
    }
  }
  return sdloaded;
}

void startWebPlaying(){
    char streamhostChar[streamhost.length()+1];
    streamhost.toCharArray(streamhostChar,streamhost.length()+1);
    startWebPlaying(streamhostChar,streampath,streamport);
}

void startWebPlaying(const String url){
  if(parseUrl(url,streamhost,streampath,streamport)){
    char streamhostChar[streamhost.length()+1];
    streamhost.toCharArray(streamhostChar,streamhost.length()+1);
    startWebPlaying(streamhostChar,streampath,streamport);
  } else {
    Serial.print("Error in url parse");
  }
}

void startWebPlaying(const char* host,const String path,const int port){
  if(WiFi.status() == WL_CONNECTED){
    Serial.print("CNX: ");  Serial.print(host); Serial.print(":"); Serial.println(port);
    if (client.connect(host, port)) {
      Serial.print("GET: "); Serial.println(path);           
      client.print(String("GET ") + path + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" + 
                   "Connection: keep-alive\r\n\r\n");
      prepareHandleHeader();
      playmode = 2;
    } else {
      Serial.println("Connection failed");
      startSdPlaying(FALLBACK_FILENAME);
    }
  } else {
    Serial.println("Not connected");
    startSdPlaying(FALLBACK_FILENAME);
  }
}

void startSdPlaying(const String filepath){
  if(sdloaded){
    if(SD.exists(filepath)){
      dataFile = SD.open(filepath);
      if(dataFile){
        playmode = 1;
        Serial.print("Playing: ");
      } else {Serial.print("Open error: ");}
    } else {Serial.print("Not existing: ");}
    Serial.println(filepath);
  } else {
    Serial.println("SD not loaded");
  }
}

void stopPlaying(){
  if(playmode == 2){
    client.flush();
    client.stop();
    Serial.println("Stop http stream");
  }
  if(playmode == 1){
    dataFile.close();
    Serial.println("Close file");
  }
  memset(mp3buff, 0, sizeof mp3buff);
  playmode = 0;
}

void processButton(){
  boolean btnPlusState = musicPlayer.GPIO_digitalRead(buttonPlus);
  boolean btnMoinsState = musicPlayer.GPIO_digitalRead(buttonMoins);
  boolean btnSnoozeState = musicPlayer.GPIO_digitalRead(buttonSnooze);
  boolean switchState = musicPlayer.GPIO_digitalRead(switchBtn);
  boolean sdcard = digitalRead(CARDCD);
  
  if(btnPlus_state != btnPlusState){
    btnPlus_state = btnPlusState;
    if(!btnPlus_state) {
      incVol();
    }
  }
  if(btnMoins_state != btnMoinsState){
    btnMoins_state = btnMoinsState;
    if(!btnMoins_state) {
      decVol();
    }
  }
  if(btnSnooze_state != btnSnoozeState){
    btnSnooze_state = btnSnoozeState;
    if(!btnSnooze_state) {
      snoozebtn();
    }
  }
  if(switch_state != switchState){
    switch_state = switchState;
    if(!switch_state) {
      desactiveAlarm();
    } else {
      activeAlarm();  
    }
  }
  if(sdcardpresence != sdcard){
    sdcardpresence = sdcard;
    if(sdcardpresence){tryLoadSd();} else {sdloaded = false;}
  }
}

void processSerial(char input[]){
    unsigned int pKey;
    String pVal;
    sscanf(input, "%u%s", &pKey, &pVal);
    Serial.print("Key: ");
    Serial.print(pKey);
    Serial.print(", Value: ");
    Serial.println(pVal);
    if(pKey == 1){
      stopPlaying();
    } else if(pKey == 2){
      copyIndex();
    } else if(pKey == 3){
      printDirectory(SD.open("/"), 0);
    } else if(pKey == 4){
      startWebPlaying(fihost,fipath,80);
    } else if(pKey == 5){
      startWebPlaying(fghost,fgpath,80);
    } else if(pKey == 6){
      Serial.println(pVal);
      startWebPlaying(pVal);
    }else if(pKey == 7){
      startWebPlaying("192.168.1.20","/",8080);
    }else if(pKey == 8){
      Serial.println("Dl...");
      downloadFiles("192.168.1.20","/data/stream.mp3",80,"/stream.mp3");
      Serial.println("Done");
    }else if(pKey == 9){
      startSdPlaying("/stream.mp3");
    }    
}
