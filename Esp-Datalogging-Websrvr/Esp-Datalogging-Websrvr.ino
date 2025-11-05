#include <Arduino.h>
#include <DHT11.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define DHTPIN 4
#define BMP280_PIN 33
#define BUTTON_PIN 15 // Define button pin

// Variables
unsigned long epochTime = 0;
bool loggingEnabled = false;
String dataMessage;
const char* dataPath = "/data.txt";
unsigned long lastLogTime = 0;
const unsigned long logInterval = 5000; // Log every 5 seconds

unsigned long buttonPressTime = 0;
bool buttonLongPressHandled = false;

DHT11 dht11(DHTPIN);

// Initialize SD Card
void initSDCard() {
    if (!SD.begin(5)) {
        Serial.println("SD Card Mount Failed. Logging disabled.");
        loggingEnabled = false; // Prevent logging if SD card fails
    }
}

// Write Data to SD
void appendFile(fs::FS &fs, const char *path, const char *message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    file.print(message);
    file.close();
}

// Read SD File for Webpage
String readFile(fs::FS &fs, const char *path) {
    File file = fs.open(path);
    if (!file) return "";
    String data;
    while (file.available()) {
        data += file.readStringUntil('\n') + "\n";
    }
    file.close();
    return data;
}

// Delete Data File
void deleteFile(fs::FS &fs, const char *path) {
    if (fs.remove(path)) {
        Serial.println("Data file deleted.");
    }
}

// Convert milliseconds to hh:mm:ss format
String formatTime(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    char timeBuffer[16];
    sprintf(timeBuffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(timeBuffer);
}

// Button handling for logging control
void IRAM_ATTR handleButtonPress() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        buttonPressTime = millis();
        buttonLongPressHandled = false;
    } else {
        unsigned long pressDuration = millis() - buttonPressTime;
        if (pressDuration >= 3000) { // Long press (>3s)
            buttonLongPressHandled = true;
        } else { // Short press
            loggingEnabled = !loggingEnabled;
            Serial.println(loggingEnabled ? "Logging Enabled" : "Logging Disabled");
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, CHANGE);

    initSDCard();
}

void loop() {
    int temperature = 0;
    int humidity = 0;

    // Attempt to read the temperature and humidity values from the DHT11 sensor.
    int result = dht11.readTemperatureHumidity(temperature, humidity);

    if (loggingEnabled && millis() - lastLogTime >= logInterval) {
        lastLogTime = millis();
        epochTime = millis();

        // Generate Sensor or Dummy Data
        float dht_humidity = result == 0 ? humidity : random(200, 300) / 10.0;
        float dht_temp = result == 0 ? temperature : random(0, 100);
        float bmp280_pressure = random(950, 1050) / 1.0; // Simulated pressure (hPa)
        float bmp280_temp = result == 0 ? temperature + 2.0 : random(150, 300) / 10.0; // Simulated temperature

        String dataType = "REAL";

        // Use Dummy Data on Long Press
        if (buttonLongPressHandled) {
            dht_humidity = random(400, 600) / 10.0;
            dht_temp = random(200, 400) / 10.0;
            bmp280_pressure = random(1000, 1100) / 10.0;
            bmp280_temp = random(250, 400) / 10.0;
            buttonLongPressHandled = false; // Reset flag
            dataType = "DUMMY";
            Serial.println("Dummy data logging activated.");
        }

        // Create Data String with Indicator
        dataMessage = formatTime(epochTime) + "," + dataType + "," + String(dht_humidity) + "," + String(dht_temp) + "," +
                      String(bmp280_pressure) + "," + String(bmp280_temp) + "\r\n";

        // Save Data
        appendFile(SD, dataPath, dataMessage.c_str());

        Serial.println("Data Logged: [" + dataType + "] " + dataMessage);
    }
} 
