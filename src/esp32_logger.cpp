#include "esp32_logger.h"

namespace esp32_logger {

std::vector<String> ESP32Logger::logEntries(MAX_LOG_ENTRIES);
int ESP32Logger::logIndex = 0;

void ESP32Logger::init() {
    logEntries.clear();
    logIndex = 0;
    log("ESP32Lgger Logger initialized");
}

String ESP32Logger::getTimeStamp() {
    time_t now = time(nullptr);
    String timeString = ctime(&now);
    timeString.trim();
    return timeString;
}

void ESP32Logger::log(const String& message) {
    String entry = getTimeStamp() + " - " + message;
    logEntries[logIndex] = entry;
    logIndex = (logIndex + 1) % MAX_LOG_ENTRIES;
    Serial.println(entry);
}

void ESP32Logger::logI2C(const String& message) {
    log("I2C: " + message);
}

void ESP32Logger::logWiFi(const String& message) {
    log("WiFi: " + message);
}

void ESP32Logger::logSystem(const String& message) {
    log("System: " + message);
}

String ESP32Logger::getLogHTML() {
    String html = "<html><body><h1>ESP8266 System Log</h1><pre>\n";
    
    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        int idx = (logIndex + i) % MAX_LOG_ENTRIES;
        if (logEntries[idx].length() > 0) {
            html += logEntries[idx] + "\n";
        }
    }
    
    html += "</pre></body></html>";
    return html;
}

/*
String ESP32Logger::getTimeStamp() {
    time_t now = time(nullptr);
    String timeStr = ctime(&now);
    timeStr.trim();
    return timeStr;
}
*/

void ESP32Logger::clearLog() {
    logEntries.clear();
    logIndex = 0;
    log("Log cleared");
}

}