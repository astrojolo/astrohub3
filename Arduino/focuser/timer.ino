void initializeTimer() {
  timer.every(TIMER_TICK_CYCLE, timerTick);
}

void timerTick() {
  unsigned long now = millis();
  if(timers.requestTemp > 0 && timers.requestTemp < now) {timers.requestTemp = 0; requestTemp();}
  if(timers.readTemp > 0 && timers.readTemp < now) {timers.readTemp = 0; readTemp();}
  if(timers.compCheck1 > 0 && timers.compCheck1 < now) {timers.compCheck1 = 0; compCheck1();}
  if(timers.compCheck2 > 0 && timers.compCheck2 < now) {timers.compCheck2 = 0; compCheck2();}
  if(timers.updatePtot > 0 && timers.updatePtot < now) {timers.updatePtot = 0; updatePtot();}
  if(timers.unlockBuzz > 0 && timers.unlockBuzz < now) {timers.unlockBuzz = 0; unlockBuzz();}
  if(timers.stopDCmotor > 0 && timers.stopDCmotor < now) {timers.stopDCmotor = 0; stopDCmotor();}
  if(timers.stopDCmotorAfterMove > 0 && timers.stopDCmotorAfterMove < now) {timers.stopDCmotorAfterMove = 0; stopDCmotorAfterMove();}
  if(timers.readCloudSensor > 0 && timers.readCloudSensor < now) {timers.readCloudSensor = 0; readCloudSensor();}
  if(timers.updateLcd > 0 && timers.updateLcd < now) {timers.updateLcd = 0; updateLcd();}  
}

