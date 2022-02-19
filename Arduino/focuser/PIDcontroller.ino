#define T_LIMIT         3.0                 // delta for full PWM=1 action
#define AO_PEL_OFFSET   10
#define sign(x) ((x>0.0)-(x<0.0))           // adapted from old Utility.h library

int getPIDdrive(float TNow, float TSet) {

  float TErr = TSet - TNow;
  float Drive;

  static float Integral, TLast;
  if (ctx.Kdir != 0) TErr = -TErr;
  Integral = (abs(TErr) < T_LIMIT) ? Integral += TErr : Integral = 0.0;

  Drive = ctx.Kscale * (ctx.Kp * TErr + ctx.Ki * Integral + ctx.Kd * (TLast - TNow)) + AO_PEL_OFFSET;
  TLast = TNow;
  return constrain((int) Drive, 0, 100);
}

