void initializeStepper() {
  for (byte i = 0; i < 2; i++) {
    writeCtxToStepper(i);
    motors[i].positionSaved = true;
    analogWrite(motors[i].pwmPin, (255 * ctx.steppers[i].pwmStop/100));
  }
}

void writeCtxToStepper(byte index) {
    motors[index].motor.setMaxSpeed(ctx.steppers[index].stepperSpeed);
    motors[index].motor.setAcceleration(ctx.steppers[index].acc);
    long curPos = readFocuserPos(index);
    if (curPos < 0) {curPos = 0; saveFocuserPos(curPos, index);}
    motors[index].motor.setCurrentPosition(curPos);
    motors[index].motor.setMode(ctx.steppers[index].stepperMode);
    motors[index].motor.setReversed(ctx.steppers[index].reversed);  
}

void checkStepper(byte index) {
  if(motors[index].motor.distanceToGo() == 0 && !motors[index].positionSaved) {
    saveFocuserPos(motors[index].motor.currentPosition(), index);
    motors[index].positionSaved = true;
    buzz(20, 1);
    analogWrite(motors[index].pwmPin, (255 * ctx.steppers[index].pwmStop/100));
    if(resetComp) {
      resetCompensation(index);
      resetComp = false;
    }
  }
}

void moveStepper(long newPos, byte index) {
  if(newPos != motors[index].motor.currentPosition()) {
    if(newPos < 0 || newPos > ctx.steppers[index].maxPos) {
      buzz(100, 2);
    }
    else
    {
      motors[index].motor.setAcceleration(ctx.steppers[index].acc);
      motors[index].motor.moveTo(newPos);
      motors[index].positionSaved = false;
    }
  }
}

void runSteppers() {
  motors[0].motor.run();
  motors[1].motor.run();
}

bool areSteppersMoving() {
  return (motors[0].motor.distanceToGo() != 0) || (motors[1].motor.distanceToGo() != 0);
}

// ---- SERIAL CALLBACKS -----------------------------------------------------------
void setTempComp(char *param) { //expect temp cycle : steps/C*100 : comp sensor
  char *token;
  int values[6]; byte i = 0;
  token = strtok(param, ":");
  while (token != NULL) {
    values[i] = atoi(token);
    i++;
    token = strtok(NULL, ":");
  }
  if(i > 4) {
    ctx.steppers[0].compCycle = values[0];
    ctx.steppers[0].compSteps = ((float) values[1])/100.0;
    ctx.steppers[0].compSensor = values[2];
    ctx.steppers[1].compCycle = values[3];
    ctx.steppers[1].compSteps = ((float) values[4])/100.0;
    ctx.steppers[1].compSensor = values[5];
    saveConfig();
    resetCompensation(0); resetCompensation(1);
  }
}

void printTempComp(char *answer) {
  char buf[6];
  sprintf(buf, "%d", ctx.steppers[0].compCycle); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", (int) (100*ctx.steppers[0].compSteps)); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.steppers[0].compSensor); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.steppers[1].compCycle); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", (int) (100*ctx.steppers[1].compSteps)); strcat(answer, buf); strcat(answer, ":");
  sprintf(buf, "%d", ctx.steppers[1].compSensor); strcat(answer, buf); 
}

void runStepper(char *param) {
  Params params = resolveParams(param);
  motors[params.index].motor.setAcceleration(ctx.steppers[params.index].acc);
  analogWrite(motors[params.index].pwmPin, (255 * ctx.steppers[params.index].pwmRun / 100));
  resetComp = true;
  moveStepper(params.value, params.index);
}

void setPosition(char *param) {
  Params params = resolveParams(param);
  motors[params.index].motor.setCurrentPosition(params.value);
  motors[params.index].positionSaved = true;
  saveFocuserPos(motors[params.index].motor.currentPosition(), params.index);
}

void setStepperSpeed(char *param) {
  Params params = resolveParams(param);
  ctx.steppers[params.index].stepperSpeed = params.value;
  motors[params.index].motor.setMaxSpeed(ctx.steppers[params.index].stepperSpeed);
}

