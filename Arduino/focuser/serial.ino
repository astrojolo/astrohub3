void initializeSerial() {
  // Initialize serial
  Serial.begin(115200);
  Serial.setTimeout(2000);
  Serial.flush();

  memset(serialInput, '\0', sizeof(serialInput));
}

// Interrupt serial event
void serialEvent() {
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      serialCommand(serialInput);
      memset(serialInput, '\0', sizeof(serialInput));
    }
    else {
      byte len = strlen(serialInput);
      serialInput[len] = inChar;
      serialInput[len + 1] = '\0';
    }
  }
}

// COMMAND SET
// R - move to new position
// P,p - set, get position
// i - get in move
// H - halt motor
// J,j - set, get buzzer on
// X,x - set, get max focuser position
// t - get temperature
// h - get humidity
// d - get dewpoint
// E,e - set, get temp compensation coefficients + compensation sensor
// B,b - set, get PWM
// C,c - set, get relays '0' means Vreg
// q - get monitoring values
// a - get small monitoring values
// u - get batch init parameters
// U - set batch init parameters
// F,f - set, get PWM prescaler (see ext.ino file)
// L,l - set, get PWM humidity sensor
// G - move DCmotor <dir>:<pwm>:<time>
// g - DCmotor status
// K - halt DCmotor
// N,n - set, get ADC voltage coefficients
// O,o - set, get cooling parameters
// M,m - set, get owner nickname
// A - get firmware
// Q - calibrate I zero
// v,V - get, set LCD config
// s,S - get, set date/time
// x,X - get, set GPS config
// r - get GPS data

void serialCommand(char *command) {
  char param[120];   memset(param, '\0', sizeof(param));
  char answer[120]; memset(answer, '\0', sizeof(answer));
  char buf[15];
  Params p;

  byte commandLen = strlen(command);
  if (commandLen > 2) {
    strncpy(param, command + 2, commandLen - 2);
    param[commandLen - 2] = '\0';
  }
  strncpy(answer, command, 1);
  strcat(answer, ":");

  switch (command[0]) {
    case '#': strcat(answer, DEVICE_RESPONSE); buzz(50, 1); break;
    case 'R': runStepper(param); break;
    case 'p': sprintf(buf, "%ld", motors[atoi(param)].motor.currentPosition()); strcat(answer, buf); break;
    case 'P': setPosition(param); break;
    case 'i': strcat(answer, ((motors[atoi(param)].motor.distanceToGo() != 0) ? "1" : "0")); break;
    case 'l': sprintf(buf, "%d", ctx.pwmHumSensor); strcat(answer, buf); break;
    case 'L': ctx.pwmHumSensor = atoi(param); saveConfig(); break;
    case 'H': motors[atoi(param)].motor.stop(); break;
    case 'j': sprintf(buf, "%d", ctx.buzzer); strcat(answer, buf); break;
    case 'J': ctx.buzzer = atoi(param); saveConfig(); break;
    case 't': printTemp(answer); break;
    case 'h': printHum(answer); break;
    case 'd': printDew(answer); break;
    case 'e': printTempComp(answer); break;
    case 'E': setTempComp(param); break;
    case 'b': sprintf(buf, "%d", readPWMint(ctxOut.pwms[atoi(param)])); strcat(answer, buf); break;
    case 'B': setPWM(param); saveConfig(); break;
    case 'c': sprintf(buf, "%d", ctxOut.rels[atoi(param)]); strcat(answer, buf); break;
    case 'C': setRelay(param); saveConfig(); break;
    case 'f': printPWMPresc(answer, param); break;
    case 'F': setPWMPresc(param); saveConfig(); break;
    case 'G': runDCmotor(param); break;
    case 'g': sprintf(buf, "%d", ((digitalRead(DCMOTOR_A) == HIGH || digitalRead(DCMOTOR_B) == HIGH) ? 1 : 0)); strcat(answer, buf); break;
    case 'K': stopDCmotor(); break;
    case 'q': printMonitor(answer); break;
    case 'a': printMonitorQuick(answer); break;
    case 'u': printConfiguration(answer); break;
    case 'U': setConfiguration(param); break;
    case 'n': printADCCoeff(answer); break;
    case 'N': setADCCoeff(param); break;
    case 'o': printCoolingParams(answer); break;
    case 'O': setCoolingParams(param); break;
    case 'M': EepromUtil::eeprom_write_string(NICK_START, param); break;
    case 'm': printOwnerNick(answer); break;
    case 'A': strcat(answer, FIRMWARE); break;
    case 'Q': calibrateIzero(answer); break;
    case 'v': printLCDconfig(answer); break;
    case 'V': setLCDconfig(param); break;
    case 's': printDateTime(answer); break;
    case 'S': setDateTime(param); break;
    case 'x': printGPSconfig(answer); break;
    case 'X': setGPSconfig(param); break;
    case 'r': printGPSdata(answer); break;

    default: strcat(answer, " error"); buzz(50, 3);
  }
  Serial.println(answer);
}


void printConfiguration(char *answer) {
  char buf[8];
  for (byte i = 0; i < 2; i++) {
    sprintf(buf, "%ld", ctx.steppers[i].maxPos); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", ctx.steppers[i].stepperSpeed); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", ctx.steppers[i].pwmStop); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", ctx.steppers[i].pwmRun); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", ctx.steppers[i].acc); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", ctx.steppers[i].reversed); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", ctx.steppers[i].stepperMode); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", ctx.steppers[i].compSensor); strcat(answer, buf); strcat(answer, ":");
    sprintf(buf, "%d", (int) (100 * ctx.steppers[i].stepSize)); strcat(answer, buf); strcat(answer, ":");
  }
  sprintf(buf, "%d", ctx.pwm14); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.pwm23); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.pwmDC); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.pwmSteppers); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.buzzer); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.manualStepperControl); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.pwmHumSensor); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.DCmotorReversed); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.outPowerON[0]); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.outPowerON[1]); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.outPowerON[2]); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.outPowerON[3]); strcat(answer, buf);
}


void setConfiguration(char *param) {
  char *token;
  long values[30]; byte i = 0;
  token = strtok(param, ":");
  while (token != NULL && i < 30) {
    values[i] = atol(token);
    i++;
    token = strtok(NULL, ":");
  }
  if (i >= 29) {
    for (i = 0; i < 2; i++) {
      ctx.steppers[i].maxPos = values[0 + 9 * i];
      if (ctx.steppers[i].maxPos < motors[i].motor.currentPosition()) {
        motors[i].motor.setCurrentPosition(ctx.steppers[i].maxPos);
        motors[i].positionSaved = true;
        saveFocuserPos(motors[i].motor.currentPosition(), i);
      }
      ctx.steppers[i].stepperSpeed = (int) values[1 + 9 * i];
      ctx.steppers[i].pwmStop = (byte) values[2 + 9 * i];
      ctx.steppers[i].pwmRun = (byte) values[3 + 9 * i];
      ctx.steppers[i].acc = (int) values[4 + 9 * i];
      ctx.steppers[i].reversed = (int) values[5 + 9 * i];
      ctx.steppers[i].stepperMode = (byte) values[6 + 9 * i];
      ctx.steppers[i].compSensor = (int) values[7 + 9 * i];
      ctx.steppers[i].stepSize =  ((float) values[8 + 9 * i]) / 100.0;
      writeCtxToStepper(i);
    }
    ctx.pwm14 = (byte) values[18];
    ctx.pwm23 = (byte) values[19];
    ctx.pwmDC = (byte) values[20];
    ctx.pwmSteppers = (byte) values[21];
    ctx.buzzer = (byte) values[22];
    ctx.manualStepperControl = (byte) values[23];
    ctx.pwmHumSensor = (byte) values[24];
    ctx.DCmotorReversed = (byte) values[25];
    ctx.outPowerON[0] = (byte) values[26];
    ctx.outPowerON[1] = (byte) values[27];
    ctx.outPowerON[2] = (byte) values[28];
    ctx.outPowerON[3] = (byte) values[29];
    saveConfig();
    updatePWMFreqs();
  }
}

void printMonitorQuick(char *combined) {
  char buf[20];
  runSteppers();
  sprintf(buf, "%ld:%d:", motors[0].motor.currentPosition(), motors[0].motor.distanceToGo());
  strcat(combined, buf);
  runSteppers();
  sprintf(buf, "%ld:%d", motors[1].motor.currentPosition(), motors[1].motor.distanceToGo());
  strcat(combined, buf);
  runSteppers();
}

void printMonitor(char *combined) {
  char buf[20];
  runSteppers();
  sprintf(buf, "%ld:%d:", motors[0].motor.currentPosition(), motors[0].motor.distanceToGo());
  strcat(combined, buf);
  runSteppers();
  sprintf(buf, "%ld:%d:", motors[1].motor.currentPosition(), motors[1].motor.distanceToGo());
  strcat(combined, buf);
  runSteppers();
  dtostrf(calcCurrent(power.currents, 5), 4, 2, buf); strcat(combined, buf); strcat(combined, ":");
  runSteppers();
  if (motors[0].motor.distanceToGo() != 0 || motors[1].motor.distanceToGo() != 0) return;
  for (byte i = 0; i < 3; i++) {
    sprintf(buf, "%d:", sensors[i].type); strcat(combined, buf);
    dtostrf(sensors[i].temp, 3, 1, buf); strcat(combined, buf); strcat(combined, ":");
    dtostrf(sensors[i].hum, 3, 1, buf); strcat(combined, buf); strcat(combined, ":");
    dtostrf(sensors[i].dew, 3, 1, buf); strcat(combined, buf); strcat(combined, ":");
  }
  sprintf(buf, "%d:%d:%d:%d:", readPWMint(ctxOut.pwms[0]), readPWMint(ctxOut.pwms[1]), readPWMint(ctxOut.pwms[2]), readPWMint(ctxOut.pwms[3]));
  strcat(combined, buf);
  sprintf(buf, "%d:%d:%d:%d:", ctxOut.rels[0], ctxOut.rels[1], ctxOut.rels[2], ctxOut.rels[3]);
  strcat(combined, buf);
  dtostrf(ctx.VinCoeff * calcVoltage(averageInt(power.vins, 5)), 3, 1, buf); strcat(combined, buf); strcat(combined, ":");
  dtostrf(ctx.VregCoeff * calcVoltage(averageInt(power.vregs, 5)), 3, 1, buf); strcat(combined, buf); strcat(combined, ":");
  dtostrf(ctx.V5Coeff * calcVoltage(averageInt(power.v5s, 5)), 3, 1, buf); strcat(combined, buf); strcat(combined, ":");
  dtostrf(calcItot(power.Itot), 4, 2, buf); strcat(combined, buf); strcat(combined, ":");
  dtostrf(ctx.VinCoeff * calcPtot(power.Ptot), 4, 2, buf); strcat(combined, buf); strcat(combined, ":");
  sprintf(buf, "%d", ((digitalRead(DCMOTOR_A) == HIGH || digitalRead(DCMOTOR_B) == HIGH) ? 1 : 0)); strcat(combined, buf);
  if(cloudSensor.isThere) {
    strcat(combined, ":");
    dtostrf(cloudSensor.Tamb, 3, 1, buf); strcat(combined, buf); strcat(combined, ":");
    dtostrf(cloudSensor.Tsky, 3, 1, buf); strcat(combined, buf);
  }
}
