//-----------包含---------------
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino_JSON.h>
const char *ssid = "CMCC-daxiong";
const char *password = "dengyi1991";
const char *mqtt_server = "121.36.171.33";
const char *mqtt_username = "server";
const char *mqtt_password = "passwd";

//-----------创建对象-----------
WiFiClient client;
PubSubClient mqttClient(client);

//----------定义变量----------
//指示灯的pin
int ledPin = 2;
//继电器控制pin
int relayPin = 13;
int controlPin = 5;
int ledState = 0;
int lastState = 1;
#include <Ticker.h>
Ticker ticker;
bool canSendHeartBeat = true;
String deviceId = "1489602874843439106";

//----------定义函数----------
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void sendHeartBeat();

void setup()
{

    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);

    pinMode(controlPin, INPUT_PULLUP);
    //连接WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        ledState = !ledState;
        digitalWrite(ledPin, ledState);
    }
    Serial.println("wifi connected");

    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(callback);
    // 30秒一次心跳上报
    ticker.attach(30, sendHeartBeat);
}

void loop()
{

    //保持mqtt连接
    if (!mqttClient.connected())
    {
        reconnect();
    }
    mqttClient.loop();

    if (canSendHeartBeat && mqttClient.connected())
    {
        String queueBeat = "heartbeat/" + String(deviceId);
        mqttClient.publish(queueBeat.c_str(), "online", 1);
        canSendHeartBeat = false;
    }
    int currentState = digitalRead(controlPin);
    //检测控制脚，状态改变后将状态发送给mqtt
    if (currentState != lastState)
    {
        lastState = currentState;
        if (currentState == 1)
        {
            digitalWrite(relayPin, LOW);
            //关闭
            String result = "{\"on\":false}";
            String queueReport = "report/" + String(deviceId);
            mqttClient.publish(queueReport.c_str(), result.c_str(), 1);
        }
        else
        {
            digitalWrite(relayPin, HIGH);
            //开启
            String result = "{\"on\":true}";
            String queueReport = "report/" + String(deviceId);
            mqttClient.publish(queueReport.c_str(), result.c_str(), 1);
        }
    }

    delay(100);
}
void callback(char *topic, byte *payload, unsigned int length)
{

    String configJson = "";
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    // JSONVar myObject = JSON.parse(payload);
    for (int i = 0; i < length; i++)
    {
        configJson += (char)payload[i];
    }
    Serial.println(configJson);
    JSONVar myObject = JSON.parse(configJson);
    bool openStatus = (bool)myObject["on"];
    if (openStatus)
    {
        digitalWrite(relayPin, HIGH);
    }
    else
    {
        digitalWrite(relayPin, LOW);
    }
}

void reconnect()
{
    while (!mqttClient.connected())
    {
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password))
        {
            digitalWrite(ledPin, LOW);
            Serial.println("connected");
            String queueBeat = "heartbeat/" + String(deviceId);
            mqttClient.publish(queueBeat.c_str(), "online", 1);
            //订阅
            String queueControl = "control/" + String(deviceId);
            mqttClient.subscribe(queueControl.c_str());
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
    Serial.println("MQTT Connected!");
}
void sendHeartBeat()
{
    canSendHeartBeat = true;
}
