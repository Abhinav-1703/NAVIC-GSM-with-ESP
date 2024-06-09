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
  Serial.println("GPS Data Parsing with TinyGPS++");
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
    
    if (gps.date.isUpdated()) {
      Serial.print("Date: ");
      Serial.print(gps.date.day());
      Serial.print("/");
      Serial.print(gps.date.month());
      Serial.print("/");
      Serial.println(gps.date.year());
    }

    if (gps.time.isUpdated()) {
      Serial.print("Time: ");
      Serial.print(gps.time.hour());
      Serial.print(":");
      Serial.print(gps.time.minute());
      Serial.print(":");
      Serial.println(gps.time.second());
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
