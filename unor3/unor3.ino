#include <AFMotor.h>

// AF_DCMotor motor(2, MOTOR12_64KHZ);  // tạo động cơ #2, 64KHz pwm
AF_DCMotor motor(3);  // tạo động cơ #3

String receivedData = "";  // Biến lưu chuỗi nhận được

void setup() {
  Serial.begin(9600);   // Kết nối với Serial monitor
  motor.setSpeed(255);  // chọn tốc độ (maximum 255/255)
}

void loop() {
  // motor.run(FORWARD);  // động cơ tiến
  motor.run(BACKWARD);  // động cơ lùi
  //   motor.run(RELEASE);  // động cơ dừng
}