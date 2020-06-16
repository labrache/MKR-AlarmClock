#include <utility/wifi_drv.h>
#include <ArduinoHttpClient.h>

int volsegtimeout = 0;
int oldWstatus = -99;
bool data_read = false;
int read_status = 0;
bool waitForRedir = false;
bool gotRedir = false;
bool gotOk = false;
String newLocation = "";

void prepareHandleHeader(){
  data_read = false;
  read_status = 0;
  gotOk = false;
}

bool isRedirection(String& host, String& path, int& port){
  if(gotRedir) {
    gotRedir = false;
    return parseUrl(newLocation,host,path,port);
  } else {
    return false;
  }
}

bool canProcessMusic(WiFiClient &client){
  if(data_read){
    return true;
  } else {
    String line = client.readStringUntil('\n');
    if(line.length() == 1){
      if(gotOk){
        data_read = true;
      }
    } else {
      if(read_status == 0){
        if(line.startsWith("HTTP")){
          int startBlank = line.indexOf(" ")+1;
          int httpCodeEnd = line.indexOf(" ",startBlank);
          String httpCode = line.substring(startBlank,httpCodeEnd);
          Serial.print("HTTP Code:");
          Serial.println(httpCode);
          switch(httpCode.toInt()){
            case 302:
              waitForRedir = true;
              break;
            case 301:
              waitForRedir = true;
              break;
            case 200:
              gotOk = true;
              break;
          }
          read_status = 1;
        }
      }else if(read_status == 1){
        if(waitForRedir){
          if(line.startsWith("Location:")){
            newLocation = line.substring(line.indexOf(" ")+1,line.length()-1);
            gotRedir = true;
            waitForRedir = false;
            Serial.print("Got new location: ");
            Serial.println(newLocation);
            client.flush();
            client.stop();
          }
        }
      }
    }
  }
  return false;
}

bool isMusic(String filename) {
  if (filename.endsWith(".mp3")) return true;
  else if (filename.endsWith(".wma")) return true;
  else if (filename.endsWith(".flac")) return true;
  else if (filename.endsWith(".wav")) return true;
  return false;
}
void printDirectory(File dir, int numTabs) {
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}
bool parseUrl(const String aurl, String& host, String& path, int& port){
  // https://github.com/lbussy/LCBUrl/blob/master/examples/Basic/Basic.ino
    LCBUrl url;
     if (!url.setUrl(aurl)) {
        return false;
    } else {
        host = url.getHost();
        port = url.getPort();
        path = "/"+url.getPath();
        return true;
    }
}

String randomFile() {
  int fileChoose = 0;
  for(int i = 0; i < 2; i++){
    int fileCount = 0;
    File dir = SD.open("/");
    bool explore = true;
    while(explore)
    {
      File entry =  dir.openNextFile();
      if(!entry){
        explore = false;
      } else {
        if (!entry.isDirectory()) {
          String entryName = String(entry.name());
          entryName.toLowerCase();
          if (isMusic(entryName)) {
            if(i == 1){
              if(fileCount == fileChoose){
                return entryName;
              }
            }
            fileCount++;
          }
        }
      }
      entry.close();
    }
    if(i == 0){
       fileChoose = random(0,fileCount);
    }
    dir.close();
  }
  return "ERROR";
}

void setupRGBLed(){
  WiFiDrv::pinMode(LED_GREEN, OUTPUT);  //GREEN
  WiFiDrv::pinMode(LED_RED, OUTPUT);  //RED
  WiFiDrv::pinMode(LED_BLUE, OUTPUT); //BLUE
}

void blue(int val){
  WiFiDrv::analogWrite(LED_BLUE, val);
}
void red(int val){
  WiFiDrv::analogWrite(LED_RED, val);
}
void green(int val){
  WiFiDrv::analogWrite(LED_GREEN, val);
}

void segPrint(int8_t middot) {
  dots = middot;
  segPrint();
}
void segPrint() {
  int8_t DispData[4] = {cur_hour / 10,cur_hour % 10,cur_min / 10,cur_min % 10};
  matrix.clear();
  matrix.writeDigitNum(0, DispData[0]);
  matrix.writeDigitNum(1, DispData[1]);
  matrix.writeDigitNum(2, dots);
  matrix.writeDigitNum(3, DispData[2]);
  matrix.writeDigitNum(4, DispData[3]);
  matrix.writeDisplay();
  if(volsegtimeout != 0){
    volsegtimeout = 0;
  }
}
void volPrint() {
  int8_t DispData[2] = {volume / 10,volume % 10};
  matrix.clear();
  matrix.writeDigitNum(2, NODOT);
  matrix.writeDigitNum(3, DispData[0]);
  matrix.writeDigitNum(4, DispData[1]);
  matrix.writeDisplay();
  volsegtimeout = 2;
}


void secondsprocessutils(){
  if(volsegtimeout > 0){
    volsegtimeout--;
    if(volsegtimeout == 0){
      segPrint();
    }
  }
}

void segHandle(){
  if(volsegtimeout == 0){
    segPrint();
  }
}

void incVol() {
  if(volume < MAX_VOLUME){
    volume++;
    musicPlayer.setVolume(volumeconvert(volume), volumeconvert(volume));
  }
  volPrint();
}
void decVol() {
  if(volume > MIN_VOLUME){
    volume--;
    musicPlayer.setVolume(volumeconvert(volume), volumeconvert(volume));
  }
  volPrint();
}

int volumeconvert(int input){
  return MIN_VOLUME_RESISTANCE + (((MAX_VOLUME_RESISTANCE - MIN_VOLUME_RESISTANCE) / MAX_VOLUME) * (MAX_VOLUME - input));
}


void controlWifi(){
  if(WiFi.status() != oldWstatus){
    switch (WiFi.status()) {
      case WL_CONNECTED:
        red(0);
        blue(0);
        ArduinoOTA.begin(WiFi.localIP(), OTA_NAME, OTA_PW, InternalStorage);
        Serial.println("WL_CONNECTED");
        break;
      case WL_IDLE_STATUS:
        blue(255);
        Serial.println("WL_IDLE_STATUS");
        break;
      case WL_NO_SSID_AVAIL:
        red(255);
        Serial.println("WL_NO_SSID_AVAIL");
        break;
      case WL_CONNECT_FAILED:
        red(255);
        Serial.println("WL_CONNECT_FAILED");
        break;
      case WL_CONNECTION_LOST:
        red(255);
        ArduinoOTA.end();
        Serial.println("WL_CONNECTION_LOST");
        break;
      case WL_DISCONNECTED:
        red(255);
        ArduinoOTA.end();
        Serial.println("WL_DISCONNECTED");
        break;
    }
    oldWstatus = WiFi.status();
  }
}

void copyIndex(){
  downloadFiles("192.168.1.20","/data/index.html",80,"/www/index.htm");
  downloadFiles("192.168.1.20","/data/alarm.js",80,"/www/alarm.js");
}

void downloadFiles(const char* aServerName, const char* aPathName, uint16_t aServerPort, const char* aFileName){
  WiFiClient wifi = server.available();
  HttpClient client = HttpClient(wifi, aServerName, aServerPort);
  const int kNetworkTimeout = 30*1000;
  const int kNetworkDelay = 1000;
  uint8_t dlData[32];
  if(client.get(aPathName) == 0){
    int statusCode = client.responseStatusCode();
    if(statusCode == 200){
    if(SD.exists(aFileName)){
      Serial.print("Remove existing:"); Serial.println(aFileName);
      SD.remove(aFileName);
    }
    Serial.print("open:"); Serial.println(aFileName);
    File f = SD.open(aFileName, FILE_WRITE);
    if(f){
      int bodyLen = client.contentLength();
      unsigned long timeoutStart = millis();
       while ( (client.connected() || client.available()) &&
             (!client.endOfBodyReached()) &&
             ((millis() - timeoutStart) < kNetworkTimeout) )
      {
          size_t asize = client.available();
          if (asize)
          {
              uint8_t bytesread = client.read(dlData, ((asize > sizeof(dlData)) ? sizeof(dlData) : asize));
              f.write(dlData, bytesread);
              timeoutStart = millis();
          }
          else
          {
              delay(kNetworkDelay);
          }
      }
      f.close();
      Serial.print("End of write "); Serial.println(aFileName);
    } else {
      Serial.print("Error opening "); Serial.println(aFileName);
    }
    } else {
      Serial.print("HTTP ");
      Serial.println(statusCode);
    }
  }
  client.stop();
  wifi.stop();
}

boolean IsDST(time_t t)
{
  tmElements_t te;
  te.Year = year(t)-1970;
  te.Month =3;
  te.Day =1;
  te.Hour = 0;
  te.Minute = 0;
  te.Second = 0;
  time_t dstStart,dstEnd, current;
  dstStart = makeTime(te);
  dstStart = nextSunday(dstStart);
  dstStart = nextSunday(dstStart); //second sunday in march
  dstStart += 2*SECS_PER_HOUR;//2AM
  te.Month=11;
  dstEnd = makeTime(te);
  dstEnd = nextSunday(dstEnd); //first sunday in november
  dstEnd += SECS_PER_HOUR; //1AM
  return (t>=dstStart && t<dstEnd);
}
