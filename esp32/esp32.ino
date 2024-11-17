// ESP32 code that sends the message to the arduino uno

#include <HardwareSerial.h>

HardwareSerial mySerial1(1);

void setup() {
  Serial.begin(115200); // Serial monitor
  Serial1.begin(4800, SERIAL_8N1, 19, 21); // UART1 với TX = GPIO 19, RX = GPIO 21 (có thể thay đổi nếu cần)

  delay(1000); // Đợi hệ thống ổn định
}

void loop() {
  String dataToSend = "01 2 RUN";
  Serial1.println(dataToSend); // Gửi chuỗi qua Serial1
  Serial.println(dataToSend); // Gửi chuỗi qua Serial1
  delay(500); // Gửi mỗi 0.5 giây
}
