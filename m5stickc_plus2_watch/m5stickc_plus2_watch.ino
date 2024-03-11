/**
 * @file battery.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief M5StickCPlus2 get battery voltage
 * @version 0.1
 * @date 2023-12-12
 *
 *
 * @Hardwares: M5StickCPlus2
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5StickCPlus2: https://github.com/m5stack/M5StickCPlus2
 */

#include "M5StickCPlus2.h"

// BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define MAX_VOLTAGE 4350
#define MIN_VOLTAGE 3000
#define TIMEZONE 1

#define TOO_MANY_SECONDS 86400 //24h

#define DEFAULT_DELAY 150
#define DEFAULT_DRAW_DELAY 2000
#define ALARM_BUZZ_INTERVAL 1000;
#define BTNB_HOLD_SWITCH_DELAY 1001;

#define MODE_CLOCK 0
#define MODE_FLASHLIGHT 1
#define MODE_ALARM 2
//#define MODE_DICE 3

#define MODE_CLOCK_DRAW_DELAY 3000
#define MODE_FLASHLIGHT_DRAW_DELAY 10000
#define MODE_ALARM_DRAW_DELAY 1000
#define DEFAULT_BRIGHTNESS 10
#define IDLE_BRIGHTNESS 1
#define IDLE_MILIS 5000
#define CLOCK_FREQUENCY 80
#define GET_BAT_ITERATIONS 5

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // random uuid

#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b2137" //random uuid

unsigned int diceType;
unsigned int diceCount;
unsigned int * diceResult;
unsigned int diceSum;

float normalized_max = (MAX_VOLTAGE - MIN_VOLTAGE)*1.0f;
int clockColor;
int batteryPercent;
int batteryVoltage;
int nextFrameDelayMilis;
int drawDelay;

int modes[] = 
{
  MODE_CLOCK,
  MODE_FLASHLIGHT,
  MODE_ALARM
};

struct Time
{
  unsigned int month;
  unsigned int day;
  unsigned int hour;
  unsigned int minute;
  unsigned int second;
};

Time startTime = {1,1,0,0,0};

Time curTime = {1,1,0,0,0};

Time modeChangeStart = {1,1,0,0,0};

Time modeChangedTime = {1,1,0,0,0};

Time drawTime = {1,1,0,0,0};

Time alarmTime = {1,1,0,0,0};

Time holdSwitchTime = {1,1,0,0,0};

Time lastClickTime = {1,1,0,0,0};

int modeChangeSeconds;
int modeChange;

int mode;
int modeIndex;
bool alarmEnable;
bool alarmExecute;
int alarmSetType;
bool alarmKilled;
int alarmLastBeepSecond;
bool isNextHour;
bool isNextMinute;
int modeCount;
int brightness;
int prevBrightness;
bool idle;
bool redraw;
int getBatCounter;
// M5.BtnPWR.

void setup() 
{
    getBatCounter = GET_BAT_ITERATIONS;
    isNextHour = false;
    isNextMinute = false;
    alarmLastBeepSecond = 0;
    alarmKilled = false;
    alarmSetType = 0;
    drawDelay = DEFAULT_DRAW_DELAY;
    nextFrameDelayMilis = DEFAULT_DELAY;
    brightness = DEFAULT_BRIGHTNESS;
    modeIndex = 0;
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(3);
    clockColor = RED;
    StickCP2.Display.setTextColor(clockColor);
    StickCP2.Display.setTextDatum(middle_center);
    StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
    StickCP2.Display.setTextSize(1);
    auto st = StickCP2.Rtc.getDateTime();
    startTime.month = st.date.month;
    startTime.day = st.date.date;
    startTime.hour = ((st.time.hours)+TIMEZONE)%24;
    startTime.minute = st.time.minutes;
    startTime.second = st.time.seconds;

    alarmTime.month = startTime.month;
    alarmTime.day = startTime.day;
    alarmTime.hour = 0;
    alarmTime.minute = 0;
    alarmTime.second = 0;

    holdSwitchTime.hour = startTime.hour;
    holdSwitchTime.minute = startTime.minute;
    holdSwitchTime.second = startTime.second;
    holdSwitchTime.day = startTime.day;
    holdSwitchTime.month = startTime.month;

    lastClickTime.hour = startTime.hour;
    lastClickTime.minute = startTime.minute;
    lastClickTime.second = startTime.second;
    lastClickTime.day = startTime.day;
    lastClickTime.month = startTime.month;
    modeCount = sizeof(modes)/sizeof(modes[0]);
    setCpuFrequencyMhz(CLOCK_FREQUENCY);
    //initBT();
}

void initBT()
{
  BLEDevice::init("M5Stick C PLUS 2");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
  CHARACTERISTIC_UUID,
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_WRITE );
  pCharacteristic->setValue("Hello World");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising(); // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void getDevicesData()
{
  StickCP2.update();

  --getBatCounter;
  if (getBatCounter <= 0)
  {
    getBatCounter = GET_BAT_ITERATIONS;
    batteryVoltage = StickCP2.Power.getBatteryVoltage();
    int normalized_vol = batteryVoltage - MIN_VOLTAGE;
    int per = (int)((normalized_vol * 1.0f) / (normalized_max) * 100.0f);
    int diff = per - batteryPercent;
    batteryPercent = per;
    if (diff > 1 || diff < -1)
    {
      redraw = true;
    }
  }

  auto dt = StickCP2.Rtc.getDateTime();

  unsigned int curHour = ((dt.time.hours)+TIMEZONE)%24;
  isNextHour = false;
  if (curHour != curTime.hour)
  {
    isNextHour = true;
    redraw = true;
  }
  curTime.hour = curHour;

  isNextMinute = false;
  if (dt.time.minutes != curTime.minute)
  {
    isNextMinute = true;
    redraw = true;
    if (alarmKilled)
    {
      alarmKilled = false;
    }
  }
  curTime.minute = dt.time.minutes;
  curTime.second = dt.time.seconds;
  curTime.day = dt.date.date;
  curTime.month = dt.date.month;

  bool btnBHold = StickCP2.BtnB.wasHold();
  bool btnBPressed = StickCP2.BtnB.wasPressed();
  bool btnBReleased = StickCP2.BtnB.wasReleased();
  bool btnAPressed = StickCP2.BtnA.wasPressed();
  if (btnBHold || btnBPressed || btnBReleased || btnAPressed)
  {
    idle = false;
    brightness = DEFAULT_BRIGHTNESS;
    redraw = true;
    lastClickTime.hour = curTime.hour;
    lastClickTime.minute = curTime.minute;
    lastClickTime.second = curTime.second;
    lastClickTime.day = curTime.day;
    lastClickTime.month = curTime.month;
  }
  if (brightness > IDLE_BRIGHTNESS)
  {
    int btnDiffMilis = GetSecsDiff(curTime, lastClickTime) * 1000;
    if (btnDiffMilis > IDLE_MILIS)
    {
      idle = true;
      brightness = IDLE_BRIGHTNESS;
    }
  }

  if (btnBHold)
  {
    holdSwitchTime.hour = curTime.hour;
    holdSwitchTime.minute = curTime.minute;
    holdSwitchTime.second = curTime.second;
    holdSwitchTime.day = curTime.day;
    holdSwitchTime.month = curTime.month;
  }

  if (btnAPressed) 
  {
    if (alarmExecute)
    {
      alarmExecute = false;
      alarmKilled = true;
    }
    setModeToNext();
  }
  if (mode == MODE_ALARM)
  {
    if (btnBHold)
    {
        alarmSetType = (alarmSetType + 1) % 3;
        if (alarmSetType == 2)
        {
          alarmEnable = !alarmEnable;
        }
    }
    else if (btnBReleased)
    {
      int milisDiff = GetSecsDiff(curTime, holdSwitchTime) * 1000;
      int minDiff = BTNB_HOLD_SWITCH_DELAY;
      if (milisDiff >= minDiff)
      {
        if (alarmSetType == 0)
        {
          alarmTime.hour = ((alarmTime.hour + 1) % 24);
        }
        else if (alarmSetType == 1)
        {
          alarmTime.minute = ((alarmTime.minute + 1) % 60);
        }
      }
    }
  }
}

void setModeToNext()
{
  modeIndex++;
  setMode();
}

void resetMode()
{
  modeIndex = 0;
  setMode();
}

void beepAlarm()
{
  if (alarmEnable != true)
  {
    return;
  }
  if (!alarmExecute)
  {
    if (alarmTime.hour == curTime.hour && alarmTime.minute == curTime.minute)
    {
      alarmExecute = true;
    }
  }
  if (alarmExecute && !alarmKilled)
  {
    if (curTime.second % 2 == 0 && alarmLastBeepSecond < curTime.second)
    {
      alarmLastBeepSecond = curTime.second;
      StickCP2.Speaker.tone(8000, 40);
    }
    if (alarmTime.hour != curTime.hour || alarmTime.minute != curTime.minute)
    {
      alarmExecute = false;
    }
  }
}

void setMode()
{
  if (modeIndex >= modeCount)
  {
    modeIndex = 0;
  }
  if (mode != modes[modeIndex])
  {
    switch (modes[modeIndex])
    {
      brightness = DEFAULT_BRIGHTNESS;
      case MODE_CLOCK:
        drawDelay = MODE_CLOCK_DRAW_DELAY;
        break;
      case MODE_FLASHLIGHT:
        drawDelay = MODE_FLASHLIGHT_DRAW_DELAY;
        brightness = 255;
        break;
      case MODE_ALARM:
        drawDelay = MODE_ALARM_DRAW_DELAY;
        break;
      default:
        drawDelay = DEFAULT_DRAW_DELAY;
        break;
    }
  }
  mode = modes[modeIndex];
}

void displayAlarm()
{
    StickCP2.Display.clear();
    
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.setCursor(20, 20);
    StickCP2.Display.printf("ALARM");

    if (alarmEnable)
    {
      StickCP2.Display.setCursor(130, 20);
      StickCP2.Display.printf("ON");
    }
    else
    {
      StickCP2.Display.setCursor(130, 20);
      StickCP2.Display.printf("OFF");
    }

    StickCP2.Display.setTextSize(2.0f);
    StickCP2.Display.setCursor(20, 60);
    StickCP2.Display.printf("%02d:%02d", alarmTime.hour, alarmTime.minute);

    StickCP2.Display.setCursor(20, 80);
    if (alarmSetType == 0)
    {
      StickCP2.Display.printf("---");
    }
    else if(alarmSetType == 1)
    {
      StickCP2.Display.printf("     ---");
    }
}

void displayMode()
{
  if (prevBrightness != brightness)
  {
    StickCP2.Display.setBrightness(brightness);
  }
  prevBrightness = brightness;

  drawTime.month = curTime.month;
  drawTime.day = curTime.day;
  drawTime.hour = curTime.hour;
  drawTime.minute = curTime.minute;
  drawTime.second = curTime.second;
  switch (mode)
  {
    case MODE_CLOCK:
      displayClockView(clockColor);
      break;
    case MODE_FLASHLIGHT:
      displayFlashlight(1);
      break;
    case MODE_ALARM:
      displayAlarm();
      break;
    default:
      resetMode();
      displayClockView(clockColor);
      break;
  }
}

void loop() 
{
  int oldMode = mode;
  getDevicesData();
  beepAlarm();

  if (redraw)
  {
    displayMode();
    redraw = false;
  }
  
  waitTillNextFrame();
}

int GetSecsDiff(Time t1, Time t2)
{
  if (t1.month != t2.month)
    return TOO_MANY_SECONDS;
  if (t1.day != t2.day)
    return TOO_MANY_SECONDS;
  int totalSeconds1 = (t1.hour * 3600) + (t1.minute * 60) + t1.second;
  int totalSeconds2 = (t2.hour * 3600) + (t2.minute * 60) + t2.second;
  int totalSecsDiff = totalSeconds1 - totalSeconds2;
  return totalSecsDiff;
}

void waitTillNextFrame()
{
  if (nextFrameDelayMilis > 0)
  {
    delay(nextFrameDelayMilis);
  }
}

void displayFlashlight(int milis)
{
  
  StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), WHITE);
}

void displayClockView(int color)
{
  if (color != clockColor)
  {
    StickCP2.Display.setTextColor(color);
    clockColor = color;
  }

    StickCP2.Display.clear();
    
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.setCursor(20, 20);
    StickCP2.Display.printf("%d %%", batteryPercent);

    if (idle)
    {
      StickCP2.Display.setCursor(20, 50);
      StickCP2.Display.printf("IDLE");
    }

    StickCP2.Display.setTextSize(2.0f);

    StickCP2.Display.setCursor(50, 90);
    
    StickCP2.Display.printf("%02d:%02d", curTime.hour, curTime.minute);
    StickCP2.Display.setTextSize(0.8f);

    StickCP2.Display.setCursor(130, 20);
    StickCP2.Display.printf("%d", brightness);

    StickCP2.Display.setCursor(130, 50);
    StickCP2.Display.printf("%02d/%02d",curTime. day, curTime.month);
}

void setAlarm(int hour, int minute)
{
  alarmTime.hour = hour;
  alarmTime.minute = minute;
  alarmEnable = true;
}

void roll()
{
  diceResult = new unsigned int(diceCount);
  diceSum = 0;
  for (unsigned int i=0; i<diceCount; i++)
  {
    diceResult[i] = (rand() % diceType) + 1;
    diceSum += diceResult[i];
  }
}

