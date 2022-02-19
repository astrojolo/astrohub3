void initializeExt() {
  pinMode(Vin_PIN, INPUT);
  pinMode(Vreg_PIN, INPUT);
  pinMode(Itot_PIN, INPUT);
  pinMode(V5_PIN, INPUT);

  // Turn relays on
  for(byte i = 0; i < 4; i++) {
    ctxOut.rels[i] = ctx.outPowerON[i];
    pinMode(relPins[i], OUTPUT);
  }
  updateRelays();
  //TODO - remove in PCB v.2
  pinMode(39, OUTPUT);
  digitalWrite(39, HIGH);

  // PWM init
  for(byte i = 0; i < 4; i++) {
    ctxOut.pwms[i] = 0;
    pinMode(pwmPins[i], OUTPUT);
  }
  ctxOut.coolerPWM = 0;
  ctxOut.heaterPWM = 0;
  updatePWM();  
  updatePWMFreqs();  
  timers.updatePtot = millis() + Ptot_CYCLE;    
  power.Izero = ctx.IzeroCalibration;
}  


void updatePtot() {
  timers.updatePtot = millis() + Ptot_CYCLE;
  int current = readCurrent(Itot_PIN);
  power.I = ((ASC_NEGATIVE) ? (power.Izero / 5) - current : current - (power.Izero / 5));
  power.Vin = readVoltage(Vin_PIN);
  power.Vreg = readVoltage(Vreg_PIN);
  power.V5 = readVoltage(V5_PIN);
  power.Itot += power.I;                 
  power.Ptot += ((float)power.I * (float)power.Vin);
  power.currents[power.curIndex] = current;
  power.vins[power.curIndex] = power.Vin;
  power.v5s[power.curIndex] = power.V5;
  power.vregs[power.curIndex] = power.Vreg;
  power.curIndex++;
  if(power.curIndex > 4) power.curIndex = 0;
}


void updatePWM() {
  calculateHeaterPWM();
  calculateCoolerPWM();
  for (byte i = 0; i < 4; i++) {updatePWMPin(pwmPins[i], ctxOut.pwms[i]);}
}

void updateRelays() {
  int beforeI, afterI = 0;
  for (byte i = 0; i < 3; i++) beforeI += readCurrent(Itot_PIN);
  for (byte i = 0; i < 4; i++) {digitalWrite(relPins[i], ctxOut.rels[i]);}
  for (byte i = 0; i < 3; i++) afterI += readCurrent(Itot_PIN);
  if((afterI - beforeI) > 120) { buzz(30,1); delay(100);}   // if current increased much use delay to stabilize (1A = 60ADU)
}

void updatePWMPin(byte pin, byte value) {
  analogWrite(pin, map(readPWMint(value), 0, 100, 0, 255));
}

int readPWMint(byte value) {
  byte pwm = value;
  if(pwm == 255) pwm = ctxOut.heaterPWM;
  if(pwm == 254) pwm = ctxOut.coolerPWM;
  return pwm;
}

void updatePWMFreqs() {
  setPWM14(ctx.pwm14);
  setPWM23(ctx.pwm23);
  setPWMDC(ctx.pwmDC);
  setPWMStepper(ctx.pwmSteppers);
}

// PWM freqs
// 0x01 - 31kHz, 0x02 - 4kHz, 0x03 - 490Hz, 0x04 - 122Hz
// n: 3             2            1             0

byte mapPrescaler(byte prescaler) {
  return (4 - prescaler);
}

void setPWM23(byte prescaler) { TCCR1B = (TCCR1B & 0xF8) | mapPrescaler(prescaler);}

void setPWM14(byte prescaler) { 
  prescaler = mapPrescaler(prescaler);
  if(prescaler == 0x04) prescaler = 0x06;
  if(prescaler == 0x03) prescaler = 0x04;
  TCCR2B = (TCCR2B & 0xF8) | prescaler;
}

void setPWMDC(byte prescaler) { TCCR4B = (TCCR4B & 0xF8) | mapPrescaler(prescaler);}

void setPWMStepper(byte prescaler) { TCCR5B = (TCCR5B & 0xF8) | mapPrescaler(prescaler);}

//PWM 2 i 3 - 12, 11 - timer 1
//PWM 1 i 4 - 9, 10 - timer 2
//DC PWM - timer 4
//STEPPERS PWM - 44, 45, 46 - timer 5, 45 - stepper2, 46 - stepper1, 44 - external stepper driver
// Timer 1 TCCR1B = (TCCR1B & 0xF8) | value ; 0x01 31kHz, 0x02 4kHz, 0x03 490Hz, 0x04 122Hz
// Timer 2 TCCR2B = (TCCR2B & 0xF8) | value ; 0x01 31kHz, 0x02 4kHz, 0x04 490Hz, 0x06 122Hz
// Timer 4 TCCR4B = (TCCR4B & 0xF8) | value ; 0x01 31kHz, 0x02 4kHz, 0x03 490Hz, 0x04 122Hz
// Timer 5 TCCR5B = (TCCR5B & 0xF8) | value ; 0x01 31kHz, 0x02 4kHz, 0x03 490Hz, 0x04 122Hz
// http://sobisource.com/arduino-mega-pwm-pin-and-frequency-timer-control/

// ---- SERIAL CALLBACKS -----------------------------------------------------------

void setADCCoeff(char *param) {
  char *token;
  float values[3]; byte i = 0;
  token = strtok(param, ":");
  while (token != NULL) {
    values[i] = atof(token);
    i++;
    token = strtok(NULL, ":");
  }
  if(i > 1) {
    ctx.VinCoeff = ((float) values[0]) / 1000.0;
    ctx.VregCoeff = ((float) values[1]) / 1000.0;
    ctx.V5Coeff = ((float) values[2]) / 1000.0;
    saveConfig();
  }
}

void printADCCoeff(char *answer) {
  char buf[8];
  dtostrf(ctx.VinCoeff, 5, 3, buf); strcat(answer, buf); strcat(answer, ":");
  dtostrf(ctx.VregCoeff, 5, 3, buf); strcat(answer, buf); strcat(answer, ":");
  dtostrf(ctx.V5Coeff, 5, 3, buf); strcat(answer, buf);
}

void setCoolingParams(char *param) { // O:0:40:-1:90:300:10:100:100:0:-1:0:40:90
  char *token;
  int values[13]; byte i = 0;
  token = strtok(param, ":");
  while (token != NULL) {
    values[i] = atoi(token);
    i++;
    token = strtok(NULL, ":");
  }
  if(i > 11) {
    ctx.coolTempSensor = (byte) values[0];
    ctx.coolTempPreset = (int) values[1]; 
    ctx.coolHumSensor = (int) values[2];
    ctx.coolHumTreshold = (byte) values[3];
    ctx.Kp = ((double) values[4]) / 1000.0;
    ctx.Ki = ((double) values[5]) / 10000.0;
    ctx.Kd = ((double) values[6]) / 10000.0;
    ctx.Kscale = ((double) values[7]);
    ctx.Kdir = (byte) values[8];    
    ctx.ToffsetSensor = values[9];
    ctx.pwmHumSensor = values[10];
    ctx.pwmHumStart = values[11];
    ctx.pwmHumFull = values[12];
    saveConfig();
  }
}

void printCoolingParams(char *answer) {
  char buf[8];
  sprintf(buf, "%d", ctx.coolTempSensor); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.coolTempPreset); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.coolHumSensor); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.coolHumTreshold); strcat(answer, buf); strcat(answer, ":");
  dtostrf(ctx.Kp, 5, 3, buf); strcat(answer, buf); strcat(answer, ":");
  dtostrf(ctx.Ki, 6, 4, buf); strcat(answer, buf); strcat(answer, ":");
  dtostrf(ctx.Kd, 6, 4, buf); strcat(answer, buf); strcat(answer, ":");
  dtostrf(ctx.Kscale, 5, 0, buf); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.Kdir); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.ToffsetSensor); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.pwmHumSensor); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.pwmHumStart); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.pwmHumFull); strcat(answer, buf); 
}

void setPWMPresc(char *param) {
  Params params = resolveParams(param);
  switch (param[0]) {
    case '1': ctx.pwm14 = params.value; break;
    case '2': ctx.pwm23 = params.value; break;
    case '3': ctx.pwmDC = params.value; break;
    case '4': ctx.pwmSteppers = params.value; break;
  }
  saveConfig();
  updatePWMFreqs();
}

void printPWMPresc(char *answer, char *param) {
  char buf[4]; memset(buf, '\0', sizeof(buf));
  switch (param[0]) {
    case '1': sprintf(buf, "%d", ctx.pwm14) ; break;
    case '2': sprintf(buf, "%d", ctx.pwm23) ; break;
    case '3': sprintf(buf, "%d", ctx.pwmDC) ; break;
    case '4': sprintf(buf, "%d", ctx.pwmSteppers) ; break;
  }
  strcat(answer, buf);
}


void calibrateIzero(char *answer) {
  analogWrite(LCD_BACKLIGHT_PIN, 0);
  int izero = 0;
  for(byte i = 0; i < 50; i++) izero += analogRead(Itot_PIN);
  ctx.IzeroCalibration = (int) ((ASC_NEGATIVE) ? (izero / 10 + 10) : (izero / 10 - 10));
  saveConfig();
  power.Izero = ctx.IzeroCalibration;
  buzz(20,1);
  lcdConfigChanged();
  char buf[8]; sprintf(buf, "%d", ctx.IzeroCalibration); strcat(answer, buf); 
}

void setPWM(char *param) {
  Params params = resolveParams(param);
  ctxOut.pwms[params.index] = params.value;
  updatePWM();
}

void setRelay(char *param) {
  Params params = resolveParams(param);
  ctxOut.rels[params.index] = params.value;
  updateRelays();
}

