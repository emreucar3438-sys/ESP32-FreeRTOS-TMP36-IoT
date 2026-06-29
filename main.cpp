#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
const char* ssid = "enter your wifi name";
const char* password = "enter your wifi password";
const char* mqtt_server = "broker.hivemq.com"; 
WiFiClient espClient;
PubSubClient client(espClient);

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
int temperaturesensor = 34;
QueueHandle_t xtemperatureQueue;
QueueHandle_t xDisplayQueue;
QueueHandle_t xMqttQueue;

void TaskSensorRead(void *pvParameter){
  for(;;){
    int temperature = analogRead(temperaturesensor);
    xQueueSend(xtemperatureQueue, &temperature, portMAX_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS);  
  }
}
float applyEMA(float rawValue, float *filteredValue) {
    float alpha = 0.1;
    *filteredValue = (alpha * (float)rawValue) + ((1.0 - alpha) * (*filteredValue));
    return *filteredValue;
}

void TaskDataProcess(void *pvParameter){
    int rawADC;

    static float emaResult = 0;
  for(;;){
    xQueueReceive(xtemperatureQueue, &rawADC, portMAX_DELAY);
    float voltage = (rawADC*3.3f)/4095.0f;
    float celsius = (voltage - 0.5f) * 100.0f;
    float cleanValue = applyEMA(celsius, &emaResult);
    xQueueSend(xDisplayQueue, &cleanValue, portMAX_DELAY);
    xQueueSend(xMqttQueue, &cleanValue, 0);
    Serial.println(cleanValue);}
  }
void TaskDisplay(void *pvParameter){
  for(;;){
    float cleanValue;
if(xQueueReceive(xDisplayQueue, &cleanValue, portMAX_DELAY)==pdTRUE){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("--- SENSOR MONITOR ---");
  display.setCursor(0, 25);
  display.print("Temperature: ");
  display.print(cleanValue, 2);
  display.println(" C");
  display.display();
}

  }
}
void TaskNetwork(void *pvParameter){
  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);
  float cleanValue;
  
  for(;;){
    if (WiFi.status() == WL_CONNECTED) {
      if (!client.connected()) {
        Serial.print("Trying MQTT connection...");
        if (client.connect("ESP32_Emre_Client")) {
          Serial.println("Connected!");
        } else {
          Serial.print("Error code: ");
          Serial.println(client.state());
          vTaskDelay(2000 / portTICK_PERIOD_MS);
          continue; 
        }
      }
      client.loop();
    } else {
      
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      continue;
    }

   
    if(xQueueReceive(xMqttQueue, &cleanValue, portMAX_DELAY) == pdTRUE) {
      if(client.connected()) {
        String payload = String(cleanValue, 2); 
        client.publish("emre/temperature", payload.c_str());
        Serial.print("MQTT data sent: ");
        Serial.println(payload);
      }
    }
  }
}
void setup(){
  Serial.begin(115200);
  pinMode(temperaturesensor, INPUT);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    for(;;){
      Serial.println("ERROR: OLED screen could not be started!");
      delay(1000); 
    }
  }
  else{
    display.clearDisplay();
    display.display();
  }
  xtemperatureQueue = xQueueCreate(5, sizeof(int));
  xDisplayQueue = xQueueCreate(5, sizeof(float));
  xMqttQueue = xQueueCreate(5, sizeof(float));
  xTaskCreate(TaskSensorRead, "TaskSensorRead", 2048, NULL, 2, NULL);
  xTaskCreate(TaskDataProcess, "TaskDataProcess", 2048, NULL, 2, NULL);
  xTaskCreate(TaskDisplay, "TaskDisplay", 4096, NULL, 1, NULL);
  xTaskCreate(TaskNetwork, "TaskNetwork", 4096, NULL, 1, NULL);

}
void loop(){

}