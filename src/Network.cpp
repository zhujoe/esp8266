#include "Network.h"

// void Network(void)
// {
// }

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

static int const timezone = 8;
static WiFiUDP ntpUDP;
static NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", timezone * 3600, 60000);

static DashboardData currentData;

void Network::initCurrentData(void)
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
}

void Network::wifiSmartConfig(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
    Serial.println("@smartconfig is running");
    while (true)
    {
        delay(500);
        if (WiFi.smartConfigDone())
        {
            break;
        }
    }
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.println(".");
    }
    
    Serial.println("@wifi is connected");
}

void Network::mqttCallback(char *topic, byte *payload, unsigned int length)
{
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

void Network::mqttConnect(void)
{
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(mqttCallback);
    Serial.println("@connecting mqtt service");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    while (!mqttClient.connected())
    {

        if (mqttClient.connect(clientId.c_str()))
        {
            // mqttClient.publish(itopic, "hello world");
            mqttClient.subscribe(itopic);
            mqttClient.setBufferSize(2048);
        }
        else
        {
            delay(3000);
        }
    }
    Serial.println("@mqtt service is connected: " + clientId);
}

struct DashboardData Network::getCurrentData(void)
{
    return currentData;
}

void Network::ntpSync(void)
{
    timeClient.update();
}

String Network::getTime(void)
{
    return timeClient.getFormattedTime();
}

void Network::init(void)
{
    initCurrentData();
    wifiSmartConfig();
    timeClient.begin();
    ntpSync();
    mqttConnect();
}