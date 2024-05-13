// define library //
#include <Servo.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// define PIN //
#define PIR_PIN 13
#define LED_PIN 12
#define SONIC_TRIGGER_PIN 6
#define SONIC_ECHO_PIN 5
#define SONIC_MAX_DISTANCE 20
#define SERVO_PIN 10
#define MOTOR_PIN1 9
#define MOTOR_PIN2 8
#define MOTOR_SPEED_PIN 3
#define BUTTON_PIN 2

// define values for ESP8266 and Firebase //
/* 1. Define the WiFi credentials */
#define WIFI_SSID "HI4 COFFEE & WORKSPACE"                        
#define WIFI_PASSWORD "24242424"

/* 2. Define the API Key */
#define API_KEY "AIzaSyAorqQLPRiR_oTKdKeMQZphPESbUi_qOBo"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "vdk-gun"

/* 4. Define the user Email and password */
#define USER_EMAIL "hoangleanhtuannnk@gmail.com"
#define USER_PASSWORD "anhtuan1605"

// define variables //
Servo servo;
int servoAngle = 0;           // Góc hiện tại của servo
bool increasingAngle = true;  // Biến để theo dõi việc tăng hoặc giảm góc của servo
bool movingServo = true;      // Biến để kiểm tra trạng thái di chuyển của servo
bool systemActive = true;
unsigned long debounceDelay = 50;    // milliseconds
unsigned long lastDebounceTime = 0;  // Thêm khai báo cho biến debounce time
unsigned long currentMillis = 0;      // Biến để lưu thời gian hiện tại

// Define Firebase Data object //
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool taskCompleted = false;

unsigned long dataMillis = 0;

// CODE

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SONIC_TRIGGER_PIN, OUTPUT);
  pinMode(SONIC_ECHO_PIN, INPUT);
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(MOTOR_SPEED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), checkButtonState, FALLING);  // Gán hàm xử lý sự kiện cho interrupt của nút nhấn

  servo.attach(SERVO_PIN);  // Servo được kết nối với chân 9 của Arduino

  // Wi-Fi connection and Firebase Setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
      Serial.print(".");
      delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  #if defined(ESP8266)
      // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
      fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
  #endif

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && (millis() - currentMillis > 500 || currentMillis == 0))
    {
      currentMillis = millis(); // Cập nhật thời gian hiện tại
      String documentPath = "turret/turret_1";
      String mask = "";
      Serial.print("Get a document... ");
      if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask.c_str()))
      {
        // print JsonData from Firebase
        //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        // Create a FirebaseJson object and set content with received payload
        FirebaseJson payload;
        payload.setJsonData(fbdo.payload().c_str());
        // Get the data from FirebaseJson object 
        FirebaseJsonData jsonData;

        //get isAutoValue
        payload.get(jsonData, "fields/isAuto/booleanValue", true);
        bool isAuto = jsonData.boolValue;

        if (!isAuto) {
          // Auto mode off , you have to control by app
        } else {
          // Auto mode activate
          if (systemActive) {
            // Đọc trạng thái của PIR và cảm biến siêu âm
            int pirState = digitalRead(PIR_PIN);
            int sonicDistance = measureDistance();

            // In ra thông tin PIR, cảm biến siêu âm và khoảng cách đo được
            Serial.print("PIR state: ");
            Serial.print(pirState == HIGH ? "Motion detected" : "No motion detected");
            Serial.print(", Sonic distance: ");
            Serial.print(sonicDistance);
            Serial.println(" cm");

            // Kiểm tra điều kiện dừng
            if (pirState == LOW) {
              movingServo = false;             // Dừng di chuyển servo
              TurnOffDC();
            } else if (pirState == HIGH && sonicDistance > 50) {
              movingServo = true;             // Bắt đầu di chuyển servo
              TurnOffDC();
            } else if (sonicDistance < 50) {
              movingServo = false;             // Dừng di chuyển servo
              TurnOnDC();
            }
            // Nếu servo đang di chuyển và chưa đạt giới hạn
            if (movingServo && servoAngle >= 0 && servoAngle <= 180) {
              servo.write(servoAngle);  // Gửi góc hiện tại của servo
              // Tăng hoặc giảm góc của servo tùy thuộc vào biến increasingAngle
              if (increasingAngle) {servoAngle += 10;  // Tăng góc của servo
                // Nếu servo đạt góc tối đa, chuyển hướng di chuyển
                if (servoAngle >= 180) {
                  increasingAngle = false;
                }
              } else {
                servoAngle -= 10;  // Giảm góc của servo
                // Nếu servo đạt góc thấp nhất, chuyển hướng di chuyển
                if (servoAngle <= 0) {
                  increasingAngle = true;
                }
              }
            }
          } else {
            // Hệ thống không hoạt động, tắt servo và motor
            servo.detach();
            TurnOffDC();
          }
        }
      }
    else
        Serial.println(fbdo.errorReason());
    }
  void TurnOffDC() {
    digitalWrite(MOTOR_PIN1, LOW);  // Stop DC motor
    digitalWrite(MOTOR_PIN2, LOW);
    analogWrite(MOTOR_SPEED_PIN, 0);  // Stop motor
    digitalWrite(LED_PIN, LOW);
  }
  void TurnOnDC() {
    digitalWrite(MOTOR_PIN1, HIGH);  // Start DC motor
    digitalWrite(MOTOR_PIN2, LOW);
    digitalWrite(LED_PIN, HIGH);
    analogWrite(MOTOR_SPEED_PIN, 255);  // Max speed
  }
  int measureDistance() {
    // Gửi xung trigger đo khoảng cách
    digitalWrite(SONIC_TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(SONIC_TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(SONIC_TRIGGER_PIN, LOW);

    long duration = pulseIn(SONIC_ECHO_PIN, HIGH);
    // Chuyển đổi thời gian thành khoảng cách (đơn vị cm)
    int distance = duration * 0.0343 / 2;
    return distance;
  }

  void checkButtonState() {
    // Kiểm tra debounce
    if (currentMillis - lastDebounceTime > debounceDelay) {
      // Thực hiện hành động nếu nút nhấn được nhấn với debounce
      systemActive = !systemActive;
      lastDebounceTime = currentMillis; // Cập nhật thời gian debounce
      if (systemActive) {
        // Nếu hệ thống được bật lại, kết nối lại servo
        servo.attach(SERVO_PIN);
      }
    }
  }
}

