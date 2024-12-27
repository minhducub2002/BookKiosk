// Include the Arduino Stepper Library
#include <Stepper.h>
#include <string>
#include <WiFi.h>
#include <esp_now.h>

#define MAX 2100

// Number of steps per output rotation
const int stepsPerRevolution = 200;
const int enA = 9;
const int enB = 10;
const int IN1 = 5;
const int IN2 = 6;
const int IN3 = 7;
const int IN4 = 8;
// Create Instance of Stepper library
Stepper myStepper(stepsPerRevolution, 5, 6, 7, 8);

int targetRPM = 60;  // Đặt tốc độ mong muốn
int stepDelay;       // Thời gian delay giữa mỗi nửa bước
int stepDelayDown;
int x = 0;
String state;
int zeroPoint = 0;
String currentCompartment;

const int stopSwitchPin = A1;       // Công tắc dừng end point
const int stopSwitchPinTest = A2;   // Công tắc
const int stopSwitchPinTest2 = A3;  // Công tắc


uint8_t storageCompartment1[] = { 0x7c, 0x2c, 0x67, 0xc8, 0x4e, 0x78 };  //7c:2c:67:c8:4e:78
uint8_t storageCompartment2[] = { 0x18, 0x8b, 0x0e, 0x2a, 0x90, 0x70 };  //18:8b:0e:2a:90:70
uint8_t centralCompartment[] = { 0xe8, 0x68, 0xe7, 0x06, 0xfc, 0x34 };   //e8:68:e7:06:fc:34

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

  handleDataToUp();
  handleDataToDown();

  dataRev.ID = "";
  dataRev.command = "";
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void setup() {
  myStepper.setSpeed(10);
  Serial.begin(921600);
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stepDelay = (60000 / (targetRPM * 400));          // 400 bước ở chế độ 1/2 bước
  stepDelayDown = (60000 / (targetRPM * 400)) / 2;  

  pinMode(stopSwitchPin, INPUT_PULLUP);       // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinTest, INPUT_PULLUP);   // Công tắc Stop với điện trở kéo lên
  pinMode(stopSwitchPinTest2, INPUT_PULLUP);  // Công tắc Stop với điện trở kéo lên


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

  peerInfo.channel = 0;
  peerInfo.encrypt = false;


  // Register peer
  memcpy(peerInfo.peer_addr, centralCompartment, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo.peer_addr, storageCompartment1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo.peer_addr, storageCompartment2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ok");
  runForward(10);
  runToZeroPoint();
}

void loop() {
  // myStepper.step(100);
  // while (x < 125) {
  //   halfStep();
  //   Serial.println(x);
  //   x++;
  // }
  if (digitalRead(stopSwitchPinTest) == 0) {
    Serial.println("chay khoang tang 3");
    runForward(1025);  // 1035 +50
  }
  if (digitalRead(stopSwitchPinTest2) == 0) {
    Serial.println("chay khoang tang 2");
    runForward(630);  // 650 +50
  }
  //Serial.println(state);
  // runForward(100);
}

void handleDataToUp() {
  /* neu khoang la 1,2,3,4 thi chay len vi tri X1; 
  neu khoang la 5,6,7,8 thi chay len vi tri X2; 
  neu khoang la 9,10,11,12 thi chay len vi tri X3 */
  if (!dataRev.command.equals("WAIT")) {  // neu la wait thi moi day khoang do sach len cao
    Serial.println("khong len");
    return;
  }
  currentCompartment = dataRev.ID;
  // xử lý chạy động cơ bước
  if (dataRev.ID.equals("1") || dataRev.ID.equals("2") || dataRev.ID.equals("3") || dataRev.ID.equals("4")) {
    Serial.println("chay dong co len tang 3");
    runForward(990);
    dataSend.ID = dataRev.ID;
    dataSend.command = "RUN";
    esp_err_t result = esp_now_send(0, (uint8_t *)&dataSend, sizeof(dataSend));
    if (result == ESP_OK) {
      Serial.println("Sent to Storage compartment success");
    } else {
      Serial.println("Error sending the data");
    }
  } else if (dataRev.ID.equals("5") || dataRev.ID.equals("6") || dataRev.ID.equals("7") || dataRev.ID.equals("8")) {
    Serial.println("chay dong co len tang 2");
    runForward(620);
    dataSend.ID = dataRev.ID;
    dataSend.command = "RUN";
    esp_err_t result = esp_now_send(0, (uint8_t *)&dataSend, sizeof(dataSend));
    if (result == ESP_OK) {
      Serial.println("Sent to Storage compartment success");
    } else {
      Serial.println("Error sending the data");
    }
  }
}

void handleDataToDown() {
  /* neu nhan lenh tu storage compartment la ok thi deliver compartment ve vi tri ban dau */
  if (!dataRev.ID.equals(currentCompartment)) {
    Serial.println("Sai ID");
    return;
  }
  if (dataRev.command.equals("DOWN") && state.equals("UP")) {
    Serial.println("chay dong co ve vi tri 0");
    runToZeroPoint();
    dataSend.ID = currentCompartment;
    dataSend.command = "OK";
    esp_err_t result = esp_now_send(centralCompartment, (uint8_t *)&dataSend, sizeof(dataSend));
    if (result == ESP_OK) {
      Serial.println("Sent to Central compartment success");
    } else {
      Serial.println("Error sending the data");
    }
    currentCompartment = "";
  }
}

void runToZeroPoint() {
  zeroPoint = 0;
  if (state != "DOWN") {
    while (zeroPoint < MAX) {
      halfStep2();
      Serial.println(zeroPoint);
      zeroPoint++;
      if (digitalRead(stopSwitchPin) == 0) {
        zeroPoint = MAX;
        Serial.println("Dong co dung lai");
      }
    }
    if (digitalRead(stopSwitchPin) == 0) {
      while (digitalRead(stopSwitchPin) == 0) {
        halfStep();
        Serial.println("Dong co tien len ham zero point");
      }
    }
  }
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(enA, 0);
  analogWrite(enB, 0);
  state = "DOWN";
  Serial.println(state);
}

void stop() {
  digitalWrite(IN1, !LOW);
  digitalWrite(IN2, !LOW);
  digitalWrite(IN3, !LOW);
  digitalWrite(IN4, !LOW);
}

void runForward(int distance) {
  int x = distance * 1.25;
  while (x > 0) {
    Serial.println("Dong co tien len ham forwawrd");
    Serial.println(state);
    halfStep();
    x--;
  }
  state = "UP";
  Serial.println(state);
}

void halfStep() {
  analogWrite(enA, 200);
  analogWrite(enB, 200);
  digitalWrite(IN1, !HIGH);
  digitalWrite(IN2, !LOW);
  digitalWrite(IN3, !LOW);
  digitalWrite(IN4, !LOW);
  delay(stepDelay);
  digitalWrite(IN1, !HIGH);
  digitalWrite(IN2, !LOW);
  digitalWrite(IN3, !HIGH);
  digitalWrite(IN4, !LOW);
  delay(stepDelay);
  digitalWrite(IN1, !LOW);
  digitalWrite(IN2, !LOW);
  digitalWrite(IN3, !HIGH);
  digitalWrite(IN4, !LOW);
  delay(stepDelay);
  digitalWrite(IN1, !LOW);
  digitalWrite(IN2, !HIGH);
  digitalWrite(IN3, !HIGH);
  digitalWrite(IN4, !LOW);
  delay(stepDelay);
  digitalWrite(IN1, !LOW);
  digitalWrite(IN2, !HIGH);
  digitalWrite(IN3, !LOW);
  digitalWrite(IN4, !LOW);
  delay(stepDelay);
  digitalWrite(IN1, !LOW);
  digitalWrite(IN2, !HIGH);
  digitalWrite(IN3, !LOW);
  digitalWrite(IN4, !HIGH);
  delay(stepDelay);
  digitalWrite(IN1, !LOW);
  digitalWrite(IN2, !LOW);
  digitalWrite(IN3, !LOW);
  digitalWrite(IN4, !HIGH);
  delay(stepDelay);
  digitalWrite(IN1, !HIGH);
  digitalWrite(IN2, !LOW);
  digitalWrite(IN3, !LOW);
  digitalWrite(IN4, !HIGH);
  delay(stepDelay);
}

void halfStep2() {
  analogWrite(enA, 170);
  analogWrite(enB, 170);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(stepDelayDown);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(stepDelayDown);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(stepDelayDown);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  delay(stepDelayDown);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(stepDelayDown);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(stepDelayDown);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(stepDelayDown);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  delay(stepDelayDown);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(stepDelayDown);
}