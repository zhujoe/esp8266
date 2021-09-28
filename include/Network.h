// 在这里可以找到网络连接相关函数的定义
#ifndef NETWORK_H
#define NETWORK_H

#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

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

class Network
{
private:
    char const *mqtt_server = "175.24.88.178"; //"192.168.31.127";
    char const *itopic = "cube/dashboard";

public:
    // Network(void);
    void init(void);
    void wifiSmartConfig(void);
    // 初始化mqtt消息内容结构体
    void initCurrentData(void);
    // mqtt消息响应需要的回调方法
    static void mqttCallback(char *topic, byte *payload, unsigned int length);
    void mqttConnect(void);
    struct DashboardData getCurrentData(void);
    // 同步互联网时间
    void ntpSync(void);
    String getTime(void);
};

#endif