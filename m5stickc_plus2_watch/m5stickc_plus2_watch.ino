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

float normalized_max = (MAX_VOLTAGE - MIN_VOLTAGE)*1.0f;

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(3);
    StickCP2.Display.setTextColor(RED);
    StickCP2.Display.setTextDatum(middle_center);
    StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
    StickCP2.Display.setTextSize(1);
}

void loop() {
    StickCP2.Display.clear();
    int vol = StickCP2.Power.getBatteryVoltage();
    int normalized_vol = vol - MIN_VOLTAGE;
    int percent = (int)((normalized_vol * 1.0f) / (normalized_max) * 100.0f);

    StickCP2.Display.setTextSize(1);
    StickCP2.Display.setCursor(20, 20);
    StickCP2.Display.printf("%d %%", percent);

    StickCP2.Display.setTextSize(2.0f);
    //auto t = time(nullptr);
    auto dt = StickCP2.Rtc.getDateTime();

    //auto tm = localtime(&t);  // for local timezone.

    StickCP2.Display.setCursor(50, 90);
    int hour = ((dt.time.hours)+TIMEZONE)%24;
    if (hour > 23)
    {
      hour = 0;
    }
    StickCP2.Display.printf("%02d:%02d", hour, dt.time.minutes);
    //StickCP2.Display.printf("%02d:%02d:%02d", dt.time.hours, dt.time.minutes, dt.time.seconds);
    
    StickCP2.Display.setTextSize(0.8f);

    StickCP2.Display.setCursor(130, 20);
    StickCP2.Display.printf("%dmv", vol);

    StickCP2.Display.setCursor(130, 50);
    StickCP2.Display.printf("%02d/%02d", dt.date.date, dt.date.month);
    delay(1000);
}
