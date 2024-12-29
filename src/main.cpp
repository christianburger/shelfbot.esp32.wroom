#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <sys/time.h>
#include "esp32_logger.h"
#include "i2c_master.h"
#include "shelfbot_comms.h"
#include "shelfbot_motor.h"

// SSID and password of Wifi connection:
const char* ssid = "dlink-30C0";
const char* password = "ypics98298";

//Webserver listening on port 80
WebServer server(80);
ShelfbotComms comms;

// simple function to decipher the encryption type of a network
String translateEncryptionType(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2";
        default:
            return "Unknown";
    }
}

// Function to scan and print all networks that can be detected by the ESP32 
void scanNetworks() {
  int numberOfNetworks = WiFi.scanNetworks();
 
  esp32_logger::ESP32Logger::logWiFi("Number of networks found: " + String(numberOfNetworks));
 
  for (int i = 0; i < numberOfNetworks; i++) {
    esp32_logger::ESP32Logger::logWiFi("Network name: " + WiFi.SSID(i));
    esp32_logger::ESP32Logger::logWiFi("Signal strength: " + String(WiFi.RSSI(i)));
    esp32_logger::ESP32Logger::logWiFi("MAC address: " + WiFi.BSSIDstr(i));
    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    esp32_logger::ESP32Logger::logWiFi("Encryption type: " + encryptionTypeDescription);
    esp32_logger::ESP32Logger::logWiFi("-----------------------");
  }
}

// function to connect to a Wifi network -> note that there is no time-out function on this
void connectToNetwork() {
  WiFi.begin(ssid, password);
  esp32_logger::ESP32Logger::logWiFi("Establishing connection to WiFi.");
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    esp32_logger::ESP32Logger::logWiFi(".");
  }
  esp32_logger::ESP32Logger::logWiFi("Connected to network");
}

void handleRoot() {
    time_t now = time(nullptr);
    String timeString = ctime(&now);
    timeString.trim();  
    
    unsigned long rawTime = millis();
    unsigned long ms = rawTime % 1000;
    unsigned long seconds = (rawTime / 1000) % 60;
    unsigned long minutes = (rawTime / 60000) % 60;
    unsigned long hours = (rawTime / 3600000);
    
    String html = "<html><body>";
    html += "<h1>Shelfbot Status</h1>";
    html += "<div>Current Time: " + timeString + "</div>";
    html += "<div>Raw Uptime: " + String(rawTime) + " ms</div>";
    html += "<div>Uptime: " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s " + String(ms) + "ms</div>";
    html += "<div>WiFi SSID: " + WiFi.SSID() + "</div>";
    html += "<div>Signal Strength: " + String(WiFi.RSSI()) + " dBm</div>";
    html += "<div>IP Address: " + WiFi.localIP().toString() + "</div>";
    html += "<div>MAC Address: " + WiFi.macAddress() + "</div>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleLog() {
    if (server.hasArg("clear")) {
        esp32_logger::ESP32Logger::clearLog();
        server.send(200, "text/plain", "Log cleared");
        return;
    }
    server.send(200, "text/html", esp32_logger::ESP32Logger::getLogHTML());
}

void handleMessage() {
    if (server.hasArg("text")) {
        String message = server.arg("text");
        esp32_logger::ESP32Logger::log(message);
        server.send(200, "text/plain", "Message logged: " + message);
    } else {
        server.send(400, "text/plain", "Missing text parameter");
    }
}

void handleCommand() {
    if (server.hasArg("cmd")) {
        String cmdString = server.arg("cmd");
        esp32_logger::ESP32Logger::log("Command received: " + cmdString);
        
        String response = "";
        //String response = comms.handleCommand(cmdString response = I2CSlave::getLastResponse();
        server.send(200, "text/plain", response);
        esp32_logger::ESP32Logger::log("Command response: " + response);
    } else {
        server.send(400, "text/plain", comms.formatResponse(RESP_ERR_PARAM, "Missing cmd parameter"));
    }
}

void setupMessageEndpoint() {
    server.on("/log", handleLog);
    server.on("/message", handleMessage);
    server.on("/command", handleCommand);
}

void setupWebServer() {
    server.on("/", handleRoot);
    setupMessageEndpoint();
    server.begin();
    esp32_logger::ESP32Logger::logSystem("HTTP server started at http://" + WiFi.localIP().toString());
}

void handleWebServer() {
    server.handleClient();
}

void setupTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);
    tzset();
}

void initLogging() {
    esp32_logger::ESP32Logger::init();
    esp32_logger::ESP32Logger::logSystem("ESP8266 System startup");
}

void initI2C() {
  esp32_logger::ESP32Logger::logI2C("device started as i2c master");
  Serial.println("\nStarting I2C Master");
  I2CMaster::begin();
}

void initWiFi() {
    scanNetworks();
    connectToNetwork();
    
    esp32_logger::ESP32Logger::logWiFi("MAC Address: " + WiFi.macAddress());
    esp32_logger::ESP32Logger::logWiFi("IP Address: " + WiFi.localIP().toString());
}

void initWebServer() {
    setupTime();
    setupWebServer();
    esp32_logger::ESP32Logger::logSystem("Web server started at http://" + WiFi.localIP().toString());
}

void testAllCommands() {
    Serial.println("\n=== Testing All Commands ===");

    // Basic system commands
    ShelfbotComms::sendCommand(CMD_GET_TEMP);
    ShelfbotComms::sendCommand(CMD_SET_LED, 1);
    ShelfbotComms::sendCommand(CMD_READ_ADC, 0);
    ShelfbotComms::sendCommand(CMD_SET_PWM, 128);
    ShelfbotComms::sendCommand(CMD_GET_STATUS);

    // Set all motors
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_1, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_2, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_3, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_4, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_5, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_6, 1000);

    // Get all motor positions
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_1_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_2_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_3_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_4_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_5_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_6_POS);

    // Get all motor velocities
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_1_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_2_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_3_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_4_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_5_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_6_VEL);

    // Stop individual motors
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_1);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_2);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_3);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_4);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_5);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_6);

    // System commands
    ShelfbotComms::sendCommand(CMD_STOP_ALL);
    ShelfbotComms::sendCommand(CMD_GET_BATTERY);
    ShelfbotComms::sendCommand(CMD_GET_SYSTEM);

    // Unformatted command sent as string, should return an error
    String result = I2CMaster::communicateWithSlave(I2C_SLAVE_ADDR, "MASTER device: send ACK!");
    Serial.println(result);

    Serial.println("=== Test Complete ===\n");
}

void nonBlockingMotorRoutine(int delayValue) {
  Serial.println("Moving to position 1000... ShelfbotMotor::nonBlockingMoveAllMotors(1000)");
  ShelfbotMotor::nonBlockingMoveAllMotors(1000);
  ShelfbotMotor::printMotorSpeeds();
  delay(delayValue);

  Serial.println("Moving to position 0... ShelfbotMotor::nonBlockingMoveAllMotors(0)");
  ShelfbotMotor::nonBlockingMoveAllMotors(0);
  ShelfbotMotor::printMotorSpeeds();
  delay(delayValue);

  Serial.println("Moving to position -2000... ShelfbotMotor::nonBlockingMoveAllMotors(-2000)");
  ShelfbotMotor::nonBlockingMoveAllMotors(-2000);
  ShelfbotMotor::printMotorSpeeds();
  delay(delayValue);

  Serial.println("Moving to position 20000... ShelfbotMotor::nonBlockingMoveAllMotors(-2000)");
  ShelfbotMotor::nonBlockingMoveAllMotors(20000);
  ShelfbotMotor::printMotorSpeeds();
  delay(delayValue);
}

void motorRoutine(int delayValue) {
    Serial.println("Moving to position 1000...");
    ShelfbotMotor::moveAllMotors(1000);
    delay(2000);
    delay(delayValue);
    
    Serial.println("Moving to position 0...");
    ShelfbotMotor::moveAllMotors(0);
    delay(delayValue);
    
    Serial.println("Moving to position -1000...");
    ShelfbotMotor::moveAllMotors(-1000);
    delay(delayValue);
 
    Serial.println("Moving to position 0...");
    ShelfbotMotor::moveAllMotors(0);
    delay(delayValue);

    Serial.println("Moving to position -2000...");
    ShelfbotMotor::moveAllMotors(2000);
    delay(delayValue);
 
    Serial.println("Moving to position 0...");
    ShelfbotMotor::moveAllMotors(0);
    delay(delayValue);

    Serial.println("Moving to position -2000...");
    ShelfbotMotor::moveAllMotors(-2000);
    delay(delayValue);
    
    // Print motor speeds for monitoring
    ShelfbotMotor::printMotorSpeeds();
}


void setup() {
    Serial.begin(115200);
    delay(1000);
    setupTime();
    initLogging();
    initI2C();
    initWiFi();
    initWebServer();
    I2CMaster::scanBus();
    testAllCommands();
    
    // Initialize motor control
    ShelfbotMotor::begin();
    
    // Set initial speeds and acceleration
    ShelfbotMotor::setAllMotorSpeeds(4000);

    for (int i=0; i<2; i++) {
        nonBlockingMotorRoutine(2000);
    }
}

void loop() {

  /*
  ShelfbotComms::sendCommand(CMD_SET_MOTOR_1, 1000);
  delay(4000);
  ShelfbotComms::sendCommand(CMD_SET_MOTOR_1, 0);
  delay(4000);
 
  ShelfbotComms::sendCommand(CMD_SET_MOTOR_2, 1000);
  delay(4000);
  ShelfbotComms::sendCommand(CMD_SET_MOTOR_2, 0);
  delay(4000);
 
  ShelfbotComms::sendCommand(CMD_SET_MOTOR_3, 1000);
  delay(4000);
  ShelfbotComms::sendCommand(CMD_SET_MOTOR_3, 0);
  delay(4000);

  ShelfbotComms::sendCommand(CMD_SET_MOTOR_4, 1000);
  delay(4000);
  ShelfbotComms::sendCommand(CMD_SET_MOTOR_4, 0);
  delay(4000);
  */

  delay(40);
  handleWebServer();
}
