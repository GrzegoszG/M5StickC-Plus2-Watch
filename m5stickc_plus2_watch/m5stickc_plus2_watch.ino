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

#define MAX_VOLTAGE 4350
#define MIN_VOLTAGE 3000
#define TIMEZONE 1

#define TOO_MANY_SECONDS 86400 //24h

#define DEFAULT_DELAY 100
#define DEFAULT_DRAW_DELAY 2000

#define MODE_CLOCK 0
#define MODE_FLASHLIGHT 1

float normalized_max = (MAX_VOLTAGE - MIN_VOLTAGE)*1.0f;
int clockColor;
int batteryPercent;
int batteryVoltage;
int nextFrameDelayMilis;
int drawDelay;

int modes[] = 
{
  MODE_CLOCK,
  MODE_FLASHLIGHT
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

int modeChangeSeconds;
int modeChange;

int mode;
int modeIndex;

void setup() 
{
    drawDelay = DEFAULT_DRAW_DELAY;
    nextFrameDelayMilis = DEFAULT_DELAY;
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
}

void getDevicesData()
{
  StickCP2.update();
  batteryVoltage = StickCP2.Power.getBatteryVoltage();
  int normalized_vol = batteryVoltage - MIN_VOLTAGE;
  batteryPercent = (int)((normalized_vol * 1.0f) / (normalized_max) * 100.0f);
  auto dt = StickCP2.Rtc.getDateTime();
  curTime.hour = ((dt.time.hours)+TIMEZONE)%24;
  curTime.minute = dt.time.minutes;
  curTime.second = dt.time.seconds;
  curTime.day = dt.date.date;
  curTime.month = dt.date.month;

  if (StickCP2.BtnA.wasPressed()) 
  {
    setModeToNext();
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

void setMode()
{
  if (modeIndex >= sizeof(modes)/sizeof(modes[0]))
  {
    modeIndex = 0;
  }
  if (mode != modes[modeIndex])
  {
    switch (modes[modeIndex])
    {
      case MODE_CLOCK:
        drawDelay = DEFAULT_DRAW_DELAY;
        StickCP2.Display.setBrightness(100);
        break;
      case MODE_FLASHLIGHT:
        drawDelay = 5000;
        StickCP2.Display.setBrightness(255);
        break;
    }
  }
  mode = modes[modeIndex];
}

void displayMode()
{
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

  int milisFromDisplay = GetSecsDiff(curTime, drawTime) * 1000;
  if (mode != oldMode || (milisFromDisplay > 0 && milisFromDisplay >= drawDelay))
  {
    displayMode();
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

    StickCP2.Display.setTextSize(2.0f);

    StickCP2.Display.setCursor(50, 90);
    
    StickCP2.Display.printf("%02d:%02d", curTime.hour, curTime.minute);
    StickCP2.Display.setTextSize(0.8f);

    StickCP2.Display.setCursor(130, 20);
    StickCP2.Display.printf("%dmv", batteryVoltage);

    StickCP2.Display.setCursor(130, 50);
    StickCP2.Display.printf("%02d/%02d",curTime. day, curTime.month);
}
