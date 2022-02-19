void initializeGPS() {
  if(!ctx.gpsConfig.enabled) return;
  gpsConfigChanged();
}

void serialEvent1() {
  if(ctx.gpsConfig.enabled && ctx.gpsConfig.serial == 0) {
    while(Serial1.available() > 0) gpsSerialEvent((char) Serial1.read());
  }
}

void serialEvent2() {
  if(ctx.gpsConfig.enabled && ctx.gpsConfig.serial == 1) {
    while(Serial2.available() > 0) gpsSerialEvent((char) Serial2.read());
  }
}

void serialEvent3() {
  if(ctx.gpsConfig.enabled && ctx.gpsConfig.serial == 2) {
    while(Serial3.available() > 0) gpsSerialEvent((char) Serial3.read());
  }
}

void gpsSerialEvent(char cc) {
  if(ctx.gpsConfig.passGPSdata && ctx.gpsConfig.serial != 1) Serial2.write(cc);
  if(gps.encode(cc)) {
    updateDateTime();
  }  
}

void gpsConfigChanged() {
  int gpsBitrate = 4800;
  if(ctx.gpsConfig.bitrate == 1) gpsBitrate = 9600;
  if(ctx.gpsConfig.bitrate == 2) gpsBitrate = 19200;
  if(ctx.gpsConfig.serial == 0) {
    Serial1.begin(gpsBitrate);
    Serial1.setTimeout(2000);
    Serial1.flush();     
  } else if (ctx.gpsConfig.serial == 1) {
    Serial2.begin(gpsBitrate);
    Serial2.setTimeout(2000);
    Serial2.flush();     
  } else if (ctx.gpsConfig.serial == 2) {
    Serial3.begin(gpsBitrate);
    Serial3.setTimeout(2000);
    Serial3.flush();     
  }   
  if(ctx.gpsConfig.passGPSdata && ctx.gpsConfig.serial != 1) {
    Serial2.begin(gpsBitrate);
    Serial2.setTimeout(2000);
    Serial2.flush();     
  }
}

bool updateDateTime() {
  static unsigned long lastTimeSync = 0;
  if(gps.date.isValid() && gps.time.isValid() && ((millis() - lastTimeSync) > 60000 || timeStatus() != timeSet)) {
    setTime(gps.time.hour() + ctx.gpsConfig.UTCoffset, gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
    lastTimeSync = millis();
  }
}

// r:2016:3:5:15:12:33:23<centiseconds>:123<time age>:54.1212:18.3123:123<ms>:224.1<m>:5<sats>:3<hdop>:13445.1<km>
void printGPSdata(char *answer) {
  char buf[12];
  if(gps.charsProcessed() < 50) {
    strcat(answer, "u:u:u:");         // no connection to GPS
  } else if(gps.date.isValid()) {
    sprintf(buf, "%d", gps.date.year()); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", gps.date.month()); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", gps.date.day()); strcat(answer, buf); strcat(answer, ":");
  } else strcat(answer, "x:x:x:"); 
  
  if(gps.time.isValid()) {
    sprintf(buf, "%d", gps.time.hour()); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", gps.time.minute()); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", gps.time.second()); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", gps.time.centisecond()); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", gps.time.age()); strcat(answer, buf); strcat(answer, ":");
  } else strcat(answer, "x:x:x:x:x:");
  if(gps.location.isValid()) {
    dtostrf(gps.location.lat(), 6, 4, buf); strcat(answer, buf); strcat(answer, ":");
    dtostrf(gps.location.lng(), 6, 4, buf); strcat(answer, buf); strcat(answer, ":");  
    sprintf(buf, "%d", gps.location.age()); strcat(answer, buf); strcat(answer, ":");
  } else strcat(answer, "x:x:x:");   // no fix
  if(gps.altitude.isValid()) {
    dtostrf(gps.altitude.meters(), 1, 0, buf); strcat(answer, buf); strcat(answer, ":");
  } else strcat(answer, "x:");
  if(gps.satellites.isValid()) {
    sprintf(buf, "%d", gps.satellites.value()); strcat(answer, buf); strcat(answer, ":");
  } else strcat(answer, "x:");      
  if(gps.hdop.isValid()) {
    sprintf(buf, "%d", gps.hdop.value()); strcat(answer, buf); strcat(answer, ":");
  } else strcat(answer, "x:");   
  if(gps.location.isValid()) {
    float distance = TinyGPSPlus::distanceBetween(gps.location.lat(), gps.location.lng(), ORIGIN_LAT, ORIGIN_LNG) / 1000;
    dtostrf(distance, 3, 1, buf); strcat(answer, buf);
  } else strcat(answer, "x"); 
}

void printDateTime(char *answer) {
  char buf[5];
  sprintf(buf, "%d", year()); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", month()); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", day()); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", hour()); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", minute()); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", second()); strcat(answer, buf);
}

void setDateTime(char *param) {
  char *token;
  int values[6]; byte i = 0;
  token = strtok(param, ":");
  while (token != NULL) {
    values[i] = atoi(token);
    i++;
    token = strtok(NULL, ":");
  }
  if(i > 5) {
    setTime(values[3],values[4],values[5],values[2],values[1],values[0]);
  }
}

void printGPSconfig(char *answer) {
  char buf[6];
  sprintf(buf, "%d", ctx.gpsConfig.serial); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.gpsConfig.bitrate); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.gpsConfig.UTCoffset); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", (ctx.gpsConfig.enabled) ? 1 : 0); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", (ctx.gpsConfig.passGPSdata) ? 1 : 0); strcat(answer, buf); 
}

void setGPSconfig(char *param) {
  char *token;
  byte values[5]; byte i = 0;
  token = strtok(param, ":");
  while (token != NULL) {
    values[i] = atoi(token);
    i++;
    token = strtok(NULL, ":");
  }
  if(i > 4) {
    ctx.gpsConfig.serial = values[0];
    ctx.gpsConfig.bitrate = values[1];
    ctx.gpsConfig.UTCoffset = values[2];
    ctx.gpsConfig.enabled = values[3] > 0;
    ctx.gpsConfig.passGPSdata = values[4] > 0;
    saveConfig();
    gpsConfigChanged();
  }  
}

