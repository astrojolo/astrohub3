void initializeMLX() {
  therm.begin(MLX_ADDR);
  therm.setUnit(TEMP_C);
  cloudSensor.isThere = therm.readID();
  if(cloudSensor.isThere) {
    timers.readCloudSensor = millis() + 1000;
  }
}

void readCloudSensor() {
  timers.readCloudSensor = millis() + 1000;
  if (motors[0].motor.distanceToGo() != 0 || motors[1].motor.distanceToGo() != 0) return;
    
  if(therm.read()) {
    cloudSensor.Tsky = therm.object();
    cloudSensor.Tamb = therm.ambient();
  }
}


