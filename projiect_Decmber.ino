#include <WiFi.h>
#include <PubSubClient.h>
#include "EmonLib.h"

// WiFi连接信息
const char* ssid = "Xiaomi_76E5";
const char* password = "12345678";

// MQTT设置
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqttuser = "helloword";
const char* mqttpassword = "331800";
const char* mqttpubtopic = "/home/power/current";
const char* mqttsubtopic = "/home/back";
const char* mqttID = "wang";
// 电流传感器设置
#define current_pin 34
#define current_cal 111.1

// 继电器引脚定义
#define relay_pin 27

// 尝试连接到MQTT服务器的最大次数
const int maxReconnectAttempts = 10;
int reconnectAttempts = 0;

EnergyMonitor emon1;
WiFiClient espClient;
PubSubClient client(espClient);

// 控制继电器状态的变量
bool relayState = false;

void setup() {
  Serial.begin(115200); // 初始化串口通信
  emon1.current(current_pin, current_cal); // 设置电流传感器
  pinMode(relay_pin, OUTPUT); // 设置继电器引脚为输出
  digitalWrite(relay_pin, LOW); // 初始化继电器为关闭状态
  setupWiFi(); // 连接WiFi
  client.setServer(mqtt_server, mqtt_port); // 设置MQTT服务器和端口
  client.setCallback(callback); // 设置MQTT回调函数
  while(!client.connected()){
    Serial.println("connecting to mqtt brocker");
    if (client.connect(mqttID,mqttuser,mqttpassword)){
      client.subscribe(mqttsubtopic);
      client.subscribe(mqttpubtopic);
      Serial.println("MQTT OK");
    } else {
      delay(5000);
      Serial.println("MQTT连接失败，重试...");
    
    }
  }
}

void loop() {
  if (!client.connected()) {
    if (reconnectAttempts < maxReconnectAttempts) {
      reconnect();
      reconnectAttempts++;
    } else {
      Serial.println("MQTT连接失败次数过多，停止重试。");
      // 可以在这里添加其他错误处理代码，比如重启ESP32
      // 延时一段时间后再次尝试，或者进入错误处理循环
      delay(60000); // 例如，每分钟尝试一次重连
    }
  } else {
    reconnectAttempts = 0; // 重置重试计数器，因为我们已经成功连接了
  }

  client.loop(); // 处理传入的MQTT消息

  // 每5秒测量并发布电流
  double Irms = emon1.calcIrms(1480); // 计算RMS电流
  char msg[50];
  snprintf(msg, 50, "%.2f", Irms); // 格式化消息
  client.publish(mqttpubtopic, msg); // 发布消息
  delay(5000); // 等待5秒
  

}

void setupWiFi() {
  delay(10); // 在开始WiFi连接前稍作延迟
  Serial.println("正在连接WiFi...");
  WiFi.begin(ssid, password); // 开始WiFi连接
  while (WiFi.status() != WL_CONNECTED) { // 等待WiFi连接
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi已连接"); // 连接成功后打印确认信息
}

void reconnect() {
  while(!client.connected()){
    Serial.println("connecting to mqtt brocker");
    if (client.connect(mqttID,mqttuser,mqttpassword)){
      client.subscribe(mqttsubtopic);
      client.subscribe(mqttpubtopic);
      Serial.println("MQTT OK");
    } else {
      delay(5000);
      Serial.println("MQTT连接失败，重试...");
    
    }
  }
}

// MQTT回调函数
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("消息到达 [");
  Serial.print(topic);
  Serial.println("] ");

  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.println(msg);

  if (strcmp(topic, mqttsubtopic) == 0) {
    int receivedValue = atoi(msg);
    Serial.print("接收到的值: ");
    Serial.println(receivedValue);

    if (receivedValue == 1) {
      digitalWrite(relay_pin, HIGH);
      relayState = true;
      Serial.println("继电器已打开");
    } else if (receivedValue == 0) {
      digitalWrite(relay_pin, LOW);
      relayState = false;
      Serial.println("继电器已关闭");
    }
  }
}