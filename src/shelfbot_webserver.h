#ifndef SHELFBOT_WEBSERVER_H
#define SHELFBOT_WEBSERVER_H

#include <WebServer.h>
#include "esp32_logger.h"
#include "shelfbot_comms.h"

class ShelfbotWebServer {
private:
    static const char* javascriptContent;

public:
    static void begin();
    static void handle();
    
private:
    static const char* ssid;
    static const char* password;
    static WebServer server;
    static ShelfbotComms comms;
    
    static void initWiFi();
    static String translateEncryptionType(wifi_auth_mode_t encryptionType);
    static void scanNetworks();
    static void connectToNetwork();
    static void setupTime();
    static void handleRoot();
    static void handleLog();
    static void setupEndpoints();
    static void handleMessage();
    static void handleCommand();
    static void handleJavaScript(); 

    static void handleMotorMove();
    static void handleAllMotorsMove();
    static void handleTwist();
    static void handleCapture();
    static void handleTest();
};
#endif