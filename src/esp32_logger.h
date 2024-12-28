#pragma once
#include <Arduino.h>
#include <vector>

namespace esp32_logger {

const int MAX_LOG_ENTRIES = 100;

class ESP32Logger {
public:
    static void init();
    static void log(const String& message);
    static void logI2C(const String& message);
    static void logWiFi(const String& message);
    static void logSystem(const String& message);
    static String getLogHTML();
    static void clearLog();
    
private:
    static std::vector<String> logEntries;
    static int logIndex;
    static String getTimeStamp();
};

}