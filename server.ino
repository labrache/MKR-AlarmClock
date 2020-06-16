#include <HttpRequest.h>
#define BUFF_LENGTH 32
String sdFolder = "/www";

WiFiServer server(80);
HttpRequest httpReq;

void startServer(){
  server.begin();
}

void printHeader(WiFiClient &client, String code, String contentType = "text/html"){
  client.print("HTTP/1.1 ");
  client.println(code);
  client.print("Content-Type: ");
  client.println(contentType);
  client.println("Connnection: close");
  client.println();
}

void printMessage(WiFiClient &client, String message){
  client.print("<!DOCTYPE HTML><html><body><h1>");
  client.print(message);
  client.println("</h1></body></html>");
}

void serveFile(WiFiClient &client, String fileName, String contentType = "", boolean onRoot = false){
  char dataStream[BUFF_LENGTH];
  String filePath;
  if(onRoot){
    filePath = fileName;
  } else {
    filePath = sdFolder;
    filePath.concat(fileName);
  }
  if(SD.exists(filePath)) {
    File sdfile = SD.open(filePath,FILE_READ);
    if(!sdfile){
      printHeader(client,"404 Not found");
      printMessage(client,"Error");
    } else {
      if(contentType == ""){
        contentType = getContentType(filePath);
      }
      printHeader(client,"200 OK",contentType);
      while(sdfile.available()){
        int n = sdfile.read(dataStream, sizeof(dataStream));
        client.write(dataStream,n);
      }
    }
  } else {
      printHeader(client,"404 Not found");
      printMessage(client,"404");
  }
}

void handleClients(){
  char name[HTTP_REQ_PARAM_NAME_LENGTH], value[HTTP_REQ_PARAM_VALUE_LENGTH];
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        httpReq.parseRequest(c);       
        if (httpReq.endOfRequest()) {
          Serial.print(httpReq.method); Serial.print(" "); Serial.println(httpReq.uri);
          if(strcmp(httpReq.method,"GET") == 0){
            if(strcmp(httpReq.uri,"/") == 0){
              serveFile(client, "/index.htm");
            } else if (strcmp(httpReq.uri,"/status.json") == 0) {
              statusGet(client);
            } else if (strcmp(httpReq.uri,"/files.json") == 0) {
              filesGet(client);
            } else if (strcmp(httpReq.uri,"/alarm.json") == 0) {
              serveFile(client, ALARM_FILE, "application/json", true);
            } else {
              serveFile(client, httpReq.uri);
            }
          } else if(strcmp(httpReq.method,"POST") == 0){
            if (strcmp(httpReq.uri,"/alarm.json") == 0) {
              alarmPost(client,httpReq.postValue);
            }
          } else {
            printHeader(client,"404 Not found");
            printMessage(client,"404");        
          }
          httpReq.resetRequest();
          break;
        }
      }
    }
    delay(1);
    client.stop();
  }
}

void statusGet(WiFiClient &client) {
  DynamicJsonDocument doc(1024);
  doc["ssid"] = WiFi.SSID();
  doc["hour"] = hour();
  doc["min"] = minute();
  doc["sec"] = second();
  doc["day"] = weekday();
  doc["alarmsize"] = ALARM_LENGTH;
  doc["alarmstate"] = alarm_active;
  doc["sdcardpresence"] = sdcardpresence;
  doc["sdloaded"] = sdloaded;
  doc["volume"] = volume;

  printHeader(client,"200 OK","application/json");
  serializeJson(doc, client);
}

void alarmPost(WiFiClient &client, String json) {
  DynamicJsonDocument doc(1024);
  DynamicJsonDocument resp(1024);
  DeserializationError error = deserializeJson(doc, json);
  if (error){
    resp["result"] = "Error";
  } else {
    resp["result"] = "Ok";
    setAlarm(doc);
    if(SD.exists(ALARM_FILE)) {
      SD.remove(ALARM_FILE); 
    }
    File sfile = SD.open(ALARM_FILE, FILE_WRITE);
    //sfile.print(json);
    serializeJson(doc, sfile);
    sfile.close();
  }
  printHeader(client,"200 OK","application/json");
  serializeJson(resp, client);
}

void filesGet(WiFiClient &client) {
  DynamicJsonDocument doc(1024);
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
            doc.add(String(entryName));
        }
      }
    }
    entry.close();
  }
  printHeader(client,"200 OK","application/json");
  serializeJson(doc, client);
}

String getContentType(String filename) {
  if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".json")) {
    return "application/json";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}
