#include <AFMotor.h>
#include <SoftwareSerial.h>

#define ID 1

#define TX_PIN 13
#define RX_PIN 2

const int stopSwitchPinFront = A3;  // Công tắc tắt đằng trước(Stop)
const int stopSwitchPinBack = A2;   // Công tắc tắt đằng sau(Stop)
const int stopSwitchPinStop = A1;   // Công tắc tắt dừng(Stop)
// bool motorRunning = false;    // Biến trạng thái của động cơ

// Khai báo SoftwareSerial
SoftwareSerial Serial1(RX_PIN, TX_PIN);  // RX, TX

// AF_DCMotor motor(2, MOTOR12_64KHZ);  // tạo động cơ #2, 64KHz pwm
AF_DCMotor motor(3);  // tạo động cơ #3

String receivedData = "";  // Biến lưu chuỗi nhận được
int checkRun = 0;          // Biến lưu giá trị hiện tại của động cơ

void setup() {
  Serial.begin(115200);  // Kết nối với Serial monitor
  Serial1.begin(4800);   // Kết nối với ESP32

  pinMode(stopSwitchPinFront, INPUT_PULLUP);  // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinBack, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinStop, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên
  motor.setSpeed(255);                        // chọn tốc độ (maximum 255/255)
}

void loop() {
  if (Serial1.available()) {
    char c = Serial1.read();  // Đọc từng ký tự từ Serial1
    receivedData += c;        // Ghép từng ký tự vào chuỗi
    if (c == '\n') {          // Khi gặp ký tự xuống dòng (\n), nghĩa là đã nhận đủ chuỗi
      receivedData.trim();    // Loại bỏ ký tự khoảng trắng thừa
      Serial.println("Chuỗi nhận được: " + receivedData);

      // Tách và xử lý chuỗi
      int index = 0;
      String parts[3];  // Mảng lưu các phần: ID, Lệnh, Dữ liệu
      while (receivedData.length() > 0 && index < 3) {
        int spaceIndex = receivedData.indexOf(' ');  // Tìm dấu cách
        if (spaceIndex == -1) {
          parts[index] = receivedData;  // Lưu phần cuối cùng
          break;
        }
        parts[index] = receivedData.substring(0, spaceIndex);   // Tách chuỗi trước khoảng trắng
        receivedData = receivedData.substring(spaceIndex + 1);  // Cắt phần đã tách ra
        index++;
      }

      // In ra các phần đã tách
      Serial.println("ID: " + parts[0]);
      Serial.println("Lệnh: " + parts[1]);

      if (parts[0].toInt() != ID) {
        receivedData = "";
        return;  // Nếu ID không trùng khớp, thoát khỏi hàm
      }

      if (parts[1].equals("RUN")) {
        Serial.println("run DC motor");
        motor.run(FORWARD);  // động cơ stop
        checkRun = 1;
      }

      receivedData = "";  // Xóa chuỗi để chuẩn bị nhận chuỗi mới
    }
  }
  // Serial.println(receivedData);
  // Serial.print("cong tac truoc: ");
  // Serial.println(digitalRead(stopSwitchPinFront));
  // Serial.print("cong tac sau: ");
  // Serial.println(digitalRead(stopSwitchPinBack));
  // Serial.print("cong tac dung: ");
  // Serial.println(digitalRead(stopSwitchPinStop));
  //delay(200);
  if (digitalRead(stopSwitchPinFront) == 0) {
    motor.run(RELEASE);   // động cơ stop
    motor.run(BACKWARD);  // động cơ lùi
    Serial.println("Dong co lui lai");
  }
  if (digitalRead(stopSwitchPinBack) == 0) {
    motor.run(RELEASE);  // động cơ stop
    Serial.println("Dong co dung lai");
    checkRun = 0;
  }
  if (digitalRead(stopSwitchPinStop) == 0 && checkRun == 1) {
    motor.run(RELEASE);  // động cơ stop
    Serial.println("Dong co stop");
    String id = "1";     // Thay thế bằng ID mong muốn
    String data = "OK";  // Thay thế bằng dữ liệu thực tế

    // Tạo chuỗi theo định dạng "ID DATA"
    String sendString = id + " " + data;
    // Gửi chuỗi qua SoftwareSerial tới ESP32
    Serial1.println(sendString);  // Gửi chuỗi và xuống dòng để ESP32 nhận biết kết thúc chuỗi

    // In chuỗi ra để kiểm tra trên màn hình Serial Monitor
    Serial.println("Gửi chuỗi: " + sendString);
    checkRun = 0;
    delay(100);
  }
}
