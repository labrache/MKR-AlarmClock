typedef struct alarmStruct
{
    unsigned int alarm_hour;
    unsigned int alarm_min;
    bool alarm_day[7];
    unsigned int alarm_type;
    char* alarm_href;
};
alarmStruct alarms[ALARM_LENGTH];
bool ringing = false;
bool snooze = false;
bool alarm_active = false;

unsigned int snoozedelay;
unsigned int alarm_type;
String alarm_href;

void activeAlarm(){
  alarm_active = true;
}
void desactiveAlarm(){
  alarm_active = false;
  if(ringing){
    ringing = false;
    stopPlaying();
  }
  if(snooze){
    snooze = false;
    segPrint(NODOT);
  }
}

bool isAlarmActive(){
  return alarm_active;
}

void minutesprocess(){
  if(alarm_active && checkAlarm(cur_hour, cur_min, weekday(),alarm_type,alarm_href)){
    ring();
  }
}

void secondprocessalarm(){
  if(snooze){
    if(snoozedelay > 0){
      snoozedelay--;
    } else {
      snooze = false;
      segPrint(NODOT);
      ring();
    }
  }
}

void ring(){
  ringing = true;
  if(snooze) {
    snooze = false;
    segPrint(NODOT);
  }
  if(alarm_type == 1) {
    startWebPlaying(String(alarm_href));
  } else if (alarm_type == 2){
    if(alarm_href == ""){
      startSdPlaying(randomFile());
    } else {
      startSdPlaying(alarm_href);
    }
    
  }
}

void snoozebtn(){
  if(ringing){
    ringing = false;
    snooze = true;
    snoozedelay = SNOOZETIME;
    stopPlaying();
    segPrint(UPDOT);
  } else if(snooze) {
    snooze = false;
    segPrint(NODOT);
  } else if (playmode != 0){
    stopPlaying();
  } else {
    startWebPlaying(fihost,fipath,80);
  }
}

bool loadAlarm(){
    if(SD.exists(ALARM_FILE)){
      File sfile = SD.open(ALARM_FILE, FILE_READ);
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, sfile);
      if (error){
        return false;
      } else {
        setAlarm(doc);
        return true;
      }
      sfile.close();
    } else {
      return false;
    }
}

void setAlarm(DynamicJsonDocument doc){
  JsonArray array = doc.as<JsonArray>();
  memset(alarms, 0, sizeof(alarms));
  int alarmIndex = 0;
  for(JsonVariant v : array) {
    if(v["en"].as<bool>()){ 
      sscanf (v["h"].as<char*>(),"%u:%u",&alarms[alarmIndex].alarm_hour,&alarms[alarmIndex].alarm_min);
      JsonArray wdarray = v["w"].as<JsonArray>();
      for(JsonVariant wd : wdarray) {
        alarms[alarmIndex].alarm_day[wd.as<unsigned int>()] = true;
      }
      JsonObject a = v["a"].as<JsonObject>();
      alarms[alarmIndex].alarm_href = const_cast<char*>(a["href"].as<char*>());
      if(a["src"] == "url"){
          alarms[alarmIndex].alarm_type = 1;
      } else if (a["src"] == "sd") {
          alarms[alarmIndex].alarm_type = 2;
      } else {
        Serial.println("Error src");
      }
    }
    alarmIndex++;
  }
}

bool checkAlarm(const int ahour,const int aminute,const int aday, unsigned int &alarm_type,String &alarm_href){
  for(int i = 0;i<ALARM_LENGTH;i++){
    if(alarms[i].alarm_day[aday]){
      if(alarms[i].alarm_hour == ahour && alarms[i].alarm_min == aminute){
        alarm_type = alarms[i].alarm_type;
        alarm_href = alarms[i].alarm_href;
        return true;
      }
    }
  }
  return false;
}
