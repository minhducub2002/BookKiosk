#include <Keypad.h>
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <string>
#include <WiFi.h>
#include <esp_now.h>

// Define TX and RX pins for UART (change if needed)
#define TXD1 22
#define RXD1 16
// Use Serial1 for UART communication
HardwareSerial mySerial(2);

// Define for FRID Reader ------------------------------------------------------------------------------------------------
#define SS_PIN 5
#define RST_PIN 0
MFRC522 rfid(SS_PIN, RST_PIN);  // Instance of the class
MFRC522::MIFARE_Key key;
// Init array that will store new NUID
byte nuidPICC[4];


// Define for Keyboard Matrix 4x4 ------------------------------------------------------------------------------------------------
#define ROWS 4
#define COLS 4
char keyMap[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
uint8_t rowPins[ROWS] = { 13, 12, 14, 27 };  // GIOP14, GIOP27, GIOP26, GIOP25
uint8_t colPins[COLS] = { 26, 25, 33, 32 };  // GIOP33, GIOP32, GIOP35, GIOP34
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, ROWS, COLS);

// Declare address receiver ------------------------------------------------------------------------------------------------------
uint8_t khoang1[] = { 0x7c, 0x2c, 0x67, 0xc8, 0x4e, 0x78 };  //7c:2c:67:c8:4e:78
uint8_t khoang2[] = { 0x18, 0x8b, 0x0e, 0x2a, 0x90, 0x70 };  //18:8b:0e:2a:90:70

typedef struct data_struct {
  String ID;
  String command;
} data_struct;

data_struct dataSend;
data_struct dataRev;

esp_now_peer_info_t peerInfo;

// Declare variables ------------------------------------------------------------------------------------------------------
int counter = 0;
String keyboardData;
String UID = "";

// Create a structure to hold the readings from each board
data_struct board1;
data_struct board2;
data_struct board3;

// Create an array with all the structures
data_struct boardsStruct[3] = { board1, board2, board3 };

// callback when data is sent
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t* mac_addr, const uint8_t* incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&dataRev, incomingData, sizeof(dataRev));
  Serial.print("ID: ");
  Serial.println(dataRev.ID);
  Serial.print("command: ");
  Serial.println(dataRev.command);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RXD1, TXD1);  // UART setup

  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();  // Init MFRC522
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  delay(100);

  WiFi.mode(WIFI_STA);
  // Setup for ESP_NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  // register peer
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // register first peer
  memcpy(peerInfo.peer_addr, khoang1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // register second peer
  memcpy(peerInfo.peer_addr, khoang2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  delay(100);

  Serial.println("Setup OK!");
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    String keyString = String(key);
    if (keyString.equals("#")) {
      Serial.println("Enter!");
      mySerial.print("Keypad ");
      mySerial.print(keyboardData);
      mySerial.println("");

      // Send to khoang day sach
      dataSend.ID = keyboardData;
      dataSend.command = "RUN";
      esp_err_t result = esp_now_send(0, (uint8_t*)&dataSend, sizeof(dataSend));
      if (result == ESP_OK) {
        Serial.println("Sent with success");
      } else {
        Serial.println("Error sending the data");
      }

      keyboardData = "";
    } else {
      keyboardData += key;
    }
    Serial.println(keyboardData);
    // dataSend.ID = keyboardData;
    // dataSend.command = "OK";
    // esp_err_t result = esp_now_send(0, (uint8_t*)&dataSend, sizeof(dataSend));
  }

  if (rfid.PICC_IsNewCardPresent()) {
    if (rfid.PICC_ReadCardSerial()) {
      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // Check is the PICC of Classic MIFARE type
      if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("Your tag is not of type MIFARE Classic."));
        return;
      }

      Serial.println(F("A new card has been detected."));
      // Store NUID into nuidPICC array
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = rfid.uid.uidByte[i];
      }

      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) {
          UID += " 0";
        } else {
          UID += " ";
        }
        UID += String(rfid.uid.uidByte[i], DEC);
      }
      Serial.println(UID);
      mySerial.print("RFID");
      mySerial.print(UID);
      UID = "";
      // Halt PICC
      rfid.PICC_HaltA();
      // Stop encryption on PCD
      rfid.PCD_StopCrypto1();
    }
  }

  // Check if data is available to read
  if (mySerial.available()) {
    // Read data and display it
    String message = mySerial.readStringUntil('\n');
    Serial.println("Received: " + message);
  }
  delay(100);
}