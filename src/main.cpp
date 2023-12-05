#include <Arduino.h>
#include <dht.h>
#include <SoftwareSerial.h>
// put function declarations here:
dht DHT;
#define DHT11_PIN 7
#define Sensor_AO A2
#define Sensor_DO A1

unsigned int sensorValue = 0; // 烟雾传感器值
const int BuzzerPin = 13;     // 蜂鸣器引脚
const int DigitalInPin = 8;   // 火焰传感器
const int LDR_PIN = A0;       // 光敏传感器
int ldrVal = 0;               // 存放光敏传感器读取到的数值
const int relayPin = 9;       // 继电器
//const int interruptPin = digitalPinToInterrupt(DigitalInPin); // 将引脚号转换为中断号

unsigned long previousMillis = 0;
const long interval = 30000; // 10秒的延时

void DHT_Data();
void MQ_2();
void DigitalIn();
void LDR_Data();
void Fmq();
void MSG();


void setup()
{
  // put your setup code here, to run once:
  pinMode(DigitalInPin, INPUT);
  //attachInterrupt(interruptPin, MSG, RISING); // 当引脚变为低电平时触发 MSG() 函数


  pinMode(Sensor_DO, INPUT);
  pinMode(LDR_PIN, INPUT); // 设置光敏电阻的引脚为输入模式
  pinMode(BuzzerPin, OUTPUT);//输出模式
  pinMode(relayPin, OUTPUT);
  Serial.begin(115200);
  while (!Serial)
  { // 串口是否准备好。
    ;
  }
  //连接MQTT服务器
  delay(5000);
  Serial.print("AT+CIPMODE=1\r\n");
  delay(2000);

  Serial.print("AT+CWJAP=\"xxx\",\"xxx\"\r\n"); // 连接WIFI
  delay(5000);

  Serial.print("AT+MQTTUSERCFG=0,1,\"ESP321\",\"espressif\",\"1234567890\",0,0,\"\"\r\n");
  delay(5000);

  Serial.print("AT+MQTTCONN=0,\"192.168.1.170\",1883,0\r\n");
  delay(5000);

  Serial.print("WiFi ok");
  delay(1000);
}

void loop()
{
  unsigned long currentMillis = millis();
  //非阻塞延时
  if (currentMillis - previousMillis >= interval) {
    // 在此处执行你的10秒延时的任务
    DHT_Data();  // 温湿度
    MQ_2();      // 烟雾传感器
    LDR_Data();  // 光敏传感器
    MSG(); //MQTT消息发送
    previousMillis = currentMillis;
  }

  DigitalIn(); // 火焰传感器
  
}

void MSG(){
  String jsonMessage = "{";
  jsonMessage += "\\\"temp\\\":\\\"" + String(DHT.temperature, 1) + "\\\"\\,";
  jsonMessage += "\\\"humi\\\":\\\"" + String(DHT.humidity, 1) + "\\\"\\,";
  jsonMessage += "\\\"MQ_2\\\":\\\"" + String(sensorValue) + "\\\"\\,";
  jsonMessage += "\\\"LDR\\\":\\\"" + String(ldrVal) + "\\\"\\,";
  jsonMessage += "\\\"DigitalIn\\\":\\\"" + String(digitalRead(DigitalInPin)) + "\\\"\\";
  jsonMessage += "}";
  Serial.println("AT+MQTTPUB=0,\"topic\",\"" + jsonMessage + "\",0,0\r\n");
}


void DHT_Data()
{
  // put your main code here, to run repeatedly:
  Serial.print("DHT11: ");
  int chk = DHT.read11(DHT11_PIN);
  
  Serial.print(DHT.humidity, 1);
  Serial.print(",\t");
  Serial.println(DHT.temperature, 1);
  //delay(1000);

  // double t = DHT.temperature;
  // double h = DHT.humidity;
  // 发送消息上服务器
  //Serial.println("AT+MQTTPUB=0,\"topic\",\"{\\\"temp\\\":\\\"" + String(t) + "\\\"\\,\\\"humi\\\":\\\"" + String(h) + "\\\"}\",0,0\r\n");
  //delay(2000);
}

void MQ_2()
{
  sensorValue = analogRead(Sensor_AO);
  Serial.print("MQ_2_Value = ");
  Serial.println(sensorValue);
  // 发送消息上服务器
  //Serial.println("AT+MQTTPUB=0,\"topic\",\"{\\\"MQ_2\\\":\\\"" + String(sensorValue) + "\\\"}\",0,0\r\n");
  if (sensorValue>100){
    //蜂鸣器
    Fmq();
    delay(500);
    }
  if (digitalRead(Sensor_DO) == LOW)
  {
    //Serial.println("Alarm!!!");
    Fmq();    
  }
  //delay(2000);
}

void DigitalIn()
{
  boolean stat = digitalRead(DigitalInPin);
  //Serial.print("DigitalIn:");
  //Serial.println(stat);
  // 发送消息上服务器
  //Serial.println("AT+MQTTPUB=0,\"topic\",\"{\\\"DigitalIn\\\":\\\"" + String(stat) + "\\\"}\",0,0\r\n");

  if (stat == HIGH)
  {
    noTone(13);
    digitalWrite(BuzzerPin, HIGH);
    digitalWrite(relayPin, HIGH);
    //Serial.print("relay start\r\n");
  }
  if (stat == LOW)
  {
    MSG(); //MQTT消息发送
    digitalWrite(BuzzerPin, LOW);
    digitalWrite(relayPin, LOW); // 继电器断电
    //Serial.print("relay stop\r\n");
    pinMode(BuzzerPin, OUTPUT); //蜂鸣器响
  }
  delay(1000);
}

void LDR_Data()
{
  ldrVal = analogRead(LDR_PIN); // 读取光敏电阻模块的数值
  Serial.print("LDR Value: ");  // 输出串口信息
  Serial.println(ldrVal);
  if (ldrVal < 300)
  {
    //digitalWrite(relayPin, HIGH); 继电器启动
    //Serial.print("LEd relay start\r\n");
  }
  if (ldrVal > 300)
  {
    //digitalWrite(relayPin, LOW); // 继电器断电
    //Serial.print("LEd relay stop\r\n");
  }
  delay(500);                 // 延时1秒钟
}

void Fmq()
{
  pinMode(BuzzerPin, OUTPUT);
  delay(1000);
  pinMode(BuzzerPin, LOW);
}
