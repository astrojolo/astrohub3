void initializeSensors() {
  bool anySensor = false;
  for (byte i = 0; i < 3; i++) {
    sensors[i].type = sensors[i].temp = sensors[i].hum = sensors[i].dew  = 0;
  }
  for (byte i = 0; i < 2; i++) {
    int chk = DHT.read22(dhtPins[i]);
    if (chk == DHTLIB_OK) {
      sensors[i].type = SENSOR_DHT;
      anySensor = true;
    }
  }

  dsSensor.begin();
  bool sensorConnected = dsSensor.getAddress(dsThermometer, 0);
  if (sensorConnected) {
    dsSensor.setResolution(dsThermometer, 10);
    dsSensor.setWaitForConversion(false);
    sensors[2].type = SENSOR_DS;  // only 3rd sensor can be DS1820
    anySensor = true;
  }

  if (anySensor)  {
    timers.requestTemp = millis() + 500;
    timers.compCheck1 = millis() + 1500;
    timers.compCheck2 = millis() + 2000;
  }
}

void requestTemp() {
  timers.readTemp = millis() + 200;
  if (motors[0].motor.distanceToGo() != 0 || motors[1].motor.distanceToGo() != 0) {
    return;
  }
  if (sensors[2].type == SENSOR_DS) {
    dsSensor.requestTemperaturesByAddress(dsThermometer);
  }
}

void readTemp() {
  timers.requestTemp = millis() + TEMP_CYCLE;
  if (motors[0].motor.distanceToGo() != 0 || motors[1].motor.distanceToGo() != 0) return;
  for (byte i = 0; i < 3; i++) {
    sensors[i].temp = sensors[i].hum = sensors[i].dew = 0.0;
    if (sensors[i].type == SENSOR_DHT) {
      int chk = DHT.read22(dhtPins[i]);
      if (chk == DHTLIB_OK) {
        sensors[i].temp = DHT.temperature;
        sensors[i].hum = DHT.humidity;
        sensors[i].dew = dewPointFast(sensors[i].temp, sensors[i].hum);
      }
    } else if (sensors[i].type == SENSOR_DS) {
      sensors[i].temp = dsSensor.getTempC(dsThermometer);
    }
  }
  if(cloudSensor.isThere) readCloudSensor();
  updatePWM();
}

void calculateHeaterPWM() {
  ctxOut.heaterPWM = map(constrain(sensors[ctx.pwmHumSensor].hum, ctx.pwmHumStart, ctx.pwmHumFull), ctx.pwmHumStart, ctx.pwmHumFull, 0, 100);
}

void calculateCoolerPWM() {
  if (sensors[ctx.coolTempSensor].type == 0) return;
  float Tset = (ctx.ToffsetSensor > -1) ? (float) (ctx.coolTempPreset + sensors[ctx.ToffsetSensor].temp) : (float) ctx.coolTempPreset;
  if (ctx.coolHumSensor > -1 && sensors[ctx.coolHumSensor].hum > ctx.coolHumTreshold) {
    ctxOut.coolerPWM = 0;
  } else {
    ctxOut.coolerPWM = getPIDdrive((float) sensors[ctx.coolTempSensor].temp, Tset);
  }
}

void compCheck1() {
  tempCompCheck(0);
}
void compCheck2() {
  tempCompCheck(1);
}

void tempCompCheck(byte index) {
  (index == 0) ? timers.compCheck1 = millis() + ctx.steppers[index].compCycle * 1000 : timers.compCheck2 = millis() + ctx.steppers[index].compCycle * 1000;
  
  if (ctx.steppers[index].compSteps != 0.0 && ctx.steppers[index].compSensor > -1 && motors[index].motor.distanceToGo() == 0)
  {
    if (compensations[index].pos == -1) {
      compensations[index].pos = motors[index].motor.currentPosition();
      compensations[index].refTemp = sensors[ctx.steppers[index].compSensor].temp;
      compensations[index].lastTemp = compensations[index].refTemp;
    }
    else if (abs(compensations[index].lastTemp - sensors[ctx.steppers[index].compSensor].temp) > COMP_DELTA_T) {
      long newPos = compensations[index].pos + (sensors[ctx.steppers[index].compSensor].temp - compensations[index].refTemp) * ctx.steppers[index].compSteps;

      motors[index].motor.setAcceleration(ctx.steppers[index].acc);
      analogWrite(motors[index].pwmPin, (255 * ctx.steppers[index].pwmRun / 100));
      moveStepper(newPos, index);
      compensations[index].lastTemp = sensors[ctx.steppers[index].compSensor].temp;
    }
  }
}

void resetCompensation(byte index) {
  compensations[index] = {0.0, 0.0, -1};
  (index == 0) ? timers.compCheck1 = millis() + ctx.steppers[index].compCycle * 1000 : timers.compCheck2 = millis() + ctx.steppers[index].compCycle * 1000;
}

// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

// ---- SERIAL CALLBACKS -----------------------------------------------------------

void printTemp(char *answer) {
  for (byte i = 0; i < 3; i++) {
    if (sensors[i].type > 0) {
      char buf[6];
      dtostrf(sensors[i].temp, 3, 1, buf);
      strcat(answer, buf);
      return;
    }
  }
  strcat(answer, "false");
}

void printHum(char *answer) {
  for (byte i = 0; i < 3; i++) {
    if (sensors[i].type == SENSOR_DHT) {
      char buf[6];
      dtostrf(sensors[i].hum, 3, 1, buf);
      strcat(answer, buf);
      return;
    }
  }
  strcat(answer, "false");
}

void printDew(char *answer) {
  for (byte i = 0; i < 3; i++) {
    if (sensors[i].type == SENSOR_DHT) {
      char buf[6];
      dtostrf(sensors[i].dew, 3, 1, buf);
      strcat(answer, buf);
      return;
    }
  }
  strcat(answer, "false");
}
