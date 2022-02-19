// ASCOM AstroHub arduino sketch
//
// Author: jolo drjolo@gmail.com
//
// 3.0.6 - added delay after updateRelays if current changed over 2A
// 3.0.7 - LCD
// 3.0.8 - GPS
// 3.1.0 - GPS + LCD working
// 3.1.1 - fixed PWM LCD values
// 3.1.2 - current sensor calibration
// 3.1.3 - stepper idle current fix
// 3.1.4 - ADC coefficient fix
#include <OneWire.h>
#include <DallasTemperature.h>
#include <dht.h>
#include <EEPROM.h>
#include <EepromUtil.h>
#include <AccelStepper.h>
#include <Timer.h>
#include <Bounce.h>
#include <Math.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <SparkFunMLX90614.h>
#include <TinyGPS++.h>
#include <TimeLib.h>

#define DEVICE_RESPONSE "Jolo AstroHub"
#define FIRMWARE "3.1.4"

//==== LOW LEVEL ====================================================
#define FOCUSER1_POS_START 100
#define FOCUSER2_POS_START 300
#define CONFIG_VERSION "hw5"
#define CONFIG_START 500
#define NICK_START 900
#define TIMER_TICK_CYCLE 10
#define ASC_NEGATIVE false   // true indicates increasing current will decrease voltage
#define ORIGIN_LAT 50.2343
#define ORIGIN_LNG 18.6031

//==== CONFIG =======================================================
struct StepperCtx {         // stepper config - will be stored in EEPROM and set from ASCOM
  int stepperSpeed; byte pwmRun; byte pwmStop; int acc; long maxPos; 
  byte reversed; int compSensor; long compCycle; float compSteps; float stepSize; 
  byte stepperMode;  // 0 - unipolar, 1 - bipolar, 234 A4988 1/4 1/8 1/16
};
                            // device config - will be stored in EEPROM and set from ASCOM
struct LCDconfig { byte lines[6][5]; bool enabled; bool disableUpdateDuringmove; byte pwm; byte contrast;} lcdConfig = 
                    { {{3,4,8,6,5},{8,9,10,11,5},{5,7,8,13,5},{2,8,13,11,5},{0,0,0,0,0},{0,0,0,0,0}}, true, true, 200, 200 };
struct GPSconfig {byte serial; int bitrate; int UTCoffset; boolean enabled; boolean passGPSdata;} gpsConfig = {1, 9600, 1, false, false};                    
struct {                    
  StepperCtx steppers[2];
  byte buzzer; byte manualStepperControl; byte pwmHumSensor;
  byte coolTempSensor; int coolTempPreset; int coolHumSensor; byte coolHumTreshold;
  byte DCmotorReversed; float VinCoeff; float VregCoeff; float V5Coeff;
  byte pwm14; byte pwm23; byte pwmDC; byte pwmSteppers;
  float Kp; float Ki; float Kscale; float Kd; byte Kdir; int ToffsetSensor; byte pwmHumFull; byte pwmHumStart;
    // This is for mere detection if they are your settings
  int focuserEEPROMoffset;
  byte outPowerON[4];
  int IzeroCalibration;
  LCDconfig lcdConfig;
  GPSconfig gpsConfig;
  char version_of_program[4];
} ctx;

//==== MOTORS ==================================================================
#define DCMOTOR_PWM_PIN 8
#define DCMOTOR_A A4
#define DCMOTOR_B A5
#define STEPPER1_PWM_PIN 46
#define STEPPER2_PWM_PIN 45
// Phase 1,2,3,4 = A B A' B'
AccelStepper stepper1 = AccelStepper(AccelStepper::HALF4WIRE, A7, A9, A6, A8);
AccelStepper stepper2 = AccelStepper(AccelStepper::HALF4WIRE, A11, A13, A10, A12);

struct StepperConfig {  // motor config - not in EEPROM
  byte pwmPin; bool positionSaved; int focuserPosStart; AccelStepper motor;
};
StepperConfig motors[2];

//==== SENSING ==================================================================
#define Vin_PIN A3
#define Vreg_PIN A2
#define Itot_PIN A15
#define V5_PIN A14
#define Ptot_CYCLE 200
#define TEMP_CYCLE 1000      
#define NO_SENSOR 0
#define SENSOR_DHT 1
#define SENSOR_DS 2
#define COMP_DELTA_T 0.5
#define MLX_ADDR 0x5A

byte pwmPins[] = {9,12,11,10};
byte relPins[] = {43,42,41,40};

OneWire dsWire(5);
DallasTemperature dsSensor(&dsWire);
DeviceAddress dsThermometer;

byte dhtPins[] = {7,6};
dht DHT;
bool resetComp = false;
bool readOtherValues = false;

struct Sensor{          // sensors structure - not in EEPROM
  byte type;  float temp;  float hum;  float dew;
};
Sensor sensors[3];

struct {
  int Izero; int Vin; int Vreg; int V5; int I; unsigned long Itot; float Ptot; int currents[5]; byte curIndex; int vins[5]; int vregs[5]; int v5s[5];
} power;

struct {double Tsky; double Tamb; bool isThere;} cloudSensor = {0.0,0.0,false};

struct TempComp {float refTemp; float lastTemp; long pos;};
TempComp compensations[2];
struct Params {byte index; long value;};

IRTherm therm;

struct {
  unsigned long requestTemp, readTemp, compCheck1, compCheck2, updatePtot, unlockBuzz, stopDCmotor, stopDCmotorAfterMove, readCloudSensor, updateLcd, iZero;
} timers = {0,0,0,0,0,0,0,0,0,0,0};

//==== OUTPUTS/OTHER ==================================================================
#define BUTTON_A_PIN A0
#define BUTTON_B_PIN A1
#define BUZZER_PIN 30
#define LCD_BACKLIGHT_PIN 13
#define LCD_CONTRAST_PIN 4
Bounce aButton = Bounce( BUTTON_A_PIN, 30 );
Bounce bButton = Bounce( BUTTON_B_PIN, 30 );

struct {                // outputs (relays, PWM) context - will NOT be stored in EEPROM
  byte pwms[4]; byte rels[4]; byte heaterPWM; byte coolerPWM;
} ctxOut;

LiquidCrystal lcd(23, 24, 25, 26, 27, 28);

Timer timer;
long StartTime;

TinyGPSPlus gps; 
int LCDupdateCounter, LCDcurrentScreenIndex, LCDnextScreenCounter;


// Global vars
char serialInput[100];
bool silent;                         // Do not buzz when using buttons for focusing, its annoying
byte lastDCmotorPWM;

void loop()
{
  runSteppers();
  checkStepper(0);
  checkStepper(1);
  doButtonsCheck();

  timer.update();
}







