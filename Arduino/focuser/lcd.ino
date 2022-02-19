void initializeLCD() {
  lcdConfigChanged();
  if(ctx.lcdConfig.enabled) lcdWelcome();
}

void updateLcd() {
  timers.updateLcd = millis() + 200;

  if(!ctx.lcdConfig.enabled) return;
  if(ctx.lcdConfig.disableUpdateDuringmove && areSteppersMoving()) return;

  displayScreens(LCDcurrentScreenIndex);
  if(LCDupdateCounter >= LCDnextScreenCounter && !areSteppersMoving()) {
    LCDupdateCounter = 0;
    byte displayTime = 0; int counter = 0;
    do {
      LCDcurrentScreenIndex++; counter++;
      if(LCDcurrentScreenIndex > 5) LCDcurrentScreenIndex = 0;
      displayTime = ctx.lcdConfig.lines[LCDcurrentScreenIndex][4];
    } while (displayTime == 0 && counter < 10);    
    LCDnextScreenCounter = 5 * ctx.lcdConfig.lines[LCDcurrentScreenIndex][4];
  }
  LCDupdateCounter++;
}

void lcdConfigChanged() {
  ctx.lcdConfig.enabled = lcdEnabled();
  LCDupdateCounter = 0;
  LCDcurrentScreenIndex = getFirstScreen();
  LCDnextScreenCounter = 5 * ctx.lcdConfig.lines[LCDcurrentScreenIndex][4];  
  lcd.begin(20,4);
  lcd.clear(); 
  lcd.noCursor();
  byte pwm = (ctx.lcdConfig.enabled) ? ctx.lcdConfig.pwm : 0;
  analogWrite(LCD_BACKLIGHT_PIN, map(pwm, 0, 100, 0, 255));
  analogWrite(LCD_CONTRAST_PIN, map(ctx.lcdConfig.contrast, 0, 100, 0, 255));
  timers.updateLcd = millis() + 1000;
  ctx.lcdConfig.enabled = lcdEnabled();
}

void displayScreens(byte index) {
  for(byte i = 0; i < 4; i++) {
    switch(ctx.lcdConfig.lines[index][i]) {
      case 1: lcdAstroHub(i); break;
      case 2: lcdFocusers(i); break;
      case 3: lcdFocuser1(i); break;
      case 4: lcdFocuser2(i); break;
      case 5: lcdPWMs(i); break;
      case 6: lcdCurrent(i); break;
      case 7: lcdOuts(i); break;
      case 8: lcdSensor1(i); break;
      case 9: lcdSensor2(i); break;
      case 10: lcdSensor3(i); break;
      case 11: lcdMLX(i); break;
      case 12: lcdDateTime(i); break;
      case 13: lcdVoltages(i); break;
      case 14: lcdGpsPosition(i); break;
      case 15: lcdElevation(i); break;
      default : lcdAstroHub(i);
    }
  }
}

// 01234567890123456789
// ********************
// **  AstroHub 3.0  **
// **  starting...   **
// ********************
void lcdWelcome() {
  lcd.setCursor(0,0); lcd.print("********************");
  lcd.setCursor(0,1); lcd.print("**  AstroHub 3.0  **");
  lcd.setCursor(0,2); lcd.print("**  starting...   **");
  lcd.setCursor(0,3); lcd.print("********************");
}

void lcdAstroHub(byte line) {     //1
  if(updateEvery(2) || areSteppersMoving()) return;
  lcd.setCursor(0,line); lcd.print("**  AstroHub 3.0  **");
}

// 01234567890123456789
// F1:123456  F2:123456
void lcdFocusers(byte line) { 
  char buf[21];
  long pos1 = motors[0].motor.currentPosition();
  long pos2 = motors[1].motor.currentPosition();
  sprintf(buf, "F1:%6ld  F2:%6ld", pos1, pos2);
  printLine(buf, line);
}

void lcdFocuser1(byte line) {lcdFocuser(line, 0);}
void lcdFocuser2(byte line) {lcdFocuser(line, 1);}

// 01234567890123456789
// Foc1 123456 123.45mm
void lcdFocuser(byte line, byte index) {
  char buf[21], tmp[7];
  byte i = index + 1;
  dtostrf((motors[index].motor.currentPosition() * ctx.steppers[index].stepSize / 1000), 6, 3, tmp);  
  sprintf(buf, "Foc%1d %6ld %smm", index+1, motors[index].motor.currentPosition(), tmp, ctx.steppers[index].compSensor);
  printLine(buf, line);
}

// 01234567890123456789
// PWM% 100 100 100 100
void lcdPWMs(byte line) {
  if(updateEvery(3) || areSteppersMoving()) return;
  char buf[21];
  sprintf(buf, "PWM%% %3d %3d %3d %3d", readPWMint(ctxOut.pwms[0]), readPWMint(ctxOut.pwms[1]), readPWMint(ctxOut.pwms[2]), readPWMint(ctxOut.pwms[3]));
  printLine(buf, line);  
}

// 01234567890123456789
// 12.0V   10.0V   5.1V
void lcdVoltages(byte line) { 
  if(updateEvery(3)) return;
  char tmp1[5], tmp2[5], tmp3[4], buf[21];
  dtostrf(ctx.VinCoeff * calcVoltage(averageInt(power.vins, 5)), 4, 1, tmp1);
  dtostrf(ctx.VregCoeff * calcVoltage(averageInt(power.vregs, 5)), 4, 1, tmp2);
  dtostrf(ctx.V5Coeff * calcVoltage(averageInt(power.v5s, 5)), 3, 1, tmp3);
  sprintf(buf, "%sV   %sV   %sV", tmp1, tmp2, tmp3);
  printLine(buf, line); 
}

// 01234567890123456789
// I=10.15A   E=213.1Ah
void lcdCurrent(byte line) { 
  if(updateEvery(3)) return;
  char tmp1[6], tmp2[7], buf[21];
  dtostrf(calcCurrent(power.currents, 5), 4, 2, tmp1);
  dtostrf(calcItot(power.Itot), 6, 2, tmp2);
  sprintf(buf, "I=%sA   E=%sAh", tmp1, tmp2);
  printLine(buf, line);
}

// 01234567890123456789
// OUT   ON  ON OFF OFF
void lcdOuts(byte line) {  
  if(updateEvery(3)) return;  
  char buf[21], out1[4], out2[4], out3[4], out4[4];
  strcpy(out1, (ctxOut.rels[0] > 0) ? " ON" : "OFF");
  strcpy(out2, (ctxOut.rels[1] > 0) ? " ON" : "OFF");
  strcpy(out3, (ctxOut.rels[2] > 0) ? " ON" : "OFF");
  strcpy(out4, (ctxOut.rels[3] > 0) ? " ON" : "OFF");
  sprintf(buf, "OUT  %s %s %s %s", out1, out2, out3, out4);
  printLine(buf, line); 
}

void lcdSensor1(byte line) {lcdSensor(line, 0);}
void lcdSensor2(byte line) {lcdSensor(line, 1);}
void lcdSensor3(byte line) {lcdSensor(line, 2);}

// 01234567890123456789
// S1:-12.3C 45% -23.1C
void lcdSensor(byte line, byte sensor) { 
  if(updateEvery(5) || areSteppersMoving()) return;
  char tmp1[6], tmp2[3], tmp3[6], buf[21];
  byte i = sensor + 1;
  if(sensors[sensor].type > 0) {
    dtostrf(sensors[sensor].temp, 5, 1, tmp1);
    dtostrf(sensors[sensor].hum, 2, 0, tmp2);
    dtostrf(sensors[sensor].dew, 5, 1, tmp3);
    sprintf(buf, "S%1d:%sC %s%% %sC", i, tmp1, tmp2, tmp3);
  } else {
    sprintf(buf, "           NO SENSOR");
  }  
  printLine(buf, line); 
}

// 01234567890123456789
// Ts:-23.5C  Ta:-12.3C
void lcdMLX(byte line) {  
  if(updateEvery(5) || areSteppersMoving()) return;
  char tmp1[6], tmp2[6], buf[21];
  if(cloudSensor.isThere) {
    dtostrf(cloudSensor.Tsky, 5, 1, tmp1);    
    dtostrf(cloudSensor.Tamb, 5, 1, tmp2);    
    sprintf(buf, "Ts:%sC  Ta:%sC", tmp1, tmp2);
  } else {
    sprintf(buf, "     NO CLOUD SENSOR");
  }
  printLine(buf, line); 
}

// 01234567890123456789
// 2016-03-04  12:33:32
void lcdDateTime(byte line) {
  char buf[21];
  if(timeStatus() == timeSet) {
    sprintf(buf, "%4d-%02d-%02d  %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second()); 
  } else {
    sprintf(buf, " DATE / TIME NOT SET");
  }
  printLine(buf, line);
}

// 01234567890123456789
// 53.12345N 123.12345W
void lcdGpsPosition(byte line) {  
  if(updateEvery(5) || areSteppersMoving()) return;
  char buf[21];  
  if(gps.location.isValid()) {
    float flat = gps.location.lat();
    float flon = gps.location.lng();
    unsigned long gpsAge;
    char tmp1[9], tmp2[10];
    dtostrf(flat, 7, 4, tmp1);  
    dtostrf(flon, 8, 4, tmp2);  
    sprintf(buf, " %s%s  %s%s", tmp1, (flat > 0) ? "N" : "S", tmp2, (flon > 0) ? "E" : "W");
  }
  else if(gps.charsProcessed() < 50) {
    sprintf(buf, "         NO GPS DATA");
  } else {
    sprintf(buf, "       GPS NOT FIXED");
  }
  printLine(buf, line);
}

// 01234567890123456789
// Alt 1234m   Sats: 32
void lcdElevation(byte line) { 
  if(updateEvery(5) || areSteppersMoving()) return;
  char buf[21], alt[5], sats[3];
  if(gps.charsProcessed() < 50) {
    sprintf(buf, "         NO GPS DATA");
  } else if(gps.satellites.isValid() || gps.altitude.isValid()) {
    if(gps.satellites.isValid()) {
      dtostrf(gps.satellites.value(), 2, 0, sats);
    } else strcat(sats, "--");
    
    if(gps.altitude.isValid()) {
      dtostrf(gps.altitude.meters(), 4, 0, alt);
    } else strcat(alt, "----");
    
    sprintf(buf, "Alt %sm   Sats: %s", alt, sats);
  } else {
    sprintf(buf, "       GPS NOT FIXED");
  }
  printLine(buf, line);
}

bool updateEvery(byte every) {return ((LCDupdateCounter % every) == 0) ;}

bool lcdEnabled() {
  if(!ctx.lcdConfig.enabled) return false;
  int sum = 0;
  for(byte i = 0; i < 6; i++) sum += ctx.lcdConfig.lines[i][4];
  return (sum > 0);
}

byte getFirstScreen() {
  for(byte i = 0; i < 6; i++) if(ctx.lcdConfig.lines[i][4] > 0) return i;
  return 0;
}

void printLine(char *buf, byte line) {
  lcd.setCursor(0, line);
  for(byte i = 0; i < 20; i++) {
    runSteppers(); lcd.write(buf[i]);
  }
}

void printLCDconfig(char *answer) {
  char buf[6];
  for(byte i = 0; i < 6; i++) {
    for(byte j = 0; j < 5; j++) {
      sprintf(buf, "%d", ctx.lcdConfig.lines[i][j]); strcat(answer, buf); strcat(answer, ":");
    }
  }
  sprintf(buf, "%d", (ctx.lcdConfig.enabled) ? 1 : 0); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", (ctx.lcdConfig.disableUpdateDuringmove) ? 1 : 0); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.lcdConfig.pwm); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.lcdConfig.contrast); strcat(answer, buf); 
}

void setLCDconfig(char *param) {
  char *token;
  byte values[34]; byte i = 0;
  token = strtok(param, ":");
  while (token != NULL) {
    values[i] = atoi(token);
    i++;
    token = strtok(NULL, ":");
  }
  if(i > 33) {
    for(byte i = 0; i < 6; i++) {
      for(byte j = 0; j < 5; j++) {
        ctx.lcdConfig.lines[i][j] = values[5*i + j];
      }
    }
    ctx.lcdConfig.enabled = (values[30] > 0);
    ctx.lcdConfig.disableUpdateDuringmove = (values[31] > 0);
    ctx.lcdConfig.pwm = values[32];
    ctx.lcdConfig.contrast = values[33];
    saveConfig();
    lcdConfigChanged();
  }  
}

