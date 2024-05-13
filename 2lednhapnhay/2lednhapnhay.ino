const int buttonPin = 13;  // Chân của nút nhấn
const int ledPin = 12;    // Chân của đèn LED

int buttonState = 0;  // Trạng thái của nút
bool ledBlinking = false; // Biến trạng thái cho biết liệu đèn LED có đang nhấp nháy hay không
int buttonPressCount = 0; // Biến để theo dõi số lần nút được nhấn

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); // Sử dụng resistor kéo lên
  Serial.begin(9600); // Khởi động giao tiếp serial
}

void loop() {
  // Đọc trạng thái của nút
  buttonState = digitalRead(buttonPin);
  
  // Kiểm tra nếu nút được nhấn
  if (buttonState == LOW) { // Đọc mức logic LOW khi sử dụng INPUT_PULLUP
    // Tăng biến đếm số lần nhấn nút lên 1
    buttonPressCount++;
    
    // Kiểm tra nếu đèn đang nhấp nháy
    if (ledBlinking) {
      // Tắt nhấp nháy đèn và đặt biến trạng thái về false
      ledBlinking = false;
      digitalWrite(ledPin, LOW);
    } else {
      // Bật nhấp nháy đèn và đặt biến trạng thái về true
      ledBlinking = true;
      digitalWrite(ledPin, HIGH);
      // Bật nhấp nháy đèn LED
      blinkLED();
    }

    // Đặt biến trạng thái của nút lại thành HIGH để tránh lặp lại lệnh khi nút được giữ
    delay(200);
    while (digitalRead(buttonPin) == LOW) {
      // Đợi cho nút được thả ra
    }
    delay(200);
  }

  // Nếu nút đã được nhấn hai lần, đặt lại biến đếm số lần nhấn nút về 0
  if (buttonPressCount == 2) {
    buttonPressCount = 0;
  }
}

// Hàm để nhấp nháy đèn LED
void blinkLED() {
  while (ledBlinking) {
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  }
}
