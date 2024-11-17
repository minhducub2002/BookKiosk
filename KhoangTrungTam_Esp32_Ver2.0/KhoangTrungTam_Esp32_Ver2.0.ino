#include <Keypad.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <MFRC522.h>
#include <string>
#include <esp_now.h>

// Define for Firebase ---------------------------------------------------------------------------------------------------
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Insert your network credentials
#define WIFI_SSID "DucNg"
#define WIFI_PASSWORD "12345678"
// Insert Firebase project API Key
#define API_KEY "AIzaSyBe6GqkiaH3G8k9EltS6YBHXYI3jXLKmbo"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://library-ea8e5-default-rtdb.asia-southeast1.firebasedatabase.app/"
//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

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
uint8_t khoang2[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

typedef struct data_struct {
  String ID;
  String command;
} data_struct;

data_struct dataSend;
data_struct dataRev;

esp_now_peer_info_t peerInfo;

// Declare variables ------------------------------------------------------------------------------------------------------
unsigned long sendDataPrevMillis = 0;
String stringValue;
int intValue = 0;
float floatValue;
bool signupOK = false;
int push_check = 0;
String UID = "";

String IdReceiver;
String DataReceiver;

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
  // memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  // if (esp_now_add_peer(&peerInfo) != ESP_OK){
  //   Serial.println("Failed to add peer");
  //   return;
  // }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  delay(100);

  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();  // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  delay(100);

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Firebase.ready() && signupOK) {
    char key = keypad.getKey();
    if (key) {
      Serial.println(key);
      String keyString = String(key);
      dataSend.ID = keyString;
      dataSend.command = "OK";
      esp_err_t result = esp_now_send(0, (uint8_t*)&dataSend, sizeof(dataSend));
      if (result == ESP_OK) {
        Serial.println("Sent with success");
      } else {
        Serial.println("Error sending the data");
      }
      if (Firebase.RTDB.setString(&fbdo, "/keyboard/key", keyString)) {
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
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
        Serial.print("send: ");
        Serial.println(UID);
        if (Firebase.RTDB.setString(&fbdo, "borrowers/UID", UID)) {
          Serial.println("PASSED");
          Serial.println("PATH: " + fbdo.dataPath());
          Serial.println("TYPE: " + fbdo.dataType());
        } else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }

        //} else Serial.println(F("Card read previously."));
        //1 ne

        // Serial.println(F("The NUID tag is:"));
        // Serial.print(F("In hex: "));
        // printHex(rfid.uid.uidByte, rfid.uid.size);
        // Serial.println();
        // Serial.print(F("In dec: "));
        // printDec(rfid.uid.uidByte, rfid.uid.size);
        // Serial.println();
        UID = "";
        // Halt PICC
        rfid.PICC_HaltA();
        // Stop encryption on PCD
        rfid.PCD_StopCrypto1();

        if (Firebase.RTDB.getInt(&fbdo, "/selected_book/book_index")) {
          if (fbdo.dataType() == "int") {
            intValue = fbdo.intData();
            Serial.println(intValue);
          }
        } else {
          Serial.println(fbdo.errorReason());
        }

        if (intValue != 0) {
          Serial.print(intValue);
          Serial.print(" RUN");
          Serial.println("");
          intValue = 0;
        }
      }
    }
  }
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(' ');
    Serial.print(buffer[i], DEC);
  }
}
