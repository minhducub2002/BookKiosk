// Chân điều khiển
int dirPinM2 = 25;  // Chân DIR_B
int pwmPinM2 = 26;  // Chân PWM_B

void setup() {
  // Khởi tạo chân điều khiển là OUTPUT
  pinMode(dirPinM2, OUTPUT);
  pinMode(pwmPinM2, OUTPUT);
}

void loop() {
  // Quay động cơ theo chiều thuận
  digitalWrite(dirPinM2, HIGH);  // Đặt chiều quay
  analogWrite(pwmPinM2, 200);    // Đặt tốc độ quay (0-255)
  delay(2000);                   // Quay trong 2 giây

  // Quay động cơ theo chiều ngược lại
  digitalWrite(dirPinM2, LOW);   // Đổi chiều quay
  analogWrite(pwmPinM2, 200);    // Đặt tốc độ quay (0-255)
  delay(2000);                   // Quay trong 2 giây

  // Dừng động cơ
  analogWrite(pwmPinM2, 0);
  delay(2000);                   // Dừng trong 2 giây
}
