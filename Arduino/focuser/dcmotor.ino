void initializeDCmotor() {
  pinMode(DCMOTOR_A, OUTPUT);
  pinMode(DCMOTOR_B, OUTPUT);
  pinMode(DCMOTOR_PWM_PIN, OUTPUT);
  stopDCmotor();
}

void stopDCmotorAfterMove() {
  if(isDCmotorStopped()) return;
  stopDCmotor();
  buzz(20, 1);
}

bool isDCmotorStopped() {
  return (digitalRead(DCMOTOR_A)) == LOW && (digitalRead(DCMOTOR_B) == LOW);
}

void stopDCmotor() {
  analogWrite(DCMOTOR_PWM_PIN, 0);  
  digitalWrite(DCMOTOR_A, LOW);
  digitalWrite(DCMOTOR_B, LOW);
}

void moveDCmotor(byte pwm, int period, byte dir, bool manual) {
  stopDCmotor();
  if(ctx.DCmotorReversed != 0) dir = !dir;
  if(dir == 0) {digitalWrite(DCMOTOR_A, HIGH);} else {digitalWrite(DCMOTOR_B, HIGH);}
  if(manual) 
  {
    analogWrite(DCMOTOR_PWM_PIN, map(pwm, 0, 100, 0, 255));
    timers.stopDCmotor = millis() + period;
  }
  else
  {
    analogWrite(DCMOTOR_PWM_PIN, map(pwm, 0, 100, 0, 255));
    lastDCmotorPWM = pwm;
    timers.stopDCmotorAfterMove = millis() + period;
  }
}

// ---- SERIAL CALLBACKS -----------------------------------------------------------
void runDCmotor(char *param) {
  char *token;
  int values[3]; byte i = 0;
  token = strtok(param, ":");
  while (token != NULL) {
    values[i] = atoi(token);
    i++;
    token = strtok(NULL, ":");
  }
  if (i > 1) moveDCmotor(values[1], values[2], values[0], false);
}
