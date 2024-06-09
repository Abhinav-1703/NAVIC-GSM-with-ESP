#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Define the serial connections
HardwareSerial gpsSerial(1);

void setup() {
  // Start the hardware serial for GPS module at 115200 baud rate
  gpsSerial.begin(115200, SERIAL_8N1, 16, 17);
  
  // Start the serial monitor
  Serial.begin(115200);
  Serial.println("GPS Data Parsing with TinyGPS++ and IST Conversion");
}

void loop() {
  // This sketch displays information every time a new sentence is correctly encoded.
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
  }
}
