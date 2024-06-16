#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Define the serial connections
HardwareSerial gpsSerial(1);

// Define the serial interface for the SIM7600 module
#define MODEM_PWRKEY 4
#define MODEM_POWER_ON 23
#define MODEM_RST 5

// LED pin definitions
#define LED_INTERNET 14

// ThingSpeak settings
const char* server = "api.thingspeak.com";
const String apiKey = "VS0F2N4CWNU3DA6K"; // Replace with your ThingSpeak API key
const unsigned long postingInterval = 2000; // Interval between updates (in milliseconds)

unsigned long lastPostingTime = 0;
bool isBlinking = false;
bool isGpsOn = false;
bool isInternetOn = false;

// Use hardware serial port 2
HardwareSerial SerialAT(2);
TinyGsm modem(SerialAT);

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(10);

  // Initialize LED pins
  pinMode(LED_INTERNET, OUTPUT);

  // Initialize GPS serial communication
  gpsSerial.begin(115200, SERIAL_8N1, 16, 17);

  // Start the hardware serial communication with the modem
  SerialAT.begin(115200, SERIAL_8N1, 27, 26); // RX2 -> GPIO27, TX2 -> GPIO26
  delay(3000);

  Serial.println("GPS Data Parsing with TinyGPS++");

  // Initialize the modem
  Serial.println("Initializing modem...");
  modem.restart();

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
    isInternetOn = false;
  } else {
    Serial.println(" success");
    isInternetOn = true;
  }

  // Check network connection
  Serial.print("Testing connection...");
  if (modem.isNetworkConnected()) {
    Serial.println(" connected");
    isInternetOn = true;
  } else {
    Serial.println(" not connected");
    isInternetOn = false;
  }
}

void loop() {
  // Continuously read and encode GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Check if GPS has satellites
  isGpsOn = (gps.satellites.value() > 0);

  // Check if it's time to send data to ThingSpeak
  if (millis() - lastPostingTime >= postingInterval) {
    lastPostingTime = millis();

    // Check if the number of satellites is not zero
    if (isGpsOn) {
      Serial.println("-----GPS Data-----");

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
      }

      if (gps.altitude.isUpdated()) {
        Serial.print("Altitude: ");
        Serial.println(gps.altitude.meters());
      }

      if (gps.speed.isUpdated()) {
        Serial.print("Speed: ");
        Serial.println(gps.speed.kmph());
      }

      if (gps.satellites.isUpdated()) {
        Serial.print("Satellites: ");
        Serial.println(gps.satellites.value());
      }

      // Prepare data for ThingSpeak
      float field1 = gps.satellites.value();
      float field2 = gps.speed.kmph();
      float field3 = gps.location.lat();
      float field4 = gps.location.lng();
      float field5 = gps.altitude.meters();

      // Print data to be sent to ThingSpeak
      Serial.println("Preparing data to send to ThingSpeak:");
      Serial.print("Satellites: ");
      Serial.println(field1);
      Serial.print("Speed: ");
      Serial.println(field2);
      Serial.print("Latitude: ");
      Serial.println(field3, 6);
      Serial.print("Longitude: ");
      Serial.println(field4, 6);
      Serial.print("Altitude: ");
      Serial.println(field5);

      if (!pushToThingSpeak(field1, field2, field3, field4, field5)) {
        reinitializeModem();
      }
    } else {
      Serial.println("No updated GPS data available or no satellites detected.");
    }
  }

  // Handle LED indications
  handleLED();
}

bool pushToThingSpeak(float field1, float field2, float field3, float field4, float field5) {
  // Create HTTP client instance
  TinyGsmClient client(modem);
  HttpClient http(client, server, 80);

  // Build the URL with the API key and data
  String url = "/update?api_key=" + apiKey;
  url += "&field1=" + String(field1);
  url += "&field2=" + String(field2);
  url += "&field3=" + String(field3, 6);
  url += "&field4=" + String(field4, 6);
  url += "&field5=" + String(field5);

  // Send HTTP GET request
  http.get(url);

  // Check response
  int statusCode = http.responseStatusCode();
  if (statusCode == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
    return true;
  } else {
    Serial.print("Failed to send data to ThingSpeak. Status code: ");
    Serial.println(statusCode);
    return false;
  }
}

void reinitializeModem() {
  Serial.println("Reinitializing modem...");
  modem.restart();
  delay(3000);

  Serial.print("Reconnecting to network...");
  if (!modem.gprsConnect("airtelgprs.com", "", "")) { // Replace "your_apn" with the actual APN
    Serial.println(" fail");
    isInternetOn = false;
  } else {
    Serial.println(" success");
    isInternetOn = true;
  }

  // Check network connection
  Serial.print("Testing connection...");
  if (modem.isNetworkConnected()) {
    Serial.println(" connected");
    isInternetOn = true;
  } else {
    Serial.println(" not connected");
    isInternetOn = false;
  }
}

void handleLED() {
  static unsigned long lastBlinkTime = 0;
  if (isInternetOn && isGpsOn) {
    // Blink twice per second
    if (millis() - lastBlinkTime >= 250) { // Blink every 250 milliseconds
      lastBlinkTime = millis();
      digitalWrite(LED_INTERNET, !digitalRead(LED_INTERNET));
    }
  } else if (isGpsOn) {
    // Blink once every 2 seconds
    if (millis() - lastBlinkTime >= 1000) { // Blink every 1000 milliseconds
      lastBlinkTime = millis();
      digitalWrite(LED_INTERNET, !digitalRead(LED_INTERNET));
    }
  } else if (isInternetOn) {
    // LED steady on
    digitalWrite(LED_INTERNET, HIGH);
  } else {
    // LED off
    digitalWrite(LED_INTERNET, LOW);
  }
}
