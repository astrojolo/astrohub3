void initializeButtons() {
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
}

void doButtonsCheck() {
  if ( aButton.update() ) {
    if ( aButton.read() == LOW) {
      moveStepperManual(ctx.steppers[ctx.manualStepperControl].maxPos);
    }
    else
    {
      stopStepperManual();
    }
  }

  if ( bButton.update() ) {
    if ( bButton.read() == LOW) {
      moveStepperManual(0);
    }
    else
    {
      stopStepperManual();
    }
  }
}

void stopStepperManual() {
  if (ctx.manualStepperControl == 2) {
    stopDCmotor();
  } else {
    silent = true;
    motors[ctx.manualStepperControl].motor.setAcceleration(ctx.steppers[ctx.manualStepperControl].acc * 3);
    motors[ctx.manualStepperControl].motor.stop();
  }
  timers.unlockBuzz = millis() + 1000;
}

void moveStepperManual(long newPos) {
  if (ctx.manualStepperControl == 2) {
    moveDCmotor(lastDCmotorPWM, 10000, (newPos == 0) ? 0 : 1, true);
  } else {
    motors[ctx.manualStepperControl].motor.setAcceleration((int) (ctx.steppers[ctx.manualStepperControl].acc / 5));
    analogWrite(motors[ctx.manualStepperControl].pwmPin, (255 * ctx.steppers[ctx.manualStepperControl].pwmRun / 100));
    moveStepper(newPos, ctx.manualStepperControl);
    motors[ctx.manualStepperControl].positionSaved = false;
  }
}

