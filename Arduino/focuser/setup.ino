void setup()
{
  prepareConfig();
  loadConfig();
  initializeSerial();
  initializeBuzzer();
  initializeSensors();
  initializeMLX();
  initializeButtons();
  initializeStepper();
  initializeDCmotor();
  adjustEEPROMFocOffset();
  initializeExt();
  initializeGPS();
  initializeLCD();

  buzz(50, 2);
  initializeTimer();
  StartTime = millis();
}

void prepareConfig() {
  lastDCmotorPWM = 255;
  
  ctx.steppers[0] = {200, 100, 0, 800, 10000, 0, -1, 100, 0, 5.0, 1};
  ctx.steppers[1] = {200, 100, 0, 800, 10000, 0, -1, 100, 0, 5.0, 1};
  ctx.buzzer = 1;
  ctx.manualStepperControl = 0;
  ctx.pwmHumSensor = 0;
  ctx.coolTempSensor = 0;
  ctx.coolTempPreset = 20;
  ctx.coolHumSensor = -1;
  ctx.coolHumTreshold = 90;
  ctx.DCmotorReversed = 0;
  ctx.VinCoeff = 3.2;
  ctx.VregCoeff = 3.2;
  ctx.V5Coeff = 3.2;
  ctx.pwm14 = 0x03;
  ctx.pwm23 = 0x01;
  ctx.pwmDC = 0x01;
  ctx.pwmSteppers = 0x01;
  ctx.Kp = 0.35;
  ctx.Ki = 0.001;
  ctx.Kd = 0.01;
  ctx.Kscale = 100.0;
  ctx.Kdir = 1;
  ctx.ToffsetSensor = -1;
  ctx.focuserEEPROMoffset = 0;
  ctx.pwmHumStart = 50;
  ctx.pwmHumFull = 95;
  ctx.IzeroCalibration = (ASC_NEGATIVE) ? 5*512 : 5*509;
  ctx.lcdConfig.enabled = false;
  ctx.gpsConfig.enabled = false;   
  strcpy(ctx.version_of_program, CONFIG_VERSION);

  motors[0].motor = stepper1;
  motors[0].positionSaved = true;
  motors[0].pwmPin = STEPPER1_PWM_PIN;
  motors[0].focuserPosStart = FOCUSER1_POS_START;

  motors[1].motor = stepper2;
  motors[1].positionSaved = true;
  motors[1].pwmPin = STEPPER2_PWM_PIN;
  motors[1].focuserPosStart = FOCUSER2_POS_START;

  compensations[0] = {0.0, 0.0, -1};
  compensations[1] = {0.0, 0.0, -1};

  power.Vin = power.Vreg = power.V5 = power.I = power.Itot = power.Ptot = power.Izero = power.curIndex = 0.0;
  for (byte i = 0; i < 5; i++) {
    power.vins[i] = 0;
    power.vregs[i] = 0;
    power.v5s[i] = 0;
    power.currents[i] = 0;
  }
}


