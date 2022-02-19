struct Params resolveParams(char *param) {
  Params params;
  params.index = param[0] - '0';
  char buf[8]; strncpy(buf, param+2, 8);
  params.value = atol(buf);
  return params;
}

float averageInt(int args[], byte counter) {
  long sum = 0L;
  for(byte i = 0; i < counter; i++) { sum += args[i];}
  return ((float) sum) / ((float) counter);
}

int readVoltage(byte pin) {return analogRead(pin);}
int readCurrent(byte pin) {return analogRead(pin);}

float calcVoltage(float value) {return (5.0/1024.0 * value);}
float calcCurrent(int args[], byte counter) {
  int current = 0;
  for(byte i = 0; i < counter; i++) {current += args[i];}
  current = ((ASC_NEGATIVE) ? power.Izero - current : current - power.Izero);
  return (float)(10.0 * current / 1024.0);
}

float calcItot(long value) {return value / 368640.0;}
float calcPtot(float value) {return value / 75497472.0;}

