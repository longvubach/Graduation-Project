#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// ===== CẤU HÌNH WIFI =====
const char* ssid = "677 5G";
const char* password = "10101010";

// ===== CẤU HÌNH MQTT =====
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port = 1883;
const char* mqtt_topic = "servo/angle"; 

// ===== CẤU HÌNH PCA9685 =====
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
#define SERVOMIN  120 
#define SERVOMAX  520

// ===== ĐỊNH NGHĨA CÁC KÊNH TRÊN PCA9685 (MỚI) =====
#define CH_THUMB     0  // Ngón cái
#define CH_INDEX     1  // Ngón trỏ
#define CH_MIDDLE    2  // Ngón giữa
#define CH_RING      3  // Ngón áp út
#define CH_PINKY     4  // Ngón út

WiFiClient espClient;
PubSubClient client(espClient);

// ===== RTOS QUEUES VÀ SEMAPHORES =====
QueueHandle_t queueThumb, queueIndex, queueMiddle, queueRing, queuePinky;
SemaphoreHandle_t i2cMutex;

// ===== KHAI BÁO HÀM =====
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setAngle(uint8_t channel, int angle);
void controlHand(int t, int i, int m, int r, int p);

// ===== RTOS TASK FUNCTIONS =====
void taskThumb(void *pvParameters);
void taskIndex(void *pvParameters);
void taskMiddle(void *pvParameters);
void taskRing(void *pvParameters);
void taskPinky(void *pvParameters);

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setClock(100000);
  
  // 1. Khởi tạo RTOS queues và semaphores
  queueThumb = xQueueCreate(5, sizeof(int));
  queueIndex = xQueueCreate(5, sizeof(int));
  queueMiddle = xQueueCreate(5, sizeof(int));
  queueRing = xQueueCreate(5, sizeof(int));
  queuePinky = xQueueCreate(5, sizeof(int));
  i2cMutex = xSemaphoreCreateMutex();
  
  if (queueThumb == NULL || queueIndex == NULL || queueMiddle == NULL || 
      queueRing == NULL || queuePinky == NULL || i2cMutex == NULL) {
    Serial.println("LỖI: Không thể tạo queues/semaphores!");
    while(1) delay(10);
  }
  
  // 2. Khởi tạo PCA9685
  if (!pwm.begin()) {
    Serial.println("LỖI: Không thể khởi tạo PCA9685!");
    while(1) delay(10);
  }
  pwm.setPWMFreq(50);
  
  // 3. Tạo RTOS tasks cho từng servo
  xTaskCreate(taskThumb, "TaskThumb", 2048, NULL, 2, NULL);
  xTaskCreate(taskIndex, "TaskIndex", 2048, NULL, 2, NULL);
  xTaskCreate(taskMiddle, "TaskMiddle", 2048, NULL, 2, NULL);
  xTaskCreate(taskRing, "TaskRing", 2048, NULL, 2, NULL);
  xTaskCreate(taskPinky, "TaskPinky", 2048, NULL, 2, NULL);
  
  Serial.println("Đã tạo 5 RTOS tasks cho 5 ngón tay!");
  delay(1000);
  
  // 4. Test tất cả các kênh khi khởi động
  Serial.println("Đang test tất cả các ngón tay...");
  controlHand(90, 90, 90, 90, 90);
  delay(2000);
  Serial.println("Test hoàn tất!");
  
  // 5. Kết nối WiFi
  setup_wifi();
  
  // 6. Cấu hình MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("Hệ thống đã sẵn sàng nhận lệnh MQTT...");
}

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}
// Hàm nhận lệnh và điều khiển ngón tay
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("Lệnh nhận được: " + message);

  if (message == "0") {
    Serial.println("--> Đang MỞ bàn tay...");
    delay(100);
    controlHand(180, 180, 180, 0, 0);
    delay(150);
    controlHand(30, 80, 70, 110, 110);
    delay(150);
    controlHand(180, 180, 180, 0, 0);
  } 
  else {
    Serial.println("--> Đang NẮM bàn tay...");
    // Góc mở: 0 độ cho tất cả các ngón
    delay(100);
    controlHand(30, 80, 70, 110, 110);
  }

}

// Hàm bổ trợ để điều khiển nhanh 5 ngón - gửi lệnh vào queues
void controlHand(int t, int i, int m, int r, int p) {
  // Giới hạn góc
  t = constrain(t, 0, 180);
  i = constrain(i, 0, 180);
  m = constrain(m, 0, 180);
  r = constrain(r, 0, 180);
  p = constrain(p, 0, 180);
  
  // Gửi lệnh vào queues của từng task
  xQueueSend(queueThumb, &t, 0);
  xQueueSend(queueIndex, &i, 0);
  xQueueSend(queueMiddle, &m, 0);
  xQueueSend(queueRing, &r, 0);
  xQueueSend(queuePinky, &p, 0);
  
  // Debug: In thông tin
  Serial.println("Đã gửi lệnh điều khiển 5 ngón:");
  Serial.print("  Ngón cái: "); Serial.print(t); Serial.println("°");
  Serial.print("  Ngón trỏ: "); Serial.print(i); Serial.println("°");
  Serial.print("  Ngón giữa: "); Serial.print(m); Serial.println("°");
  Serial.print("  Ngón áp út: "); Serial.print(r); Serial.println("°");
  Serial.print("  Ngón út: "); Serial.print(p); Serial.println("°");
}

void setAngle(uint8_t channel, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(channel, 0, pulse);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Hand-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  vTaskDelay(15 / portTICK_PERIOD_MS)
}

// ===== RTOS TASK IMPLEMENTATIONS =====

// Task điều khiển ngón cái
void taskThumb(void *pvParameters) {
  int targetAngle = 90;
  int currentAngle = 90;
  
  while(1) {
    // Kiểm tra queue có lệnh mới không
    if (xQueueReceive(queueThumb, &targetAngle, 0) == pdTRUE) {
      // Có lệnh mới, bắt đầu di chuyển
    }
    
    // Di chuyển mượt từ góc hiện tại đến góc mục tiêu
    if (currentAngle != targetAngle) {
      if (currentAngle < targetAngle) {
        currentAngle++;
      } else {
        currentAngle--;
      }
      
      // Bảo vệ I2C bus với mutex
      if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        setAngle(CH_THUMB, currentAngle);
        xSemaphoreGive(i2cMutex);
      }
    }
    
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task điều khiển ngón trỏ
void taskIndex(void *pvParameters) {
  int targetAngle = 90;
  int currentAngle = 90;
  
  while(1) {
    if (xQueueReceive(queueIndex, &targetAngle, 0) == pdTRUE) {
    }
    
    if (currentAngle != targetAngle) {
      if (currentAngle < targetAngle) {
        currentAngle++;
      } else {
        currentAngle--;
      }
      
      if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        setAngle(CH_INDEX, currentAngle);
        xSemaphoreGive(i2cMutex);
      }
    }
    
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task điều khiển ngón giữa
void taskMiddle(void *pvParameters) {
  int targetAngle = 90;
  int currentAngle = 90;
  
  while(1) {
    if (xQueueReceive(queueMiddle, &targetAngle, 0) == pdTRUE) {
    }
    
    if (currentAngle != targetAngle) {
      if (currentAngle < targetAngle) {
        currentAngle++;
      } else {
        currentAngle--;
      }
      
      if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        setAngle(CH_MIDDLE, currentAngle);
        xSemaphoreGive(i2cMutex);
      }
    }
    
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task điều khiển ngón áp út
void taskRing(void *pvParameters) {
  int targetAngle = 90;
  int currentAngle = 90;
  
  while(1) {
    if (xQueueReceive(queueRing, &targetAngle, 0) == pdTRUE) {
    }
    
    if (currentAngle != targetAngle) {
      if (currentAngle < targetAngle) {
        currentAngle++;
      } else {
        currentAngle--;
      }
      
      if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        setAngle(CH_RING, currentAngle);
        xSemaphoreGive(i2cMutex);
      }
    }
    
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Task điều khiển ngón út
void taskPinky(void *pvParameters) {
  int targetAngle = 90;
  int currentAngle = 90;
  
  while(1) {
    if (xQueueReceive(queuePinky, &targetAngle, 0) == pdTRUE) {
    }
    
    if (currentAngle != targetAngle) {
      if (currentAngle < targetAngle) {
        currentAngle++;
      } else {
        currentAngle--;
      }
      
      if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        setAngle(CH_PINKY, currentAngle);
        xSemaphoreGive(i2cMutex);
      }
    }
    
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}