#define ID 2

#define IN1 8
#define IN2 9
#define ENA 6
#define MAX_SPEED 255  //từ 0-255
#define MIN_SPEED 0

const int stopSwitchPinFront = 1;  // Công tắc tắt đằng trước(Stop)
const int stopSwitchPinBack = 2;   // Công tắc tắt đằng sau(Stop)
const int stopSwitchPinStop = 3;   // Công tắc tắt dừng(Stop)

// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

// Khai bao 1 so bien
int checkStop = 0;
int engineStatus = 0;

void setup() {
  Serial.begin(115200);

  // sets the pins as outputs:
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(stopSwitchPinFront, INPUT_PULLUP);  // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinBack, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinStop, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên

  // configure LEDC PWM
  ledcAttachChannel(ENA, freq, resolution, pwmChannel);
}

void loop() {
  // // Move the DC motor forward at maximum speed
  // Serial.println("Moving Forward");
  // motor_forward(MAX_SPEED);
  // delay(5000);
  // // Move the DC motor backward at maximum speed
  // Serial.println("Moving Backward");
  // motor_backward(MAX_SPEED);
  // delay(5000);
  // // Stop the DC motor
  // Serial.println("Stop");
  // motor_stop();
  // delay(5000);

  Serial.print("engineStatus: ");
  Serial.println(engineStatus);
  Serial.print("checkStop: ");
  Serial.println(checkStop);
  Serial.print("cong tac dung: ");
  Serial.println(digitalRead(stopSwitchPinStop));
  Serial.println("====================");

  if (digitalRead(stopSwitchPinFront) == 0) {
    motor_backward(MAX_SPEED);  // động cơ lùi
    engineStatus = 1;
    Serial.println("Dong co lui lai");
  }
  if (digitalRead(stopSwitchPinBack) == 0) {
    motor_forward(MAX_SPEED);
    engineStatus = 1;
    checkStop = 1;
    //motor_1_Dung();  // động cơ stop
    Serial.println("Dong co dung lai");
  }
  if (digitalRead(stopSwitchPinBack) == 1 && checkStop == 1) {
    motor_stop();  // động cơ stop
    checkStop = 0;
    engineStatus = 1;
  }
  // if (digitalRead(stopSwitchPinStop) == 0) {
  //   motor_stop();  // động cơ stop
  //   Serial.println("Dong co stop");
  //   String id = "1";     // Thay thế bằng ID mong muốn
  //   String data = "OK";  // Thay thế bằng dữ liệu thực tế

  //   // Tạo chuỗi theo định dạng "ID DATA"
  //   String sendString = id + " " + data;
  //   // Gửi chuỗi qua SoftwareSerial tới ESP32
  //   //Serial1.println(sendString);  // Gửi chuỗi và xuống dòng để ESP32 nhận biết kết thúc chuỗi

  //   // In chuỗi ra để kiểm tra trên màn hình Serial Monitor
  //   Serial.println("Gửi chuỗi: " + sendString);
  //   // Serial1.print(" 1324");
  //   // Serial1.print("\n");
  // }
  if (digitalRead(stopSwitchPinStop) == 0) {
    if (engineStatus == 0) {
      motor_forward(MAX_SPEED);
      engineStatus = 1;
    } else if (engineStatus == 1) {
      motor_stop();
      engineStatus = 0;
    }
  }
  delay(200);
}

void motor_forward(int speed) {
  speed = constrain(speed, MIN_SPEED, MAX_SPEED);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  ledcWrite(ENA, speed);
}

void motor_backward(int speed) {
  speed = constrain(speed, MIN_SPEED, MAX_SPEED);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  ledcWrite(ENA, speed);
}

void motor_stop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}
