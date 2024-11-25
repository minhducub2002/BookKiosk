#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Define TX and RX pins for UART (change if needed)
#define TXD1 17
#define RXD1 21
// Use Serial1 for UART communication
HardwareSerial mySerial(2);

// Define for Firebase ---------------------------------------------------------------------------------------------------
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Insert your network credentials
#define WIFI_SSID "DucNg"
#define WIFI_PASSWORD "12345678"
// Insert Firebase project API Key
#define API_KEY "AIzaSyBO0RO39AzXHCz0NL4ZqP5bUrJhMgBRCBM"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://controlling-esp32-default-rtdb.asia-southeast1.firebasedatabase.app/"
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
String ID = "";       // Biến lưu ID nhận được từ esp32_offline
String command = "";  // Biến lưu command nhận được từ esp32_offline

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RXD1, TXD1);  // UART setup

  WiFi.mode(WIFI_STA);
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

  Serial.println("ESP32 Online: ");
}

void loop() {
  // Check if data is available to read
  if (mySerial.available()) {
    // Read data and display it
    String receivedData = mySerial.readStringUntil('\n');
    // Loại bỏ khoảng trắng thừa ở đầu và cuối
    receivedData.trim();

    // Tách chuỗi
    int spaceIndex = receivedData.indexOf(' ');  // Tìm vị trí dấu cách đầu tiên
    if (spaceIndex != -1) {
      ID = receivedData.substring(0, spaceIndex);        // Lấy phần ID
      command = receivedData.substring(spaceIndex + 1);  // Lấy phần command
      //command.trim();                                  // Loại bỏ khoảng trắng dư thừa trong command
    }
    Serial.println(ID);
    Serial.println(command);

    if (ID.equals("Keypad")) {
      if (Firebase.RTDB.setString(&fbdo, "/keyboard/key", command)) {
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }

    if (ID.equals("RFID")) {
      if (Firebase.RTDB.setString(&fbdo, "borrowers/UID", command)) {
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }
    receivedData = "";
    ID = "";
    command = "";
  }

  if (Firebase.ready() && signupOK) {
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
      mySerial.print(intValue);
      mySerial.print(" RUN");
      mySerial.println("");
      intValue = 0;
    }
    delay(100);
  }
}