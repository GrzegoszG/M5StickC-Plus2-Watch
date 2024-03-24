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
#include <Preferences.h>

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
#define MODE_ROLL 3

#define MODE_CLOCK_DRAW_DELAY 3000
#define MODE_FLASHLIGHT_DRAW_DELAY 10000
#define MODE_ALARM_DRAW_DELAY 1000
#define DEFAULT_BRIGHTNESS 10
#define IDLE_BRIGHTNESS 1
#define IDLE_MILIS 5000
#define CLOCK_FREQUENCY 80
#define GET_BAT_ITERATIONS 5
#define RTC_SEC_OFFSET 5

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // random uuid

#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b2137" //random uuid
#define DICE_TYPES_COUNT 9
#define DICE_COUNT_MAX 6

#define BTN_STATUS_NONE 0
#define BTN_STATUS_HOLD 1
#define BTN_STATUS_CLICKED 2
#define BTN_STATUS_PRESSED 3
#define BTN_STATUS_RELEASED 4

#define BTNA 0
#define BTNB 1
#define BTNPWR 2

unsigned int diceType;
unsigned int diceCount;
int * diceResult;
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
  MODE_ALARM,
  MODE_ROLL
};

int colors[] = 
{
  RED,
  YELLOW,
  GREEN,
  BLUE,
  CYAN,
  MAGENTA,
  WHITE,
  ORANGE
};

int diceTypes[] = 
{
  2, 3, 4, 6, 8, 10, 12, 20, 100
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
int randCounter = 1000;
// M5.BtnPWR.
bool prevDeviceConnected;
bool deviceConnected;

int btnStatus[3] = 
{
  BTN_STATUS_NONE, BTN_STATUS_NONE, BTN_STATUS_NONE
};

int rollMode = 0;

Preferences preferences;


//Setup callbacks onConnect and onDisconnect
class BLECallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class BLECharCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic* pCharacteristic, esp_ble_gatts_cb_param_t* param)
  {

  }

  void onRead(BLECharacteristic* pCharacteristic)
  {

  }

  void onWrite(BLECharacteristic* pCharacteristic, esp_ble_gatts_cb_param_t* param)
  {

  }

  void onWrite(BLECharacteristic* pCharacteristic)
  {

  }

  void onNotify(BLECharacteristic* pCharacteristic)
  {

  }

  void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code)
  {

  }
};

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
    preferences.begin("app", false);
    StickCP2.Display.setRotation(3);
    clockColor = YELLOW;
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

    if (preferences.getBool("useBLE") || true)
    {
      initBT();
    }
}

int getRandom(int from, int to)
{
  int range = to - from;
  int r = randCounter % range;
  return r;
}

void initBT()
{
  BLEDevice::init("M5Stick C PLUS 2");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BLECallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
  CHARACTERISTIC_UUID,
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_WRITE );
  pCharacteristic->setValue("Hello World");
  pCharacteristic->setCallbacks(new BLECharCallbacks());
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
  randCounter++;
  if (randCounter >= 10000)
  {
    randCounter = 1000;
  }
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

  btnStatus[BTNA] = M5.BtnA.wasHold() ? BTN_STATUS_HOLD
                  //: M5.BtnA.wasClicked() ? BTN_STATUS_CLICKED
                  : M5.BtnA.wasPressed() ? BTN_STATUS_PRESSED
                  : M5.BtnA.wasReleased() ? BTN_STATUS_RELEASED
                  : BTN_STATUS_NONE;
  btnStatus[BTNB] = M5.BtnB.wasHold() ? BTN_STATUS_HOLD
                  //: M5.BtnB.wasClicked() ? BTN_STATUS_CLICKED
                  : M5.BtnB.wasPressed() ? BTN_STATUS_PRESSED
                  : M5.BtnB.wasReleased() ? BTN_STATUS_RELEASED
                  : BTN_STATUS_NONE;
  btnStatus[BTNPWR] = 
                    M5.BtnPWR.wasHold() ? BTN_STATUS_HOLD
                  //: M5.BtnPWR.wasClicked() ? BTN_STATUS_CLICKED
                  : M5.BtnPWR.wasPressed() ? BTN_STATUS_PRESSED
                  : M5.BtnPWR.wasReleased() ? BTN_STATUS_RELEASED
                  : BTN_STATUS_NONE;

  if (btnStatus[BTNA] != BTN_STATUS_NONE ||
      btnStatus[BTNB] != BTN_STATUS_NONE ||
      btnStatus[BTNPWR] != BTN_STATUS_NONE)
  {
    idle = false;
    brightness = DEFAULT_BRIGHTNESS;
    redraw = true;
    lastClickTime.hour = curTime.hour;
    lastClickTime.minute = curTime.minute;
    lastClickTime.second = curTime.second;
    lastClickTime.day = curTime.day;
    lastClickTime.month = curTime.month;

    if (alarmExecute)
    {
      alarmExecute = false;
      alarmKilled = true;
    }
  }
  if (brightness > IDLE_BRIGHTNESS)
  {
    int btnDiffMilis = GetSecsDiff(curTime, lastClickTime) * 1000;
    if (btnDiffMilis > IDLE_MILIS && mode != MODE_FLASHLIGHT)
    {
      idle = true;
      brightness = IDLE_BRIGHTNESS;
    }
  }

  if (btnStatus[BTNB] == BTN_STATUS_HOLD)
  {
    holdSwitchTime.hour = curTime.hour;
    holdSwitchTime.minute = curTime.minute;
    holdSwitchTime.second = curTime.second;
    holdSwitchTime.day = curTime.day;
    holdSwitchTime.month = curTime.month;
  }

  if (btnStatus[BTNA] == BTN_STATUS_HOLD || (mode != MODE_ROLL && btnStatus[BTNA] == BTN_STATUS_PRESSED)) 
  {
    setModeToNext();
  }
  if (mode == MODE_ALARM)
  {
    if (btnStatus[BTNB] == BTN_STATUS_HOLD)
    {
        alarmSetType = (alarmSetType + 1) % 3;
        if (alarmSetType == 2)
        {
          alarmEnable = !alarmEnable;
        }
    }
    else if (btnStatus[BTNB] == BTN_STATUS_RELEASED)
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
  else if (mode == MODE_ROLL)
  {
    if (btnStatus[BTNB] == BTN_STATUS_RELEASED)
    {
      if (rollMode == 0)
      {
        incrementDiceType();
      }
      else if (rollMode == 1)
      {
        incrementDiceCount();
      }
      roll();
    }
    else if (btnStatus[BTNB] == BTN_STATUS_HOLD)
    {
      rollMode++;
      if (rollMode > 1)
      {
        rollMode = 0;
      }
      roll();
    }
    else if(btnStatus[BTNA] == BTN_STATUS_PRESSED)
    {
      roll();
    }
    else if(btnStatus[BTNPWR] == BTN_STATUS_PRESSED)
    {
      roll();
    }
  }
  else if (mode == MODE_FLASHLIGHT)
  {
    brightness = 255;
  }
  if (prevDeviceConnected != deviceConnected)
  {
    redraw = true;
    prevDeviceConnected = deviceConnected;
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
        clockColor = colors[getRandom(0, 8)];
        StickCP2.Display.setTextColor(clockColor);
        redraw = true;
        break;
      case MODE_FLASHLIGHT:
        drawDelay = MODE_FLASHLIGHT_DRAW_DELAY;
        brightness = 255;
        break;
      case MODE_ALARM:
        drawDelay = MODE_ALARM_DRAW_DELAY;
        break;
      case MODE_ROLL:
        drawDelay = DEFAULT_DRAW_DELAY;
        diceCount = 2;
        diceType = 6;
        roll();
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
    case MODE_ROLL:
      displayRoll();
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
  brightness = 255;
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
    if (deviceConnected)
    {
      StickCP2.Display.setCursor(150, 20);
      StickCP2.Display.printf("BLE");
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

void displayRoll()
{
  StickCP2.Display.clear();
  StickCP2.Display.setTextSize(1.0f);

  StickCP2.Display.setCursor(20, 20);
  StickCP2.Display.printf("ROLL %dK%d", diceCount, diceType);

  if (rollMode == 0)
  {
    StickCP2.Display.setCursor(150, 25);
    StickCP2.Display.printf("_");
  }
  else
  {
    StickCP2.Display.setCursor(110, 25);
    StickCP2.Display.printf("_");
  }

  StickCP2.Display.setTextSize(0.9f);
  //StickCP2.Display.setCursor(170, 20);
  //auto resultCount = sizeof(diceResult)/sizeof(diceResult[0]);
  //int len = *(&diceResult + 1) - diceResult;
  //StickCP2.Display.printf("(%d)", resultCount);

  for (int i=0; i<diceCount; i++)
  {
    int result = diceResult[i];
    if (result <= 0)
    {
      continue;
    }
    
    StickCP2.Display.setCursor((35 * (i+1)) - 15, 50);
    if (i == diceCount - 1)
    {
      StickCP2.Display.printf("%d", result);
    }
    else
    {
      StickCP2.Display.printf("%d,", result);
    }
  }
  StickCP2.Display.setTextSize(2.5f);
  StickCP2.Display.setCursor(30, 90);
  StickCP2.Display.printf("= %d", diceSum);
  StickCP2.Display.setTextSize(1.0f);
}

void incrementDiceType()
{
  int oldType = diceType;

  for (int i=0; i<DICE_TYPES_COUNT; i++)
  {
    if (diceTypes[i] > diceType)
    {
      diceType = diceTypes[i];
      break;
    }
  }

  if (diceType == oldType)
  {
    diceType = diceTypes[0];
  }

  clearDiceResults();
}

void incrementDiceCount()
{
  diceCount++;
  if (diceCount > DICE_COUNT_MAX)
  {
    diceCount = 1;
  }
  clearDiceResults();
}

void roll()
{
  clearDiceResults();
  diceSum = 0;
  for (int i=0; i<diceCount; i++)
  {
    diceResult[i] = (rand() % diceType) + 1;
    diceSum += diceResult[i];
  }
}

void clearDiceResults()
{
  diceResult = new int[6] {0,0,0,0,0,0}; //new int(diceCount);
  for (int i=0; i<DICE_COUNT_MAX; i++)
  {
    diceResult[i] = 0;
  }
}

