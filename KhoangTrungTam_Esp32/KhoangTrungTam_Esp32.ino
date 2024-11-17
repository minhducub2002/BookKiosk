#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>
#include <string>


// Define UART PIN
#define TX_PIN 22
#define RX_PIN 21
HardwareSerial mySerial1(1);

// Define for FRID Reader ------------------------------------------------------------------------------------------------
#define SS_PIN 5
#define RST_PIN 0
MFRC522 rfid(SS_PIN, RST_PIN);  // Instance of the class
MFRC522::MIFARE_Key key;
// Init array that will store new NUID
byte nuidPICC[4];

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

void setup() {
  Serial.begin(115200);
  mySerial1.begin(4800, SERIAL_8N1, RX_PIN, TX_PIN);  // TX_Pin - RX_Pin
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
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 100 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (mySerial1.available()) {
      String inputString = mySerial1.readStringUntil('\n');  // Đọc chuỗi cho đến khi gặp ký tự xuống dòng
      Serial.println(inputString);
      inputString.trim();  // Loại bỏ khoảng trắng thừa

      // Tìm vị trí của khoảng trắng giữa ID và DATA
      int separatorIndex = inputString.indexOf(' ');

      if (separatorIndex != -1) {
        // Tách ID và DATA
        IdReceiver = inputString.substring(0, separatorIndex);
        DataReceiver = inputString.substring(separatorIndex + 1);

        // In ra kết quả
        Serial.println("ID: " + IdReceiver);
        Serial.println("DATA: " + DataReceiver);
      } else {
        Serial.println("Sai định dạng, không tìm thấy khoảng trắng giữa ID và DATA");
      }
    }

    if (Firebase.RTDB.getInt(&fbdo, "/selected_book/book_index")) {
      if (fbdo.dataType() == "int") {
        intValue = fbdo.intData();
        Serial.println(intValue);
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    //  if (Firebase.RTDB.getString(&fbdo, "/selected_book")) {

    //     stringValue = fbdo.stringData();
    //     Serial.println(stringValue);

    // } else {
    //   Serial.println(fbdo.errorReason());
    // }

    if (intValue != 0) {
      Serial.print(intValue);
      Serial.print(" RUN");
      Serial.println("");
      mySerial1.print(intValue);
      mySerial1.print(" RUN");
      mySerial1.println("");
      intValue = 0;
    }
    // else {
    //   Serial.print("01 ");
    //   Serial.print(intValue);
    //   Serial.print(" STOP");
    //   Serial.println("");
    //   mySerial1.print("01 ");
    //   mySerial1.print(intValue);
    //   mySerial1.print(" STOP");
    //   mySerial1.println("");
    // }

    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
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

        // tim 1 o ben duoi
        if (rfid.uid.uidByte[0] != nuidPICC[0] || rfid.uid.uidByte[1] != nuidPICC[1] || rfid.uid.uidByte[2] != nuidPICC[2] || rfid.uid.uidByte[3] != nuidPICC[3]) {

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

        } else Serial.println(F("Card read previously."));
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

        //mySerial1.println(mssage);
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