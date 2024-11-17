#include <esp_now.h>
#include <WiFi.h>

#define IN1 6
#define IN2 5
#define ENA 7
#define MAX_SPEED 255  //từ 0-255
#define MIN_SPEED 0

const int stopSwitchPinFront = A1;  // Công tắc tắt đằng trước(Stop)
const int stopSwitchPinBack = A2;   // Công tắc tắt đằng sau(Stop)
const int stopSwitchPinStop = A3;   // Công tắc tắt dừng(Stop)

uint8_t khoangTrungTam[] = { 0xe8, 0x68, 0xe7, 0x06, 0xfc, 0x34 };  //e8:68:e7:06:fc:34

//Structure to receive data
//Must match the sender structure
typedef struct data {
  String ID;
  String command;
} data_receiver;

//Create a struct_message called dataRev
data dataRev;
data dataSend;


// Create peer interface
esp_now_peer_info_t peerInfo;

int checkStop = 0;
int runFlag = 0;

//callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&dataRev, incomingData, sizeof(dataRev));

  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("ID: ");
  Serial.println(dataRev.ID);
  Serial.print("command: ");
  Serial.println(dataRev.command);
  Serial.println();

  if (!dataRev.ID.equals("1")) {
    return;
  }

  if (dataRev.command.equals("RUN")) {
    Serial.println("run DC motor");
    motor_1_Tien(MAX_SPEED);  // động cơ tiến
  }
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void setup() {
  //Initialize Serial Monitor
  Serial.begin(115200);

  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, khoangTrungTam, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(stopSwitchPinFront, INPUT_PULLUP);  // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinBack, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinStop, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên

  Serial.println("ok");
}

void loop() {

  // Serial.print("cong tac truoc: ");
  // Serial.println(digitalRead(stopSwitchPinFront));
  // Serial.print("cong tac sau: ");
  // Serial.println(digitalRead(stopSwitchPinBack));
  // Serial.print("cong tac dung: ");
  // Serial.println(digitalRead(stopSwitchPinStop));
  // Serial.println("***");

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
    dataSend.ID = "1";
    dataSend.command = "OK";
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(khoangTrungTam, (uint8_t *)&dataSend, sizeof(dataSend));

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    } else {
      Serial.println("Error sending the data");
    }
    delay(200);
  }
  delay(200);
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