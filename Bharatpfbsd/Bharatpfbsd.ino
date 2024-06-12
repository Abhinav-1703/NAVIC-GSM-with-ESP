#define TINY_GSM_MODEM_SIM7600
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <TinyGsmClient.h>
#include <FirebaseESP32.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>

#define WIFI_SSID "xxxxP"
#define WIFI_PASSWORD "xxxx"
// Create a TinyGPS++ object
TinyGPSPlus gps;

// Define the serial connections
HardwareSerial gpsSerial(1);

// Define the serial interface for the SIM7600 module

// Firebase settings
#define FIREBASE_HOST "xxxxx" // Replace with your Firebase database URL
#define FIREBASE_AUTH "xxxxx" // Replace with your Firebase API key

// Use hardware serial port 2
HardwareSerial SerialAT(2);
TinyGsm modem(SerialAT);

// Firebase Data object
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Define the posting interval (in milliseconds)
const unsigned long postingInterval = 20000; // Replace with your desired posting interval

// SD card file name
const char* offlineDataFile = "/offline_data.txt";

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(10);
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Initialize GPS serial communication
  gpsSerial.begin(115200, SERIAL_8N1, 33, 32);

  // Start the hardware serial communication with the modem
  SerialAT.begin(115200, SERIAL_8N1, 16, 17); // RX2 -> GPIO16, TX2 -> GPIO17
  delay(3000);

  Serial.println("GPS Data Parsing with TinyGPS++ and IST Conversion");

  // Initialize the modem

  // Print modem info
  String modemInfo = modem.getModemInfo();
  Serial.println("Modem Info: " + modemInfo);

  // Check signal quality
  int16_t signalQuality = modem.getSignalQuality();
  Serial.println("Signal quality: " + String(signalQuality));

  // Connect to network
  Serial.print("Connecting to network...");
  if (!modem.gprsConnect("airtelgprs.com", "", "")) { // Replace "your_apn" with the actual APN
    Serial.println(" fail");
  } else {
    Serial.println(" success");
  }

  // Check network connection
  Serial.print("Testing connection...");
  if (modem.isNetworkConnected()) {
    Serial.println(" connected");
  } else {
    Serial.println(" not connected");
  }

  // Initialize Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());

    if (gps.location.isUpdated()) {
      Serial.print("Latitude: ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("Longitude: ");
      Serial.println(gps.location.lng(), 6);
    }

    if (gps.date.isUpdated() && gps.time.isUpdated()) {
      // Get the UTC time and date from the GPS
      int year = gps.date.year();
      int month = gps.date.month();
      int day = gps.date.day();
      int hour = gps.time.hour();
      int minute = gps.time.minute();
      int second = gps.time.second();

      // Convert to IST (UTC + 5:30)
      minute += 30;
      if (minute >= 60) {
        minute -= 60;
        hour += 1;
      }
      hour += 5;
      if (hour >= 24) {
        hour -= 24;
        day += 1;
        // Adjust month and year for end of month/year
        if ((month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10) && day > 31) {
          day = 1;
          month += 1;
        } else if (month == 12 && day > 31) {
          day = 1;
          month = 1;
          year += 1;
        } else if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
          day = 1;
          month += 1;
        } else if (month == 2) {
          // Leap year check
          bool isLeap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
          if ((isLeap && day > 29) || (!isLeap && day > 28)) {
            day = 1;
            month += 1;
          }
        }
      }

      Serial.print("Date (IST): ");
      Serial.print(day);
      Serial.print("/");
      Serial.print(month);
      Serial.print("/");
      Serial.println(year);

      Serial.print("Time (IST): ");
      if (hour < 10) Serial.print("0");
      Serial.print(hour);
      Serial.print(":");
      if (minute < 10) Serial.print("0");
      Serial.print(minute);
      Serial.print(":");
      if (second < 10) Serial.print("0");
      Serial.println(second);

      // Prepare data for Firebase
      String date = String(day) + "/" + String(month) + "/" + String(year);
      String time = String(hour) + ":" + String(minute) + ":" + String(second);

      float latitude = gps.location.lat();
      float longitude = gps.location.lng();
      float altitude = gps.altitude.meters();
      float speed = gps.speed.kmph();
      int satellites = gps.satellites.value();

      // Push data to Firebase
      if (!pushToFirebase(latitude, longitude, altitude, speed, satellites, date, time)) {
        // If failed to push to Firebase, store data to SD card
        storeDataToSD(latitude, longitude, altitude, speed, satellites, date, time);
      }

      // Try to push stored data to Firebase if network is available
      if (modem.isNetworkConnected()) {
        pushStoredDataToFirebase();
      }

      // Wait for the posting interval
      delay(postingInterval);
    }
  }
}

bool pushToFirebase(float latitude, float longitude, float altitude, float speed, int satellites, String date, String time) {
  String path = "/gps_data";
  FirebaseJson json;

  json.set("latitude", latitude);
  json.set("longitude", longitude);
  json.set("altitude", altitude);
  json.set("speed", speed);
  json.set("satellites", satellites);
  json.set("date", date);
  json.set("time", time);

  if (Firebase.pushJSON(firebaseData, path, json)) {
    Serial.println("Data sent to Firebase successfully.");
    return true;
  } else {
    Serial.print("Failed to send data to Firebase: ");
    Serial.println(firebaseData.errorReason());
    return false;
  }
}

void storeDataToSD(float latitude, float longitude, float altitude, float speed, int satellites, String date, String time) {
  File file = SD.open(offlineDataFile, FILE_APPEND);
  if (file) {
    String data = String(latitude) + "," + String(longitude) + "," + String(altitude) + "," + String(speed) + "," + String(satellites) + "," + date + "," + time + "\n";
    file.print(data);
    file.close();
    Serial.println("Data stored to SD card.");
  } else {
    Serial.println("Failed to open file for writing.");
  }
}

void pushStoredDataToFirebase() {
  File file = SD.open(offlineDataFile);
  if (file) {
    while (file.available()) {
      String data = file.readStringUntil('\n');
      // Parse the stored data
      int firstComma = data.indexOf(',');
      int secondComma = data.indexOf(',', firstComma + 1);
      int thirdComma = data.indexOf(',', secondComma + 1);
      int fourthComma = data.indexOf(',', thirdComma + 1);
      int fifthComma = data.indexOf(',', fourthComma + 1);
      int sixthComma = data.indexOf(',', fifthComma + 1);

      float latitude = data.substring(0, firstComma).toFloat();
      float longitude = data.substring(firstComma + 1, secondComma).toFloat();
      float altitude = data.substring(secondComma + 1, thirdComma).toFloat();
      float speed = data.substring(thirdComma + 1, fourthComma).toFloat();
      int satellites = data.substring(fourthComma + 1, fifthComma).toInt();
      String date = data.substring(fifthComma + 1, sixthComma);
      String time = data.substring(sixthComma + 1);

      if (pushToFirebase(latitude, longitude, altitude, speed, satellites, date, time)) {
        // If push successful, remove the data line from the SD card
        removeLineFromSD(offlineDataFile, data);
      } else {
        // If failed, break the loop to avoid further attempts
        break;
      }
    }
    file.close();
  } else {
    Serial.println("Failed to open file for reading.");
  }
}

void removeLineFromSD(const char* fileName, String lineToRemove) {
  File file = SD.open(fileName, FILE_READ);
  File tempFile = SD.open("/temp.txt", FILE_WRITE);

  if (file && tempFile) {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      if (line != lineToRemove) {
        tempFile.print(line + "\n");
      }
    }
    file.close();
    tempFile.close();

    SD.remove(fileName);
    SD.rename("/temp.txt", fileName);
  } else {
    Serial.println("Failed to open files for removing line.");
  }
}
