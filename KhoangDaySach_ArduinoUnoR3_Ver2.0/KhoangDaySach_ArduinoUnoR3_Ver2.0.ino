#define ID 2

#define TX_PIN 13
#define RX_PIN 12

#define IN1 6
#define IN2 5
#define ENA 7
#define MAX_SPEED 255  //từ 0-255
#define MIN_SPEED 0

const int stopSwitchPinFront = A3;  // Công tắc tắt đằng trước(Stop)
const int stopSwitchPinBack = A2;   // Công tắc tắt đằng sau(Stop)
const int stopSwitchPinStop = A1;   // Công tắc tắt dừng(Stop)

// Khai báo SoftwareSerial
SoftwareSerial Serial1(RX_PIN, TX_PIN);  // RX, TX

String receivedData = "";  // Biến lưu chuỗi nhận được
int checkStop = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(4800);  // Kết nối với ESP32

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(stopSwitchPinFront, INPUT_PULLUP);  // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinBack, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinStop, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên

  // pinMode(IN3, OUTPUT);
  // pinMode(IN4, OUTPUT);
}

void motor_1_Dung() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

void motor_1_Tien(int speed) {                     //speed: từ 0 - MAX_SPEED
  speed = constrain(speed, MIN_SPEED, MAX_SPEED);  //đảm báo giá trị nằm trong một khoảng từ 0 - MAX_SPEED - http://arduino.vn/reference/constrain
  digitalWrite(IN1, HIGH);                         // chân này không có PW
  digitalWrite(IN2, LOW);                          // chân này không có PWM
  analogWrite(ENA, speed);
}

void motor_1_Lui(int speed) {
  speed = constrain(speed, MIN_SPEED, MAX_SPEED);  //đảm báo giá trị nằm trong một khoảng từ 0 - MAX_SPEED - http://arduino.vn/reference/constrain
  digitalWrite(IN1, LOW);                          // chân này không có PWM
  digitalWrite(IN2, HIGH);                         // chân này không có PWM
  analogWrite(ENA, speed);
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
        motor_1_Tien(MAX_SPEED);  // động cơ tiến
      }
      // if (parts[2] == "STOP") {
      //   Serial.println("stop DC motor");
      //   motor_1_Dung();  // động cơ dừng
      // }
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

  if (digitalRead(stopSwitchPinFront) == 0) {
    motor_1_Dung();          // động cơ stop
    motor_1_Lui(MAX_SPEED);  // động cơ lùi
    Serial.println("Dong co lui lai");
  }
  if (digitalRead(stopSwitchPinBack) == 0) {
    motor_1_Tien(MAX_SPEED);
    checkStop = 1;
    //motor_1_Dung();  // động cơ stop
    Serial.println("Dong co dung lai");
  }
  if (digitalRead(stopSwitchPinBack) == 1 && checkStop == 1) {
    motor_1_Dung();  // động cơ stop
    checkStop = 0;
  }
  if (digitalRead(stopSwitchPinStop) == 0) {
    motor_1_Dung();  // động cơ stop
    Serial.println("Dong co stop");
    String id = "1";     // Thay thế bằng ID mong muốn
    String data = "OK";  // Thay thế bằng dữ liệu thực tế

    // Tạo chuỗi theo định dạng "ID DATA"
    String sendString = id + " " + data;
    // Gửi chuỗi qua SoftwareSerial tới ESP32
    Serial1.println(sendString);  // Gửi chuỗi và xuống dòng để ESP32 nhận biết kết thúc chuỗi

    // In chuỗi ra để kiểm tra trên màn hình Serial Monitor
    Serial.println("Gửi chuỗi: " + sendString);
    // Serial1.print(" 1324");
    // Serial1.print("\n");
  }
  //motor_1_Tien(MAX_SPEED);  // motor 1 tiến
  //Serial.println("Dong co dang tien!");
  // delay(5000);              //tiến 5 s
  // motor_2_Lui(MAX_SPEED);   //motor 2 lùi
  // motor 1 vẫn tiến
  delay(200);  //delay 10 s
  // motor_1_Lui(MAX_SPEED);
  // Serial.println("Dong co dang lui!");
  // motor_2_Dung();
  // delay(2000);  //dừng 2s
  // delay(10000);  //delay 10 s
}