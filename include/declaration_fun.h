#ifndef DECLARATION_FUN_H
#define DECLARATION_FUN_H

/*-------------------------------------------------------
    jingwei_wificfg
  -------------------------------------------------------*/

void smartcfgPage_off();
void smartcfgPage_on();
void wifiErrPage();
void smartConfig();
String wifi_smartConfigGetStr();
void wifi_setup();
void restWifiConfig();

/*-------------------------------------------------------
    jingwei_eeprom
  -------------------------------------------------------*/
void wifi_eepromGetStr(int);
void wifi_eepromPutStr(int, String);

/*-------------------------------------------------------
    jingwei_mqtt
  -------------------------------------------------------*/

void mqttCallback(char *, byte *, unsigned int);
void mqtt_reconnect();

/*-------------------------------------------------------
    jingwei_menu.c
  -------------------------------------------------------*/

void timePage();
void totalityPage();
void uptimePage();
void cpuPage();
void diskPage();
void memoryPage();
void networkPage();
void noMessagesPage();
void rebootPage();
void doreboot();
void shutdownPage();
void doshutdown();
void serverPage();
void datePage();
void notePage();
void lifePage();
void countDownPage();
void controlPage();
void volumePage();
void settingPage();
void dosettingPage();

void initdev();
void clockCallback();
void initCurrentData();
void serverHomePage();
void light_sleep();

#endif