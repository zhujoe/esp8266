#ifndef DECLARATION_FUN_H
#define DECLARATION_FUN_H

void zeroScreenBuf(uint8_t *);
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

uint8_t *timePage();
uint8_t *totalityPage();
uint8_t *uptimePage();
uint8_t *cpuPage();
uint8_t *diskPage();
uint8_t *memoryPage();
uint8_t *networkPage();
// uint8_t* noMessagesPage();
uint8_t *rebootPage();
uint8_t *doreboot();
uint8_t *shutdownPage();
uint8_t *doshutdown();
uint8_t *serverPage();
uint8_t *datePage();
uint8_t *notePage();
uint8_t *lifePage();
uint8_t *countDownPage();
uint8_t *controlPage();
uint8_t *volumePage();
uint8_t *settingPage();
uint8_t *dosettingPage();

void initdev();
void clockCallback();
void initCurrentData();
void serverHomePage();
void light_sleep();

#endif