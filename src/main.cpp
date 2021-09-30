#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <ESP8266mDNS.h>
#include "declaration_fun.h"

#define EEPROM_INITADDR 0
#define EEPROM_SETUPNUMADDR 1
#define EEPROM_WIFIADDR 2

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 12, 14);
int onboardledPin = 2;
int enterVal, backVal, rightVal, leftVal;
int timezone = 8;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", timezone * 3600, 60000);
ESP8266WebServer mcuserver(80);
char const *mDNSname = "setting";
bool serverflg = 0;
struct Menu
{
  int index;
  int left;
  int right;
  int enter;
  int back;
  void (*callback)();
};
uint8_t enterKey = 5;
uint8_t backKey = 4;
uint8_t rightKey = 13;
uint8_t leftKey = 0;
char const *mqtt_server = "175.24.88.178"; //"192.168.31.127";
char const *itopic = "cube/dashboard";
uint8_t intervalmessagetime = 5;
int timenowEpoch = 0;
int recmessagetime = 0;
uint8_t nomessagesflag = 1;
WiFiClient espClient;
PubSubClient client(espClient);
struct DashboardData
{
  int up_time;
  float cpu_percent;
  int cpu_current;
  float virtual_memory_free;
  float virtual_memory_total;
  float swap_memory_free;
  float swap_memory_total;
  float disk_usage_free;
  float disk_usage_percent;
  int cpu_temperature;
  int time_timestamp_cst;
  float net_speed_recv;
  float net_speed_sent;
  String remote_ip;
  String host_name;
  int tm_stamp_cst;
  int tm_year;
  int tm_mon;
  int tm_mday;
  int alert;
};
DashboardData currentData;
struct EepromData
{
  String init;
  String setupnum;
  String wifi_ssid;
  String wifi_password;
};
EepromData memoryData;
Ticker restclock;
bool wifisetupflg = 1;

void ICACHE_FLASH_ATTR wifi_smartConfig()
{
  int wificonnum = 0;
  smartcfgPage_off();
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  while (true)
  {
    delay(500);
    if (WiFi.smartConfigDone())
    {
      digitalWrite(onboardledPin, HIGH);
      delay(500);
      digitalWrite(onboardledPin, LOW);
      break;
    }
  }

  smartcfgPage_on();
  delay(1000);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    wificonnum++;
    if (wificonnum > 20)
    {
      EEPROM.write(EEPROM_INITADDR, 0);
      EEPROM.commit();
      u8g2.setFontDirection(0);
      wifiErrPage();
      u8g2.setFontDirection(1);
      delay(2000);
      ESP.restart();
    }
  }
  digitalWrite(onboardledPin, HIGH);
  wifisetupflg = 0;
}

String ICACHE_FLASH_ATTR wifi_smartConfigGetStr()
{
  String wificonfig = "";

  wificonfig += WiFi.SSID().c_str();
  memoryData.wifi_ssid = WiFi.SSID().c_str();
  wificonfig += " ";
  wificonfig += WiFi.psk().c_str();
  memoryData.wifi_password = WiFi.psk().c_str();

  return wificonfig;
}

void ICACHE_FLASH_ATTR wifi_eepromGetStr(int addr)
{
  String dataread;
  char *strlist[2];
  unsigned int y = 0;
  unsigned int i = 0;
  unsigned int endflg = 0;

  for (unsigned int i = 0; i < EEPROM.read(addr); i++)
  {
    dataread += char(EEPROM.read((addr + 1) + i));
  }
  for (unsigned x = 0; x < 2; x++)
  {
    strlist[x] = (char *)malloc(20 * sizeof(char));
  }
  for (i = 0; i < dataread.length(); i++, endflg++)
  {
    if (dataread[i] == ' ')
    {
      strlist[y][endflg] = '\0';
      endflg = 0;
      y++;
      i++;
    }
    strlist[y][endflg] = dataread[i];
  }
  strlist[y][endflg++] = '\0';
  memoryData.wifi_ssid = strlist[0];
  memoryData.wifi_password = strlist[1];
  for (unsigned x = 0; x < 2; x++)
  {
    free(strlist[x]);
  }
}

void ICACHE_FLASH_ATTR wifi_eepromPutStr(int addr, String datawritten)
{
  EEPROM.write(addr, datawritten.length());
  for (unsigned int i = 0; i < datawritten.length(); i++)
  {
    EEPROM.write((addr + 1) + i, datawritten[i]);
  }
  EEPROM.commit();
}

void ICACHE_FLASH_ATTR initdev()
{
  wifi_smartConfig();
  wifi_eepromPutStr(EEPROM_WIFIADDR, wifi_smartConfigGetStr());
  EEPROM.write(EEPROM_SETUPNUMADDR, 0);
  EEPROM.write(EEPROM_INITADDR, 1);
  EEPROM.commit();
}

void ICACHE_FLASH_ATTR clockCallback()
{
  EEPROM.write(EEPROM_SETUPNUMADDR, 0);
  EEPROM.commit();
}

void ICACHE_FLASH_ATTR restWifiConfig()
{
  EEPROM.write(EEPROM_SETUPNUMADDR, EEPROM.read(EEPROM_SETUPNUMADDR) + 1);
  EEPROM.commit();
  if (EEPROM.read(EEPROM_SETUPNUMADDR) < 3)
  {
    wifi_eepromGetStr(EEPROM_WIFIADDR);
    restclock.once(3, clockCallback);
  }
  else
  {
    initdev();
  }
}

void ICACHE_FLASH_ATTR initCurrentData()
{
  currentData.up_time = 0;
  currentData.cpu_percent = 0;
  currentData.cpu_temperature = -128;
  currentData.cpu_current = 0;
  currentData.disk_usage_free = 0;
  currentData.disk_usage_percent = 0;
  currentData.virtual_memory_free = 0;
  currentData.virtual_memory_total = 0;
  currentData.swap_memory_free = 0;
  currentData.swap_memory_total = 0;
  currentData.time_timestamp_cst = 0;
  currentData.net_speed_recv = 0;
  currentData.net_speed_sent = 0;
  currentData.remote_ip = "0.0.0.0";
  currentData.host_name = "null";
  currentData.tm_stamp_cst = 0;
  currentData.tm_year = 0;
  currentData.tm_mon = 0;
  currentData.tm_mday = 0;
  currentData.alert = 0;
};

void ICACHE_FLASH_ATTR wifi_setup()
{
  int wificonnum = 0;

  WiFi.begin(memoryData.wifi_ssid, memoryData.wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    wificonnum++;
    if (wificonnum > 20)
    {
      EEPROM.write(EEPROM_INITADDR, 0);
      EEPROM.write(EEPROM_SETUPNUMADDR, 0);
      EEPROM.commit();
      u8g2.setFontDirection(0);
      wifiErrPage();
      u8g2.setFontDirection(1);
      delay(2000);
      ESP.restart();
    }
  }
  digitalWrite(onboardledPin, HIGH);
}

void ICACHE_FLASH_ATTR mqttCallback(char *topic, byte *payload, unsigned int length)
{
  recmessagetime = timeClient.getEpochTime();

  String jsonString = "";
  for (int i = 0; i < (int)length; i++)
  {
    jsonString += (char)payload[i];
  }
  DynamicJsonDocument doc((int)length * 4);
  DeserializationError err = deserializeJson(doc, jsonString);
  switch (err.code())
  {
  case DeserializationError::Ok:
    currentData.up_time = doc["up_time"];
    currentData.cpu_percent = doc["cpu_percent"];
    currentData.cpu_temperature = doc["cpu_temperature"];
    currentData.cpu_current = doc["cpu_freq"]["current"];
    currentData.time_timestamp_cst = doc["time"]["timestamp_cst"];
    currentData.virtual_memory_free = doc["virtual_memory"]["free"];
    currentData.virtual_memory_total = doc["virtual_memory"]["total"];
    currentData.swap_memory_free = doc["swap_memory"]["free"];
    currentData.swap_memory_total = doc["swap_memory"]["total"];
    currentData.disk_usage_free = doc["disk_usage"]["free"];
    currentData.disk_usage_percent = doc["disk_usage"]["percent"];
    currentData.net_speed_recv = doc["net_io_speed"]["recv"];
    currentData.net_speed_sent = doc["net_io_speed"]["sent"];
    currentData.remote_ip = (String)doc["ip"];
    currentData.host_name = (String)doc["host_name"];
    currentData.tm_stamp_cst = doc["time"]["timestamp_cst"];
    currentData.tm_year = doc["time"]["tm_year"];
    currentData.tm_mon = doc["time"]["tm_mon"];
    currentData.tm_mday = doc["time"]["tm_mday"];
    currentData.alert = doc["alert"];
    break;
  case DeserializationError::InvalidInput:
    break;
  case DeserializationError::NoMemory:
    break;
  default:
    break;
  }
}

void ICACHE_FLASH_ATTR mqtt_reconnect()
{
  while (!client.connected())
  {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str()))
    {
      // client.publish(itopic, "hello world");
      client.subscribe(itopic);
      client.setBufferSize(2048);
    }
    else
    {
      u8g2.sendBuffer();
      delay(3000);
    }
  }
}

void ICACHE_FLASH_ATTR headPage()
{
  // u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
  // u8g2.drawGlyph(0, 8, 123);
  u8g2.setFont(u8g2_font_artossans8_8n);
  u8g2.setCursor(8, 8);
  u8g2.print(timeClient.getFormattedTime());

  if (currentData.alert)
  {
    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g2.drawGlyph(120, 8, 238); //cpu占用100%
  }

  if (nomessagesflag == 1)
  {
    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g2.drawGlyph(110, 8, 197); // mqtt disconnect
  }
  else
  {
    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g2.drawGlyph(110, 8, 198); // mqtt connect
  }

  u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
  u8g2.drawGlyph(100, 8, 248); // wifi

  if (currentData.net_speed_sent >= 0.01)
  {
    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g2.drawGlyph(90, 8, 143); // updata
  }
  if (currentData.net_speed_recv >= 0.01)
  {
    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g2.drawGlyph(80, 8, 142); // download
  }
  u8g2.drawHLine(0, 9, 128);
}

void ICACHE_FLASH_ATTR timePage()
{
  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_timB24_tn);
  u8g2.setCursor(0, 50);
  u8g2.print(timeClient.getFormattedTime());

  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR totalityPage()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setCursor(netx, nety);
  u8g2.print("cpu_temperature   ");
  u8g2.print(currentData.cpu_temperature);
  u8g2.print("'C");
  nety += 9;

  u8g2.setCursor(netx, nety);
  u8g2.print("cpu_percent      ");
  u8g2.print(currentData.cpu_percent);
  u8g2.print("%");
  nety += 9;

  u8g2.setCursor(netx, nety);
  u8g2.print("net_recv         ");
  u8g2.print(currentData.net_speed_recv);
  u8g2.print("mb");
  nety += 9;

  u8g2.setCursor(netx, nety);
  u8g2.print("memory_free   ");
  // u8g2.print( (currentData.virtual_memory_free*100+0.05)/100 );
  u8g2.print(currentData.virtual_memory_free);
  u8g2.print("mb");
  nety += 9;

  u8g2.setCursor(netx, nety);
  u8g2.print("disk_free   ");
  u8g2.print(currentData.disk_usage_free);
  u8g2.print("mb");
  nety += 9;

  u8g2.setCursor(netx, nety);
  u8g2.print("disk_percent    ");
  u8g2.print(currentData.disk_usage_percent);
  u8g2.print("%");
  // nety += 9;

  // u8g2.setCursor(netx, nety);
  // u8g2.print("time_stamp  ");
  // u8g2.print(currentData.tm_stamp_cst);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR uptimePage()
{
  int second = 0;
  int minute = 0;
  int hour = 0;
  uint8_t netx = 2;
  uint8_t nety = 18;

  second = currentData.up_time % 60;
  minute = currentData.up_time / 60 % 60;
  hour = currentData.up_time / 3600;
  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print(hour);
  u8g2.print("h ");
  u8g2.print(minute);
  u8g2.print("m ");
  u8g2.print(second);
  u8g2.print("s");

  u8g2.setCursor(2, 60);
  u8g2.setFont(u8g2_font_8x13_mf);
  u8g2.print("Up_time");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 123);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR cpuPage()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("temperature  ");
  u8g2.print(currentData.cpu_temperature);
  u8g2.print("'C");
  nety += 11;

  u8g2.setCursor(netx, nety);
  u8g2.print("percent ");
  u8g2.print(currentData.cpu_percent);
  u8g2.print("%");
  nety += 11;

  u8g2.setCursor(netx, nety);
  u8g2.print("current ");
  u8g2.print(currentData.cpu_current);
  u8g2.print("Mhz");

  u8g2.setCursor(2, 60);
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.print("Cpu");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 177);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR diskPage()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("free  ");
  u8g2.print(currentData.disk_usage_free);
  u8g2.print("mb");
  nety += 11;

  u8g2.setCursor(netx, nety);
  u8g2.print("percent  ");
  u8g2.print(currentData.disk_usage_percent);
  u8g2.print("%");
  nety += 11;

  u8g2.setCursor(2, 60);
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.print("Disk");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 171);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR memoryPage()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("v-total ");
  u8g2.print(currentData.virtual_memory_total);
  u8g2.print("mb");
  nety += 11;

  u8g2.setCursor(netx, nety);
  u8g2.print("v-free  ");
  u8g2.print(currentData.virtual_memory_free);
  u8g2.print("mb");
  nety += 11;

  // u8g2.setCursor(netx, nety);
  // u8g2.print("s-total ");
  // u8g2.print(currentData.swap_memory_total);
  // u8g2.print(" mb");
  // nety += 11;

  // u8g2.setCursor(netx, nety);
  // u8g2.print("s-free  ");
  // u8g2.print(currentData.swap_memory_free);
  // u8g2.print(" mb");

  u8g2.setCursor(2, 60);
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.print("Memory");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 139);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR networkPage()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("ip    ");
  u8g2.print(currentData.remote_ip);
  nety += 11;

  // u8g2.setCursor(netx, nety);
  // u8g2.print("name  ");
  // u8g2.print(currentData.host_name);
  // nety += 11;

  u8g2.setCursor(netx, nety);
  u8g2.print("recv  ");
  u8g2.print(currentData.net_speed_recv);
  u8g2.print("mb");
  nety += 11;

  u8g2.setCursor(netx, nety);
  u8g2.print("sent  ");
  u8g2.print(currentData.net_speed_sent);
  u8g2.print("mb");

  u8g2.setCursor(2, 60);
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.print("Network");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 194);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR noMessagesPage()
{
  uint8_t netx = 2;
  uint8_t nety = 11;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("no messages received!");
  nety += 11;
  u8g2.setCursor(netx, nety);
  u8g2.print("check mqtt server");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Mqtt_Err");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 121);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR rebootPage()
{
  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Reboot");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 243);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR doreboot()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  client.publish(itopic, "reboot", 0);
  u8g2.clearBuffer();
  headPage();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("success");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("reboot");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 120);
  u8g2.sendBuffer();
  delay(2000);
  ESP.restart();
}

void ICACHE_FLASH_ATTR shutdownPage()
{
  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Shutdown");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 235);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR doshutdown()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  client.publish(itopic, "shutdown", 0);
  u8g2.clearBuffer();
  headPage();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("success");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Shutdown");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 120);
  u8g2.sendBuffer();
  delay(2000);
  ESP.restart();
}

void ICACHE_FLASH_ATTR smartcfgPage_off()
{
  // uint8_t netx = 2;
  // uint8_t nety = 17;

  u8g2.clearBuffer();

  // u8g2.setFont(u8g2_font_6x10_tf);
  // u8g2.setCursor(netx, nety);
  // u8g2.print("instruction");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Smartcfg");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 197);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR smartcfgPage_on()
{
  // uint8_t netx = 2;
  // uint8_t nety = 17;

  u8g2.clearBuffer();

  // u8g2.setFont(u8g2_font_6x10_tf);
  // u8g2.setCursor(netx, nety);
  // u8g2.print("instruction");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Smartcfg");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 198);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR wifiErrPage()
{
  uint8_t netx = 2;
  uint8_t nety = 11;

  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("no Internet!");
  nety += 11;
  u8g2.setCursor(netx, nety);
  u8g2.print("check your wifi");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("wifi_err");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 121);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR serverPage()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  u8g2.clearBuffer();
  headPage();

  if (nomessagesflag == 1)
  {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(netx, nety);
    u8g2.print("There is no terminal access");
  }

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Server");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 222);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR datePage()
{
  // uint8_t netx = 2;
  // uint8_t nety = 17;

  u8g2.clearBuffer();
  headPage();
  // u8g2.setFont(u8g2_font_6x10_tf);
  // u8g2.setCursor(netx, nety);
  // u8g2.print("instruction");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Date");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 249);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR notePage()
{
  // uint8_t netx = 2;
  // uint8_t nety = 17;

  u8g2.clearBuffer();
  headPage();
  // u8g2.setFont(u8g2_font_6x10_tf);
  // u8g2.setCursor(netx, nety);
  // u8g2.print("instruction");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("NotePad");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 257);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR lifePage()
{
  // uint8_t netx = 2;
  // uint8_t nety = 17;

  u8g2.clearBuffer();
  headPage();
  // u8g2.setFont(u8g2_font_6x10_tf);
  // u8g2.setCursor(netx, nety);
  // u8g2.print("instruction");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Life");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 98);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR countDownPage()
{
  // uint8_t netx = 2;
  // uint8_t nety = 17;

  u8g2.clearBuffer();
  headPage();
  // u8g2.setFont(u8g2_font_6x10_tf);
  // u8g2.setCursor(netx, nety);
  // u8g2.print("instruction");

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Countdown");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 269);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR controlPage()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  u8g2.clearBuffer();
  headPage();

  if (nomessagesflag == 1)
  {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(netx, nety);
    u8g2.print("There is no terminal access");
  }

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Control");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 265);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR volumePage()
{
  u8g2.clearBuffer();
  headPage();
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Volume");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 105);
  u8g2.sendBuffer();
}

void ICACHE_FLASH_ATTR settingPage()
{
  u8g2.clearBuffer();
  headPage();

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Setting");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 129);
  u8g2.sendBuffer();

  if (serverflg)
  {
    serverflg = 0;
    mcuserver.stop();
    // MDNS.close();
  }
}

void ICACHE_FLASH_ATTR serverHomePage()
{
  mcuserver.send(200, "text/plain", "test homepage !");
}

void ICACHE_FLASH_ATTR doSettingPage()
{
  uint8_t netx = 2;
  uint8_t nety = 18;

  u8g2.clearBuffer();
  headPage();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(netx, nety);
  u8g2.print("local ");
  u8g2.print(WiFi.localIP());
  // nety += 11;

  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.setCursor(2, 60);
  u8g2.print("Setting");

  u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
  u8g2.drawGlyph(95, 62, 120);
  u8g2.sendBuffer();

  if (serverflg == 0)
  {
    MDNS.begin(mDNSname);
    mcuserver.on("/", serverHomePage);
    mcuserver.begin();
    serverflg = 1;
  }
}

// void ICACHE_FLASH_ATTR light_sleep()
// {
//   wifi_station_disconnect();
//   wifi_set_opmode_current(NULL_MODE);
//   wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
//   wifi_fpm_open();
//   gpio_pin_wakeup_enable(GPIO_ID_PIN(5), GPIO_PIN_INTR_LOLEVEL);
//   wifi_fpm_do_sleep(0xFFFFFFF);
//   u8g2.setPowerSave(1);
// }

Menu menu[21] = {
    //---------time-----------
    {0, 18, 4, 1, 0, timePage},
    {1, 3, 2, 1, 0, countDownPage},
    {2, 1, 3, 2, 0, datePage},
    {3, 2, 1, 3, 0, lifePage},
    //---------server----------
    {4, 0, 17, 5, 4, serverPage},
    {5, 11, 6, 5, 4, totalityPage},
    {6, 5, 7, 6, 4, cpuPage},
    {7, 6, 8, 7, 4, diskPage},
    {8, 7, 9, 8, 4, memoryPage},
    {9, 8, 10, 9, 4, networkPage},
    {10, 9, 11, 10, 4, uptimePage},
    //-------server_control-------
    {11, 10, 5, 12, 4, controlPage},
    {12, 16, 14, 13, 11, rebootPage},
    {13, 13, 13, 13, 13, doreboot},
    {14, 12, 16, 15, 11, shutdownPage},
    {15, 15, 15, 15, 15, doshutdown},
    {16, 14, 12, 16, 11, volumePage},
    //---------note----------
    {17, 4, 18, 17, 17, notePage},
    //--------setting----------
    {18, 17, 0, 19, 18, settingPage},
    {19, 19, 19, 19, 18, doSettingPage},
    //--------other status-----
    {20, 20, 20, 20, 20, noMessagesPage},

};
void (*operation_index)();
int func_index = 0;
// int stack_func_index = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(onboardledPin, OUTPUT);
  digitalWrite(onboardledPin, LOW);

  EEPROM.begin(1024);
  u8g2.begin();

  Serial.println(EEPROM.read(EEPROM_INITADDR));
  Serial.println(EEPROM.read(EEPROM_SETUPNUMADDR));

  if (EEPROM.read(EEPROM_INITADDR) != 1)
  {
    initdev();
  }
  else
  {
    restWifiConfig();
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_open_iconic_all_6x_t);
  u8g2.drawGlyph(40, 48, 97);
  u8g2.drawRFrame(30, 54, 68, 6, 2);
  u8g2.sendBuffer();

  pinMode(enterKey, INPUT_PULLUP);
  pinMode(backKey, INPUT_PULLUP);
  pinMode(rightKey, INPUT_PULLUP);
  pinMode(leftKey, INPUT_PULLUP);

  u8g2.setFont(u8g2_font_baby_tf);
  u8g2.setFontDirection(1);

  randomSeed(micros()); //
  u8g2.drawRBox(30, 54, 10, 6, 2);
  u8g2.drawStr(0, 0, "wifi");
  u8g2.sendBuffer();

  if (wifisetupflg)
  {
    wifi_setup(); //
    u8g2.drawRBox(30, 54, 20, 6, 2);
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 0, 6, 60);
    u8g2.setDrawColor(1);
    u8g2.drawStr(0, 0, "client_set");
    u8g2.sendBuffer();
  }

  client.setServer(mqtt_server, 1883); //
  client.setCallback(mqttCallback);    //
  u8g2.drawRBox(30, 54, 40, 6, 2);
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 6, 60);
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 0, "time_set");
  u8g2.sendBuffer();

  timeClient.begin(); //
  u8g2.drawRBox(30, 54, 48, 6, 2);
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 6, 60);
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 0, "time_update");
  u8g2.sendBuffer();

  timeClient.update(); //
  u8g2.drawRBox(30, 54, 56, 6, 2);
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 6, 60);
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 0, "mqtt");
  u8g2.sendBuffer();

  mqtt_reconnect(); //
  client.loop();    //
  u8g2.drawRBox(30, 54, 63, 6, 2);
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 6, 60);
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 0, "ready");
  u8g2.sendBuffer();

  u8g2.drawRBox(30, 54, 68, 6, 2);
  u8g2.sendBuffer();
  recmessagetime = timeClient.getEpochTime();
  u8g2.setFontDirection(0);
}

void loop()
{
  if (!client.connected())
  {
    noMessagesPage();
    mqtt_reconnect();
  }
  client.loop();

  if (serverflg)
  {
    mcuserver.handleClient();
  }

  enterVal = digitalRead(enterKey);
  backVal = digitalRead(backKey);
  leftVal = digitalRead(leftKey);
  rightVal = digitalRead(rightKey);

  nomessagesflag = 0;
  timenowEpoch = timeClient.getEpochTime();
  if (timenowEpoch - recmessagetime > intervalmessagetime)
  {
    nomessagesflag = 1;
  }

  if ((enterVal == LOW) || (backVal == LOW) || (leftVal == LOW) || (rightVal == LOW))
  {
    delay(5);
    if (enterVal == LOW)
    {
      while (!enterVal)
      {
        enterVal = digitalRead(enterKey);
        operation_index = menu[func_index].callback;
        (*operation_index)();
      }
      func_index = menu[func_index].enter;
    }
    if (backVal == LOW)
    {
      while (!backVal)
      {
        backVal = digitalRead(backKey);
        operation_index = menu[func_index].callback;
        (*operation_index)();
      }
      func_index = menu[func_index].back;
    }
    if (leftVal == LOW)
    {
      while (!leftVal)
      {
        leftVal = digitalRead(leftKey);
        operation_index = menu[func_index].callback;
        (*operation_index)();
      }
      func_index = menu[func_index].left;
    }
    if (rightVal == LOW)
    {
      while (!rightVal)
      {
        rightVal = digitalRead(rightKey);
        operation_index = menu[func_index].callback;
        (*operation_index)();
      }
      func_index = menu[func_index].right;
    }
  }

  operation_index = menu[func_index].callback;
  (*operation_index)();
}