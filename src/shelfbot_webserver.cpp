#include <ArduinoJson.h>
#include "shelfbot_webserver.h"
#include "shelfbot_motor.h"

WebServer ShelfbotWebServer::server(80);
ShelfbotComms ShelfbotWebServer::comms;

const char* ShelfbotWebServer::ssid = "dlink-30C0";
const char* ShelfbotWebServer::password = "ypics98298";

void ShelfbotWebServer::begin() {
    setupTime();
    initWiFi();
    setupEndpoints();
    server.begin();
    esp32_logger::ESP32Logger::logSystem("HTTP server started at http://" + WiFi.localIP().toString());
}

void ShelfbotWebServer::initWiFi() {
    scanNetworks();
    connectToNetwork();
    esp32_logger::ESP32Logger::logWiFi("MAC Address: " + WiFi.macAddress());
    esp32_logger::ESP32Logger::logWiFi("IP Address: " + WiFi.localIP().toString());
}

String ShelfbotWebServer::translateEncryptionType(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        default: return "Unknown";
    }
}

void ShelfbotWebServer::scanNetworks() {
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

void ShelfbotWebServer::connectToNetwork() {
    WiFi.begin(ssid, password);
    esp32_logger::ESP32Logger::logWiFi("Establishing connection to WiFi.");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        esp32_logger::ESP32Logger::logWiFi(".");
    }
    esp32_logger::ESP32Logger::logWiFi("Connected to network");
}

void ShelfbotWebServer::setupTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);
    tzset();
}

const char* ShelfbotWebServer::javascriptContent = R"(
    function sendCommand(motor, position) {
        const cmd = `<<M${motor}{{${position}}}##3F$$>>`;
        fetch(`/command?cmd=${encodeURIComponent(cmd)}`)
            .then(response => response.text())
            .then(data => console.log(data));
    }

    function setSpeed(speed) {
        const cmd = `<<SP{{${speed}}}##3F$$>>`;
        fetch(`/command?cmd=${encodeURIComponent(cmd)}`)
            .then(response => response.text())
            .then(data => console.log(data));
    }
)";

void ShelfbotWebServer::handleJavaScript() {
    server.send(200, "application/javascript", javascriptContent);
}

void ShelfbotWebServer::handle() {
    server.handleClient();
}

void ShelfbotWebServer::handleRoot() {
    time_t now = time(nullptr);
    String timeString = ctime(&now);
    timeString.trim();
    
    unsigned long rawTime = millis();
    unsigned long ms = rawTime % 1000;
    unsigned long seconds = (rawTime / 1000) % 60;
    unsigned long minutes = (rawTime / 60000) % 60;
    unsigned long hours = (rawTime / 3600000);
    
    String html = "<html><head>";
    html += "<script src='/control.js'></script>";
    html += "</head><body>";
    html += "<h1>Shelfbot Status</h1>";
    html += "<div>Current Time: " + timeString + "</div>";
    html += "<div>Raw Uptime: " + String(rawTime) + " ms</div>";
    html += "<div>Uptime: " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s " + String(ms) + "ms</div>";
    html += "<div>WiFi SSID: " + WiFi.SSID() + "</div>";
    html += "<div>Signal Strength: " + String(WiFi.RSSI()) + " dBm</div>";
    html += "<div>IP Address: " + WiFi.localIP().toString() + "</div>";
    html += "<div>MAC Address: " + WiFi.macAddress() + "</div>";
    
    // Add motor control UI
    html += "<div style='margin-top: 20px'>";
    html += "<h2>Motor Control</h2>";
    for (int i = 1; i <= 6; i++) {
        html += "<div style='margin: 10px'>";
        html += "<button onclick='sendCommand(" + String(i) + ", 4000)'>Motor " + String(i) + " Forward</button>";
        html += "<button onclick='sendCommand(" + String(i) + ", 0)'>Motor " + String(i) + " Back</button>";
        html += "</div>";
    }
    html += "<div style='margin: 10px'>";
    html += "<input type='number' id='speed' value='4000'>";
    html += "<button onclick='setSpeed(document.getElementById(\"speed\").value)'>Set Speed</button>";
    html += "</div>";
    html += "</div>";
    
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void ShelfbotWebServer::handleLog() {
    if (server.hasArg("clear")) {
        esp32_logger::ESP32Logger::clearLog();
        server.send(200, "text/plain", "Log cleared");
        return;
    }
    server.send(200, "text/html", esp32_logger::ESP32Logger::getLogHTML());
}

void ShelfbotWebServer::handleMessage() {
    if (server.hasArg("text")) {
        String message = server.arg("text");
        esp32_logger::ESP32Logger::log(message);
        server.send(200, "text/html", "<html><body>Message logged: " + message + "</body></html>");
    } else {
        server.send(400, "text/html", "<html><body>Error: Missing text parameter</body></html>");
    }
}

void ShelfbotWebServer::handleCommand() {
    if (server.hasArg("cmd")) {
        String cmdString = server.arg("cmd");
        esp32_logger::ESP32Logger::log("Command received: " + cmdString);
        
        // Parse the raw command and value
        CommandType cmd = ShelfbotComms::parseCommand(cmdString);
        String value = ShelfbotComms::parseValue(cmdString);
        
        // Format with proper markers and checksum
        String formattedCmd = ShelfbotComms::formatCommand(cmd, value);
        
        char cmdBuffer[32];
        formattedCmd.toCharArray(cmdBuffer, sizeof(cmdBuffer));
        String response = ShelfbotComms::handleCommand(cmdBuffer);
        
        server.send(200, "text/html", response);
    }
}

void ShelfbotWebServer::setupEndpoints() {
    Serial.println("\n=== Setting up Web Endpoints ===");
    
    // Existing GET endpoints
    server.on("/", handleRoot);
    server.on("/log", handleLog);
    server.on("/message", handleMessage);
    server.on("/command", handleCommand);
    server.on("/control.js", handleJavaScript);

    // New POST endpoints with debug prints
    Serial.println("Registering POST /motor endpoint");
    server.on("/motor", HTTP_POST, handleMotorMove);
    
    Serial.println("Registering POST /motors endpoint");
    server.on("/motors", HTTP_POST, handleAllMotorsMove);
    
    Serial.println("Registering POST /twist endpoint");
    server.on("/twist", HTTP_POST, handleTwist);
    
    Serial.println("Registering POST /capture endpoint");
    server.on("/capture", HTTP_POST, handleCapture);
    
    Serial.println("=== Web Endpoints Setup Complete ===\n");
    Serial.println("Registering POST /test endpoint");
    server.on("/test", HTTP_POST, handleTest);
}
void ShelfbotWebServer::handleMotorMove() {
    Serial.println("\n=== Motor Move Request ===");
    
    if (!server.hasArg("plain")) {
        Serial.println("Error: Missing request body");
        server.send(400, "text/plain", "Missing body");
        return;
    }
    
    String body = server.arg("plain");
    Serial.print("Received body: ");
    Serial.println(body);
    
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }
    
    uint8_t motor = doc["motor"];
    long position = doc["position"];
    long speed = doc["speed"];
    
    Serial.printf("Moving motor %d to position %ld at speed %ld\n", motor, position, speed);
    
    String response = ShelfbotMotor::setMotorPosition(motor, position);
    Serial.print("Motor response: ");
    Serial.println(response);
    
    server.send(200, "application/json", response);
    Serial.println("=== Request Complete ===\n");
}
void ShelfbotWebServer::handleAllMotorsMove() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }
    
    String body = server.arg("plain");
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }
    
    long position = doc["position"];
    long speed = doc["speed"];
    bool nonBlocking = doc["nonBlocking"] | false;
    
    ShelfbotMotor::moveAllMotors(position, speed, nonBlocking);
    server.send(200, "application/json", "{\"status\":\"moving\"}");
}

void ShelfbotWebServer::handleTwist() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }
    
    String body = server.arg("plain");
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }
    
    String direction = doc["direction"];
    long speed = doc["speed"];
    
    // Left motors move opposite to right motors for twist
    if (direction == "left") {
        ShelfbotMotor::setMotorPosition(0, 1000);
        ShelfbotMotor::setMotorPosition(2, 1000);
        ShelfbotMotor::setMotorPosition(1, -1000);
        ShelfbotMotor::setMotorPosition(3, -1000);
    } else {
        ShelfbotMotor::setMotorPosition(0, -1000);
        ShelfbotMotor::setMotorPosition(2, -1000);
        ShelfbotMotor::setMotorPosition(1, 1000);
        ShelfbotMotor::setMotorPosition(3, 1000);
    }
    
    server.send(200, "application/json", "{\"status\":\"twisting\"}");
}

void ShelfbotWebServer::handleCapture() {
    server.send(200, "application/json", "{\"status\":\"captured\"}");
}

void ShelfbotWebServer::handleTest() {
    Serial.println("Test endpoint hit!");
    server.send(200, "text/plain", "Test endpoint working");
}